#include <stdio.h>
#include <string.h>
#include "main.h"
#include "ctrl.h"
#include "chip.h"
#include "fsm.h"

// ---------------------------------------------------------------------
// Internal bus-sequencing primitives. Static - nothing outside this
// file needs raw pin-level control; all external access goes through
// ---------------------------------------------------------------------

// Controller drives CMD/ADDR/CS# pins for one event. Never touches
// ck0/ck1 - clock is owned exclusively by ctrl_toggle_clock().
static void ctrl_drive_command(struct phy_pins_t *pins, enum event_t event, unsigned rank, unsigned bank, unsigned short addr) {
    // Default bus state: Deselect both ranks (all control lines high)
    // Note: ck0/ck1 are NOT touched here so the clock runs continuously
    pins->cke = 1;
    pins->reset_n = 1;
    pins->cs0_n = 1;
    pins->cs1_n = 1;
    pins->ras_n = 1;
    pins->cas_n = 1;
    pins->we_n = 1;
    pins->odt = 0;
    pins->ba.val = (unsigned char)bank;
    pins->addr.val = addr;
    pins->dq.word32 = 0;
    for (int lane = 0; lane < 4; lane++) {
        pins->dqs[lane] = 0;
        pins->dm[lane]  = 0;
    }

    if (event == CMD_NOP) {
        return; // already deselected/NOP above
    }

    // Select only the rank this command targets - RAS#/CAS#/WE# are a
    // shared bus, CS# is what makes the command rank-specific.
    if (rank == 0) pins->cs0_n = 0; else pins->cs1_n = 0;

    switch (event) {
        case CMD_ACT:
            pins->ras_n = 0; pins->cas_n = 1; pins->we_n = 1;
            break;
        case CMD_READ:
            pins->ras_n = 1; pins->cas_n = 0; pins->we_n = 1;
            break;
        case CMD_WRITE:
            pins->ras_n = 1; pins->cas_n = 0; pins->we_n = 0;
            break;
        case CMD_PRE:
            pins->ras_n = 0; pins->cas_n = 1; pins->we_n = 0;
            break;
        case CMD_REF:
            pins->ras_n = 0; pins->cas_n = 0; pins->we_n = 0;
            break;
        default:
            pins->cs0_n = 1; pins->cs1_n = 1;
            pins->ras_n = 1; pins->cas_n = 1; pins->we_n = 1;
            break;
    }
}

/* dump field into stdout */
void ctrl_trace_pins(unsigned long long cycle, const struct phy_pins_t *pins) {
    printf("CYCLE,%llu,"
           "%u,%u,%u,%u,"                      /* 1-> ck0, ck1, cke, reset_n            */
           "%u,%u,%u,%u,%u,%u,"                /* 5-> cs0_n, cs1_n, ras_n, cas_n, we_n, odt */
           "%u,%u,%u,"                         /* 11-> ba0, ba1, ba2                     */
           "%u,%u,%u,%u,%u,%u,%u,%u,"          /* 14-> a0..a7                            */
           "%u,%u,%u,%u,%u,%u,%u,%u,"          /* 22-> a8..a15                           */
           "%u,%u,%u,%u,%u,%u,%u,%u,"          /* 30-> dq byte0 b0..b7                   */
           "%u,%u,%u,%u,%u,%u,%u,%u,"          /* 38-> dq byte1 b0..b7                   */
           "%u,%u,%u,%u,%u,%u,%u,%u,"          /* 46-> dq byte2 b0..b7                   */
           "%u,%u,%u,%u,%u,%u,%u,%u,"          /* 54-> dq byte3 b0..b7                   */
           "%u,%u,%u,%u,"                      /* 62-> dqs0, dqs1, dqs2, dqs3             */
           "%u,%u,%u,%u\n",                    /* 66-> dm0, dm1, dm2, dm3                 */
           cycle,
           pins->ck0, pins->ck1, pins->cke, pins->reset_n,
           pins->cs0_n, pins->cs1_n, pins->ras_n, pins->cas_n, pins->we_n, pins->odt,
           pins->ba.ba0, pins->ba.ba1, pins->ba.ba2,
           pins->addr.a0,  pins->addr.a1,  pins->addr.a2,  pins->addr.a3,
           pins->addr.a4,  pins->addr.a5,  pins->addr.a6,  pins->addr.a7,
           pins->addr.a8,  pins->addr.a9,  pins->addr.a10, pins->addr.a11,
           pins->addr.a12, pins->addr.a13, pins->addr.a14, pins->addr.a15,
           pins->dq.bytes[0].b0, pins->dq.bytes[0].b1, pins->dq.bytes[0].b2, pins->dq.bytes[0].b3,
           pins->dq.bytes[0].b4, pins->dq.bytes[0].b5, pins->dq.bytes[0].b6, pins->dq.bytes[0].b7,
           pins->dq.bytes[1].b0, pins->dq.bytes[1].b1, pins->dq.bytes[1].b2, pins->dq.bytes[1].b3,
           pins->dq.bytes[1].b4, pins->dq.bytes[1].b5, pins->dq.bytes[1].b6, pins->dq.bytes[1].b7,
           pins->dq.bytes[2].b0, pins->dq.bytes[2].b1, pins->dq.bytes[2].b2, pins->dq.bytes[2].b3,
           pins->dq.bytes[2].b4, pins->dq.bytes[2].b5, pins->dq.bytes[2].b6, pins->dq.bytes[2].b7,
           pins->dq.bytes[3].b0, pins->dq.bytes[3].b1, pins->dq.bytes[3].b2, pins->dq.bytes[3].b3,
           pins->dq.bytes[3].b4, pins->dq.bytes[3].b5, pins->dq.bytes[3].b6, pins->dq.bytes[3].b7,
           pins->dqs[0], pins->dqs[1], pins->dqs[2], pins->dqs[3],
           pins->dm[0], pins->dm[1], pins->dm[2], pins->dm[3]);
    fflush(stdout);
}

// Step clock half-cycle and dump trace.
// This is the ONLY function in the entire codebase allowed to mutate
// ck0/ck1. Both are toggled (never force-assigned), guaranteeing strict
// alternation every call regardless of how many half-cycles preceded
// it - this is what eliminates the "two consecutive HIGH samples"
// glitch that force-assignment used to cause at odd/even tCL-tCWL
// boundaries.
static void ctrl_toggle_clock(unsigned long long *cycle, struct phy_pins_t *pins) {
    (*cycle)++;
    pins->ck0 = !pins->ck0;
    pins->ck1 = !pins->ck1;   // independent toggle - NOT !ck0 (ck1 is not ck0's complement)
    ctrl_trace_pins(*cycle, pins);
}

// Rank selection + command application. CS# gating happens HERE (ctrl
// owns ranks), chip.c never looks at cs0_n/cs1_n at all. Also owns all
// rank-aware transition logging, since only ctrl.c has the rank index.
static void ctrl_dispatch_to_rank(struct ctrl_t *ctrl, unsigned rank, unsigned bank, const struct phy_pins_t *pins) {
    if (rank >= MAX_RANKS || !ctrl->rank[rank].enabled) return;

    unsigned cs_n = (rank == 0) ? pins->cs0_n : pins->cs1_n;
    if (cs_n == 1) return; // this rank deselected, ignore

    struct bank_t *bank_ptr = &ctrl->rank[rank].chip.banks[bank];
    enum state_t prev = bank_ptr->state;
    chip_apply_command(&ctrl->rank[rank].chip, bank, pins);
    if (prev != bank_ptr->state) {
        printf("[Transition](Rank %u, Bank %u) " RED "%s" RST " -> " GRN "%s" RST "\n",
               rank, bank, state_to_str(prev), state_to_str(bank_ptr->state));
    }
}

// Controller originates WRITE data. One real, independent 32-bit word
// per beat (fixes the old "pattern[beat]+1" fake-lane1 hack), with a
// TRUE per-beat, per-lane mask array.
static void ctrl_drive_write_burst(unsigned long long *cycle, struct phy_pins_t *pins,
                                    const unsigned int *burst_words, unsigned length,
                                    const unsigned char dm[][4]) {
    for (unsigned beat = 0; beat < length; beat++) {
        // --- Rising Strobe Edge ---
        for (int lane = 0; lane < 4; lane++) pins->dqs[lane] = 1;
        pins->dq.word32 = burst_words[beat];
        for (int lane = 0; lane < 4; lane++) pins->dm[lane] = dm[beat][lane];
        ctrl_toggle_clock(cycle, pins);

        // --- Falling Strobe Edge ---
        for (int lane = 0; lane < 4; lane++) pins->dqs[lane] = 0;
        ctrl_toggle_clock(cycle, pins);
    }
}

// Controller clocks the bus during a READ; the DEVICE (chip) side
// originates the actual DQ payload via chip_drive_read_beat() - the
// controller must never invent READ data itself.
static void ctrl_drive_read_burst(unsigned long long *cycle, struct phy_pins_t *pins, unsigned length) {
    for (unsigned beat = 0; beat < length; beat++) {
        // --- Rising Strobe Edge ---
        for (int lane = 0; lane < 4; lane++) pins->dqs[lane] = 1;

        chip_drive_read_beat(pins, beat);
        ctrl_toggle_clock(cycle, pins);

        // --- Falling Strobe Edge ---
        for (int lane = 0; lane < 4; lane++) pins->dqs[lane] = 0;
        ctrl_toggle_clock(cycle, pins);
    }
}

// ---------------------------------------------------------------------
// Public command API
// ---------------------------------------------------------------------

void ddr_init_controller(struct ctrl_t *ctrl, unsigned num_ranks) {
    if (num_ranks > MAX_RANKS) num_ranks = MAX_RANKS;
    ctrl->num_ranks = num_ranks;

    // JEDEC DDR3-1333 (9-9-9) Timing Specifications
    ctrl->timing.tRCD = 9;
    ctrl->timing.tRP = 9;
    ctrl->timing.tCL = 9;
    ctrl->timing.tCWL = 7;
    ctrl->timing.tWR = 10;
    ctrl->timing.tRFC = 107;
    ctrl->timing.burst_length = 8;

    ctrl->timing.tCK = 1.5;
    ctrl->timing.tDQSCK = 0.255;
    ctrl->timing.tQSH = 0.40 * ctrl->timing.tCK;
    ctrl->timing.tQSL = 0.40 * ctrl->timing.tCK;
    ctrl->timing.tBURST = (ctrl->timing.burst_length / 2.0) * ctrl->timing.tCK;

    for (unsigned r = 0; r < MAX_RANKS; r++) {
        ctrl->rank[r].enabled = (r < num_ranks);
        if (ctrl->rank[r].enabled) {
            chip_init(&ctrl->rank[r].chip);
        }
        for (int b = 0; b < NUM_BANKS; b++) {
            ctrl->bank_timing[r][b].last_act_cycle = 0;
            ctrl->bank_timing[r][b].last_pre_cycle = 0;
        }
    }
    ctrl->last_ref_cycle = 0;
    ctrl->current_cycle = 0;
}

void ctrl_boot_sequence(struct ctrl_t *ctrl, struct phy_pins_t *pins) {
    (void)pins;
    for (unsigned r = 0; r < ctrl->num_ranks; r++) {
        if (!ctrl->rank[r].enabled) continue;
        struct chip_t *chip = &ctrl->rank[r].chip;
        for (int b = 0; b < NUM_BANKS; b++) {
            enum state_t p1 = chip->banks[b].state;
            chip->banks[b].state = process_event(p1, POWER_ON);
            printf("[Transition](Rank %u, Bank %u) " RED "%s" RST " -> " GRN "%s" RST "\n",
                   r, b, state_to_str(p1), state_to_str(chip->banks[b].state));

            enum state_t p2 = chip->banks[b].state;
            chip->banks[b].state = process_event(p2, INIT_DONE);
            printf("[Transition](Rank %u, Bank %u) " RED "%s" RST " -> " GRN "%s" RST "\n",
                   r, b, state_to_str(p2), state_to_str(chip->banks[b].state));
        }
    }
}

void ctrl_handle_status(const struct ctrl_t *ctrl) {
    for (unsigned r = 0; r < MAX_RANKS; r++) {
        if (!ctrl->rank[r].enabled) continue;
        printf("Rank %u:\n", r);
        chip_print_banks(&ctrl->rank[r].chip);
    }
}

void ctrl_handle_refresh(struct ctrl_t *ctrl, struct phy_pins_t *pins, unsigned rank) {
    if (rank >= MAX_RANKS || !ctrl->rank[rank].enabled) {
        printf("Error: Invalid or unpopulated rank %u\n", rank);
        return;
    }
    if (!chip_all_banks_idle(&ctrl->rank[rank].chip)) {
        printf("[Protocol Error] " YEL "Cannot issue refresh. All banks in rank %u must be IDLE." RST "\n", rank);
        return;
    }
    for (int b = 0; b < NUM_BANKS; b++) {
        // 1. Command Phase (REF)
        ctrl_drive_command(pins, CMD_REF, rank, b, 0);
        // Rising edge: toggle only, command sampled on this edge
        ctrl_toggle_clock(&ctrl->current_cycle, pins);
        ctrl_dispatch_to_rank(ctrl, rank, b, pins);
        // Falling edge
        ctrl_toggle_clock(&ctrl->current_cycle, pins);

        // 2. Internal Completion Phase (REF_DONE)
        struct bank_t *bank_ptr = &ctrl->rank[rank].chip.banks[b];
        enum state_t prev = bank_ptr->state;
        bank_ptr->state = process_event(prev, REF_DONE);
        if (prev != bank_ptr->state) {
            printf("[Transition](Rank %u, Bank %u) " RED "%s" RST " -> " GRN "%s" RST "\n",
                   rank, b, state_to_str(prev), state_to_str(bank_ptr->state));
        }
    }
}

void ctrl_handle_activate(struct ctrl_t *ctrl, struct phy_pins_t *pins, unsigned rank, unsigned bank, unsigned short row) {
    if (rank >= MAX_RANKS || !ctrl->rank[rank].enabled) {
        printf("Error: Invalid or unpopulated rank %u\n", rank);
        return;
    }
    if (bank >= NUM_BANKS) {
        printf("Error: Invalid bank %u\n", bank);
        return;
    }
    struct bank_t *bank_ptr = &ctrl->rank[rank].chip.banks[bank];
    if (bank_ptr->state != IDLE) {
        printf("[Protocol Error] Cannot ACTIVATE Rank %u Bank %u from state " GRN "%s" RST " (Bank must be " YEL "IDLE" RST ")\n",
               rank, bank, state_to_str(bank_ptr->state));
        return;
    }
    bank_ptr->open_row = row;

    ctrl_drive_command(pins, CMD_ACT, rank, bank, row);
    pins->addr.val = row & 0x3FFF; // 14-bit Row address on A[0..13]

    // Rising edge
    ctrl_toggle_clock(&ctrl->current_cycle, pins);
    ctrl_dispatch_to_rank(ctrl, rank, bank, pins);
    // Falling edge
    ctrl_toggle_clock(&ctrl->current_cycle, pins);
}

void ctrl_handle_precharge(struct ctrl_t *ctrl, struct phy_pins_t *pins, unsigned rank, unsigned bank) {
    if (rank >= MAX_RANKS || !ctrl->rank[rank].enabled) {
        printf("Error: Invalid or unpopulated rank %u\n", rank);
        return;
    }
    if (bank >= NUM_BANKS) {
        printf("Error: Invalid bank %u\n", bank);
        return;
    }
    struct bank_t *bank_ptr = &ctrl->rank[rank].chip.banks[bank];
    if (bank_ptr->state != BANK_ACTIVE) {
        printf("[Protocol Error] Cannot PRECHARGE Rank %u Bank %u from state " GRN "%s" RST " (Bank must be " YEL "BANK_ACTIVE" RST ")\n",
               rank, bank, state_to_str(bank_ptr->state));
        return;
    }
    // 1. Command Phase (PRE)
    ctrl_drive_command(pins, CMD_PRE, rank, bank, 0);
    // Rising edge
    ctrl_toggle_clock(&ctrl->current_cycle, pins);
    ctrl_dispatch_to_rank(ctrl, rank, bank, pins);
    // Falling edge
    ctrl_toggle_clock(&ctrl->current_cycle, pins);

    // 2. Internal Completion Phase (PRE_DONE)
    enum state_t prev = bank_ptr->state;
    bank_ptr->state = process_event(prev, PRE_DONE);
    if (prev != bank_ptr->state) {
        printf("[Transition](Rank %u, Bank %u) " RED "%s" RST " -> " GRN "%s" RST "\n", rank, bank, state_to_str(prev), state_to_str(bank_ptr->state));
    }
}

void ctrl_handle_write(struct ctrl_t *ctrl, struct phy_pins_t *pins,
                        unsigned rank, unsigned bank, unsigned short col,
                        unsigned int data_word,
                        const unsigned char dm[DDR_MAX_BURST_LENGTH][4]) {
    if (rank >= MAX_RANKS || !ctrl->rank[rank].enabled) {
        printf("Error: Invalid or unpopulated rank %u\n", rank);
        return;
    }
    if (bank >= NUM_BANKS) {
        printf("Error: Invalid bank %u\n", bank);
        return;
    }
    if (ctrl->timing.burst_length > DDR_MAX_BURST_LENGTH) {
        printf("[Config Error] burst_length %u exceeds DDR_MAX_BURST_LENGTH %u\n",
               ctrl->timing.burst_length, DDR_MAX_BURST_LENGTH);
        return;
    }
    struct bank_t *bank_ptr = &ctrl->rank[rank].chip.banks[bank];
    if (bank_ptr->state != BANK_ACTIVE) {
        printf("[Protocol Error] Cannot WRITE Rank %u Bank %u from state " GRN "%s" RST " (Bank must be " YEL "BANK_ACTIVE" RST ")\n",
               rank, bank, state_to_str(bank_ptr->state));
        return;
    }

    // 1. Command Phase
    ctrl_drive_command(pins, CMD_WRITE, rank, bank, col);
    unsigned short col_bits = col & 0x03FF; // Column address on A[0..9]
    pins->addr.val = col_bits;

    // Rising edge
    ctrl_toggle_clock(&ctrl->current_cycle, pins);
    ctrl_dispatch_to_rank(ctrl, rank, bank, pins);
    // Falling edge
    ctrl_toggle_clock(&ctrl->current_cycle, pins);

    // 2. Latency Phase (Controller enforces tCWL)
    for (unsigned c = 0; c < ctrl->timing.tCWL; c++) {
        ctrl_drive_command(pins, CMD_NOP, rank, 0, 0);
        ctrl_toggle_clock(&ctrl->current_cycle, pins);
    }

    // 3. Data Phase - BL8 is mandatory (DDR3 8n-prefetch architecture),
    // incomming word32 is forced to beat 0, for testing, cz. taking 4 masks for 8 beats is impractical 
    unsigned int burst_words[DDR_MAX_BURST_LENGTH] = {0};
    burst_words[0] = data_word;
    ctrl_drive_write_burst(&ctrl->current_cycle, pins, burst_words, ctrl->timing.burst_length, dm);

    // 4. Internal Completion Phase (BURST_DONE)
    enum state_t prev = bank_ptr->state;
    bank_ptr->state = process_event(prev, BURST_DONE);
    if (prev != bank_ptr->state) {
        printf("[Transition](Rank %u, Bank %u) " RED "%s" RST " -> " GRN "%s" RST "\n", rank, bank, state_to_str(prev), state_to_str(bank_ptr->state));
    }
}

void ctrl_handle_read(struct ctrl_t *ctrl, struct phy_pins_t *pins,
                       unsigned rank, unsigned bank, unsigned short col) {
    if (rank >= MAX_RANKS || !ctrl->rank[rank].enabled) {
        printf("Error: Invalid or unpopulated rank %u\n", rank);
        return;
    }
    if (bank >= NUM_BANKS) {
        printf("Error: Invalid bank %u\n", bank);
        return;
    }
    struct bank_t *bank_ptr = &ctrl->rank[rank].chip.banks[bank];
    if (bank_ptr->state != BANK_ACTIVE) {
        printf("[Protocol Error] Cannot READ Rank %u Bank %u from state " GRN "%s" RST " (Bank must be " YEL "BANK_ACTIVE" RST ")\n",
               rank, bank, state_to_str(bank_ptr->state));
        return;
    }

    // 1. Command Phase
    ctrl_drive_command(pins, CMD_READ, rank, bank, col);
    unsigned short col_bits = col & 0x03FF;
    pins->addr.val = col_bits;

    // Rising edge
    ctrl_toggle_clock(&ctrl->current_cycle, pins);
    ctrl_dispatch_to_rank(ctrl, rank, bank, pins);
    // Falling edge
    ctrl_toggle_clock(&ctrl->current_cycle, pins);

    // 2. Latency Phase (Controller enforces tCL)
    for (unsigned c = 0; c < ctrl->timing.tCL; c++) {
        ctrl_drive_command(pins, CMD_NOP, rank, 0, 0);
        ctrl_toggle_clock(&ctrl->current_cycle, pins);
    }

    // 3. Data Phase - chip (device) side drives DQ. 
    // STUB: no row_buffer storage exists yet, so this will NOT reflect any prior WRITE.
    ctrl_drive_read_burst(&ctrl->current_cycle, pins, ctrl->timing.burst_length);

    // 4. Internal update Phase (BURST_DONE)
    enum state_t prev = bank_ptr->state;
    bank_ptr->state = process_event(prev, BURST_DONE);
    if (prev != bank_ptr->state) {
        printf("[Transition](Rank %u, Bank %u) " RED "%s" RST " -> " GRN "%s" RST "\n", rank, bank, state_to_str(prev), state_to_str(bank_ptr->state));
    }
}