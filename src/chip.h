#ifndef __CHIP_H__
#define __CHIP_H__

#include <stdbool.h>
#include "fsm.h"

#define NUM_BANKS 8
#define NUM_ROWS  16384 // 14-bit row address
// #define NUM_COLS  512   // 9-bit column address
#define NUM_COLS  1024   // 10-bit column address

/*
 * density = banks × rows × columns × organization
 * page_sz = 2^ROW_BITS * ORG / 8
 * bank_bits = log2(banks)
 * row_bits  = log2(rows)
 * col_bits  = log2(columns)
 * Payload_sz[Bytes per Burst Length BL] = (Organization / 8) × Burst_Length
 */

struct byte_lane_t {
    union {
        unsigned char byte;
        struct {
            unsigned char b0 : 1;
            unsigned char b1 : 1;
            unsigned char b2 : 1;
            unsigned char b3 : 1;
            unsigned char b4 : 1;
            unsigned char b5 : 1;
            unsigned char b6 : 1;
            unsigned char b7 : 1;
        };
    };
};

struct bank_addr_t {
    union {
        unsigned char val;
        struct {
            unsigned char ba0 : 1;
            unsigned char ba1 : 1;
            unsigned char ba2 : 1;
        };
    };
};

struct addr_lines_t {
    union {
        unsigned short val;
        struct {
            unsigned short a0  : 1;
            unsigned short a1  : 1;
            unsigned short a2  : 1;
            unsigned short a3  : 1;
            unsigned short a4  : 1;
            unsigned short a5  : 1;
            unsigned short a6  : 1;
            unsigned short a7  : 1;
            unsigned short a8  : 1;
            unsigned short a9  : 1;
            unsigned short a10 : 1;
            unsigned short a11 : 1;
            unsigned short a12 : 1;
            unsigned short a13 : 1;
            unsigned short a14 : 1;
            unsigned short a15 : 1;
        };
    };
};

/*
 * Physical pins of the DDRAM interface.
 */
struct phy_pins_t {
    /* Clock Lines (Clocks, Enable & Reset) */
    unsigned ck0;      /* Rank0 clock (true signal) */
    unsigned ck1;      /* Rank1 clock (true signal) */
    unsigned cke;      /* Clock Enable */
    unsigned reset_n;  /* Hardware Reset */

    /* Control & Command Bus */
    unsigned cs0_n;   /* Chip Select - Rank0 */
    unsigned cs1_n;   /* Chip Select - Rank1 */
    unsigned ras_n;   /* Row Address Strobe */
    unsigned cas_n;   /* Column Address Strobe */
    unsigned we_n;    /* Write Enable */
    unsigned odt;     /* On-Die Termination */

    /* Bank & Address Bus */
    struct bank_addr_t ba;     /* Bank Address: BA[0..2] */
    struct addr_lines_t addr;  /* Address Lines: A[0..15] */

    /* 32-bit Data Bus (4 Byte Lanes: DQ[0..31]) */
  struct {
        union {
            unsigned int word32;          /* 32-bit word */
            struct byte_lane_t bytes[4];  /* 4 individual byte lanes */
        };
    } dq;

    /* 4 Data Strobes (DQS[0..3]), per byte lane */
    unsigned dqs[4];

    /* 4 Data Masks (DM[0..3]), per byte lane */
    unsigned dm[4];
};

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

// statful struct of the bank
struct bank_t {
    enum state_t state;     // IDLE, BANK_ACTIVE, READING, WRITING, etc.
    unsigned open_row;      // (if BANK_ACTIVE) currently in sense amplifier
    // NOTE: no backing row_buffer/storage array yet - READ data is a
};

// Represents a single physical DRAM chip's definition: geometry + bank
struct chip_t {
    enum dram_density_t dens;
    enum dram_org_t org;
    struct bank_t banks[NUM_BANKS];
};

void chip_init(struct chip_t *chip);
bool chip_all_banks_idle(const struct chip_t *chip);
void chip_print_banks(const struct chip_t *chip);


void chip_apply_command(struct chip_t *chip, unsigned bank_idx, const struct phy_pins_t *pins);
void chip_drive_read_beat(struct phy_pins_t *pins, unsigned beat);

#endif /* __CHIP_H__ */