#include "ctrl.h"
#include <stdio.h>
#include <string.h>

void ddr3_init_controller(struct ddr_ctrl_t *ctrl) {
    ctrl->current_cycle = 0;

    // 1. Initialize cycle-count latencies
    ctrl->timing.tRCD = 11;
    ctrl->timing.tRP  = 11;
    ctrl->timing.tCL  = 11;
    ctrl->timing.tCWL = 8;
    ctrl->timing.tWR  = 12;
    ctrl->timing.tRFC = 74;

    // 2. Configure default Burst Length
    ctrl->timing.burst_length = 8; // BL8

    // 3. Initialize AC Timing specifications (DDR3-1600 @ 800MHz -> tCK = 1.25 ns)
    ctrl->timing.tCK = 1.25;
    ctrl->timing.tDQSCK = 0.45;
    ctrl->timing.tQSH = 0.45 * ctrl->timing.tCK;
    ctrl->timing.tQSL = 0.45 * ctrl->timing.tCK;
    ctrl->timing.tBURST = (ctrl->timing.burst_length / 2.0) * ctrl->timing.tCK;

    // 4. Initialize bank timestamps
    for (int i = 0; i < NUM_BANKS; i++) {
        ctrl->banks[i].last_act_cycle = 0;
        ctrl->banks[i].last_pre_cycle = 0;
    }
    ctrl->last_ref_cycle = 0;
}

void ctrl_boot_sequence(struct ddr_ctrl_t *ctrl, struct dram_chip_t *dram, struct dram_bus_t *bus) {
    (void)ctrl;
    (void)bus;
    for (int i = 0; i < NUM_BANKS; i++) {
        process_event(&dram->banks[i], i, POWER_ON);
        process_event(&dram->banks[i], i, INIT_DONE);
    }
}

void ctrl_handle_status(const struct dram_chip_t *dram) {
    dram_chip_print_status(dram);
}

void ctrl_handle_refresh(struct ddr_ctrl_t *ctrl, struct dram_chip_t *dram, struct dram_bus_t *bus) {
    if (!dram_chip_all_banks_idle(dram)) {
        printf("[Protocol Error] " YEL "Cannot issue refresh. All banks must be IDLE." RST "\n");
        return;
    }
    for (int i = 0; i < NUM_BANKS; i++) {
        // 1. Command Phase (REF)
        dram_bus_encode(bus, CMD_REF, i, 0);
        dram_bus_dump_trace(++ctrl->current_cycle, bus);
        dram_chip_receive_bus(dram, i, bus);

        // 2. Internal Completion Phase (REF_DONE)
        process_event(&dram->banks[i], i, REF_DONE);
    }
}

void ctrl_handle_activate(struct ddr_ctrl_t *ctrl, struct dram_chip_t *dram, struct dram_bus_t *bus, unsigned bank, unsigned short row) {
    if (bank >= NUM_BANKS) {
        printf("Error: Invalid bank %u\n", bank);
        return;
    }
    struct dram_bank_t *bank_ptr = &dram->banks[bank];
    if (bank_ptr->state != IDLE) {
        printf("[Protocol Error] Cannot ACTIVATE Bank %u from state " GRN "%s" RST " (Bank must be " YEL "IDLE" RST ")\n",
               bank, state_to_str(bank_ptr->state));
        return;
    }
    bank_ptr->open_row = row;

    dram_bus_encode(bus, CMD_ACT, bank, row);
    bus->addr.val = row & 0x3FFF; // 14-bit Row address on A[0..13]
    dram_bus_dump_trace(++ctrl->current_cycle, bus);
    dram_chip_receive_bus(dram, bank, bus);
}

void ctrl_handle_precharge(struct ddr_ctrl_t *ctrl, struct dram_chip_t *dram, struct dram_bus_t *bus, unsigned bank) {
    if (bank >= NUM_BANKS) {
        printf("Error: Invalid bank %u\n", bank);
        return;
    }
    struct dram_bank_t *bank_ptr = &dram->banks[bank];
    if (bank_ptr->state != BANK_ACTIVE) {
        printf("[Protocol Error] Cannot PRECHARGE Bank %u from state " GRN "%s" RST " (Bank must be " YEL "BANK_ACTIVE" RST ")\n",
               bank, state_to_str(bank_ptr->state));
        return;
    }
    // 1. Command Phase (PRE)
    dram_bus_encode(bus, CMD_PRE, bank, 0);
    dram_bus_dump_trace(++ctrl->current_cycle, bus);
    dram_chip_receive_bus(dram, bank, bus);

    // 2. Internal Completion Phase (PRE_DONE)
    process_event(bank_ptr, bank, PRE_DONE);
}

void ctrl_handle_data_command(struct ddr_ctrl_t *ctrl, struct dram_chip_t *dram, struct dram_bus_t *bus, const char *type, unsigned bank, unsigned short col) {
    if (bank >= NUM_BANKS) {
        printf("Error: Invalid bank %u\n", bank);
        return;
    }
    struct dram_bank_t *bank_ptr = &dram->banks[bank];
    if (bank_ptr->state != BANK_ACTIVE) {
        printf("[Protocol Error] Cannot %s Bank %u from state " GRN "%s" RST " (Bank must be " YEL "BANK_ACTIVE" RST ")\n",
               type, bank, state_to_str(bank_ptr->state));
        return;
    }

    enum event_t cmd_ev = (strcmp(type, "read") == 0) ? CMD_READ : CMD_WRITE;
    
    // 1. Command Phase
    dram_bus_encode(bus, cmd_ev, bank, col);
    unsigned short col_bits = col & 0x03FF;                  // Column address on A[0..9]
    bus->addr.val = col_bits;
    dram_bus_dump_trace(++ctrl->current_cycle, bus);
    dram_chip_receive_bus(dram, bank, bus);

    // 2. Latency Phase (Controller enforces tCL / tCWL)
    unsigned latency = (cmd_ev == CMD_READ) ? ctrl->timing.tCL : ctrl->timing.tCWL;
    for (unsigned c = 0; c < latency; c++) {
        ctrl->current_cycle++;
        dram_bus_encode(bus, CMD_NOP, 0, 0);
        dram_bus_dump_trace(ctrl->current_cycle, bus);
    }

    // 3. Data Phase (Strobe streaming)
    unsigned char pattern[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    dram_bus_stream_burst(&ctrl->current_cycle, bus, pattern, ctrl->timing.burst_length);

    // 4. Internal Completion Phase (BURST_DONE)
    process_event(bank_ptr, bank, BURST_DONE);
}
