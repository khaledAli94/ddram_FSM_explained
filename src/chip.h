#ifndef __CHIP_H__
#define __CHIP_H__

#include <stdbool.h>
#include "fsm.h"
#include "bus.h"

#define NUM_BANKS 8
#define NUM_ROWS  16384 // 14-bit row address
#define NUM_COLS  512   // 9-bit column address

/*
 * density = banks × rows × columns × organization
 * page_sz = 2^ROW_BITS * ORG / 8
 * bank_bits = log2(banks)
 * row_bits  = log2(rows)
 * col_bits  = log2(columns)
 * Payload_sz[Bytes per Burst Length BL] = (Organization / 8) × Burst_Length
 * ============================================================================
 */

enum dram_org_t {
    ORG_X4  = 4,
    ORG_X8  = 8,
    ORG_X16 = 16,
    ORG_X32 = 32,
};

enum dram_density_t {
    DENSITY_128M,
    DENSITY_512M,
    DENSITY_1G,
    DENSITY_2G,
    DENSITY_4G
};

// Represents an individual bank inside the DRAM chip
struct dram_bank_t {
    enum state_t state;     // IDLE, BANK_ACTIVE, READING, WRITING, etc.
    unsigned open_row;      // Currently active row in this bank (if BANK_ACTIVE)
};

// Represents the physical DRAM Chip IC
struct dram_chip_t {
    struct dram_bank_t banks[NUM_BANKS];
    enum dram_density_t dens;
    enum dram_org_t org;
};

void dram_chip_init(struct dram_chip_t *chip);
void dram_chip_print_status(const struct dram_chip_t *chip);
bool dram_chip_all_banks_idle(const struct dram_chip_t *chip);
void dram_chip_receive_bus(struct dram_chip_t *chip, unsigned bank_idx, const struct dram_bus_t *bus);

#endif /* __CHIP_H__ */
