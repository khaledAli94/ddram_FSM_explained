#include "chip.h"

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
