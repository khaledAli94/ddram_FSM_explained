#include <stdio.h>
#include "main.h"
#include "chip.h"

void chip_init(struct chip_t *chip) {
    for (int b = 0; b < NUM_BANKS; b++) {
        chip->banks[b].state = RESET;
        chip->banks[b].open_row = 0;
    }
    chip->org = ORG_X16; // Default x16

    // density = banks × rows × columns × organization
    unsigned long long total_bits = (unsigned long long)NUM_BANKS * NUM_ROWS * NUM_COLS * chip->org;

    if (total_bits <= 128ULL * 1024 * 1024)       chip->dens = DENSITY_128M;
    else if (total_bits <= 512ULL * 1024 * 1024)  chip->dens = DENSITY_512M;
    else if (total_bits <= 1024ULL * 1024 * 1024) chip->dens = DENSITY_1G;
    else if (total_bits <= 2048ULL * 1024 * 1024) chip->dens = DENSITY_2G;
    else                                          chip->dens = DENSITY_4G;
}

void chip_print_banks(const struct chip_t *chip) {
    // ranks' ownership belongs to ctl not chips
    for (int b = 0; b < NUM_BANKS; b++) {
        printf("  Bank %u "GRN "%s" RST ", Active Row: %u\n", b, state_to_str(chip->banks[b].state), chip->banks[b].open_row);
    }
}

bool chip_all_banks_idle(const struct chip_t *chip) {
    for (int b = 0; b < NUM_BANKS; b++) {
        if (chip->banks[b].state != IDLE) return false;
    }
    return true;
}

// chip-side command truth table decode (JEDEC)
static enum event_t chip_decode_command(const struct phy_pins_t *pins) {
    // JEDEC Command Truth Table Decoding (sampled on rising edge of CK)
    if (pins->ras_n == 0 && pins->cas_n == 1 && pins->we_n == 1) return CMD_ACT;
    if (pins->ras_n == 1 && pins->cas_n == 0 && pins->we_n == 1) return CMD_READ;
    if (pins->ras_n == 1 && pins->cas_n == 0 && pins->we_n == 0) return CMD_WRITE;
    if (pins->ras_n == 0 && pins->cas_n == 1 && pins->we_n == 0) return CMD_PRE;
    if (pins->ras_n == 0 && pins->cas_n == 0 && pins->we_n == 0) return CMD_REF;

    return CMD_NOP;
}

void chip_apply_command(struct chip_t *chip, unsigned bank_idx, const struct phy_pins_t *pins) {
    if (bank_idx >= NUM_BANKS) return;
    enum event_t ev = chip_decode_command(pins);
    chip->banks[bank_idx].state = process_event(chip->banks[bank_idx].state, ev);
}

void chip_drive_read_beat(struct phy_pins_t *pins, unsigned beat) {
    /* STUB: mem cells have not been yet implmented for storage/retrive */
    // 8n prefetch applies cz. cells are relatively slow while IO pins faster, thus we pull that burst due to time cost.
    //A legal DDR controller will not issue a burst that extends beyond the row boundary it must compute and check this boundary first.
    // swapped in lane perspective 

    static const unsigned stub_pattern[DDR_MAX_BURST_LENGTH] = {
    0xDEADBEEFu, 0xFEEDFACEu, 0xC0DECAFEu, 0xA5A5A5A5u,
    0xBABEFACEu, 0xBFBFBFBFu, 0xA0A0A0A0u, 0x5A5A5A5Au
    };

    pins->dq.word32 = stub_pattern[beat];
}