#include "chip.h"

void dram_chip_init(struct dram_chip_t *chip) {
    for (int i = 0; i < NUM_BANKS; i++) {
        chip->banks[i].state = RESET;
        chip->banks[i].open_row = 0;
    }
}
