#ifndef __CHIP_H__
#define __CHIP_H__

#include <stdbool.h>
#include "fsm.h"
#include "bus.h"

#define NUM_BANKS 8
#define NUM_ROWS  16384 // 14-bit row address
#define NUM_COLS  512   // 9-bit column address

/*
 * ============================================================================
 * JEDEC DRAM FORMULAS REFERENCE:
 * ============================================================================
 * 1. Total Density (in bits):
 *    Density = Banks × Rows × Columns × Organization (Data Width in bits)
 *
 * 2. Total Capacity (in Megabytes - MB):
 *    Capacity (MB) = (Banks × Rows × Columns × Organization) / 8 / 1,048,576
 *
 * 3. Address Lines to Geometry Mapping:
 *    Rows    = 2 ^ (Number of Row Address Pins)
 *    Columns = 2 ^ (Number of Column Address Pins)
 *
 * 4. Burst Transfer Payload Size (Bytes per Burst Length BL):
 *    Payload = (Organization / 8) × Burst_Length
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
