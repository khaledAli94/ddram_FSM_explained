#include "chip.h"
#include "main.h"
#include <stdio.h>

void dram_chip_init(struct dram_chip_t *chip) {
    for (int i = 0; i < NUM_BANKS; i++) {
        chip->banks[i].state = RESET;
        chip->banks[i].open_row = 0;
    }
    chip->org = ORG_X8; // Default organization x8

    // Formula: density = banks × rows × columns × organization
    unsigned long long total_bits = (unsigned long long)NUM_BANKS * NUM_ROWS * NUM_COLS * chip->org;

    if (total_bits <= 128ULL * 1024 * 1024)      chip->dens = DENSITY_128M;
    else if (total_bits <= 512ULL * 1024 * 1024)  chip->dens = DENSITY_512M;
    else if (total_bits <= 1024ULL * 1024 * 1024) chip->dens = DENSITY_1G;
    else if (total_bits <= 2048ULL * 1024 * 1024) chip->dens = DENSITY_2G;
    else                                          chip->dens = DENSITY_4G;
}

void dram_chip_print_status(const struct dram_chip_t *chip) {
    for (int i = 0; i < NUM_BANKS; i++) {
        printf("Bank %u \033[32m%s\033[0m, Active Row: %u\n", 
               i, state_to_str(chip->banks[i].state), chip->banks[i].open_row);
    }
}

bool dram_chip_all_banks_idle(const struct dram_chip_t *chip) {
    for (int i = 0; i < NUM_BANKS; i++) {
        if (chip->banks[i].state != IDLE) return false;
    }
    return true;
}

void dram_chip_receive_bus(struct dram_chip_t *chip, unsigned bank_idx, const struct dram_bus_t *bus) {
    if (bus->cs_n == 1) return; // Ignore if deselected
    enum event_t ev = dram_bus_decode(bus);
    enum state_t prev = chip->banks[bank_idx].state;
    chip->banks[bank_idx].state = process_event(prev, ev);
    if (prev != chip->banks[bank_idx].state) {
        printf("[Transition](Bank %u) " RED "%s" RST " -> " GRN "%s" RST "\n", 
               bank_idx, state_to_str(prev), state_to_str(chip->banks[bank_idx].state));
    }
}
