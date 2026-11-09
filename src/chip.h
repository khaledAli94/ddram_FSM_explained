#ifndef __CHIP_H__
#define __CHIP_H__

#include "fsm.h"

#define NUM_BANKS 8
#define NUM_ROWS  16384 // e.g., 14-bit row address
#define NUM_COLS  1024  // e.g., 10-bit column address

// Represents an individual bank inside the DRAM chip
struct dram_bank_t {
    enum state_t state;     // IDLE, BANK_ACTIVE, READING, WRITING, etc.
    unsigned open_row;      // Currently active row in this bank (if BANK_ACTIVE)
};

// Represents the physical DRAM Chip IC
struct dram_chip_t {
    struct dram_bank_t banks[NUM_BANKS]; // All 8 independent banks
};

void dram_chip_init(struct dram_chip_t *chip);

#endif /* __CHIP_H__ */
