#ifndef __BUS_H__
#define __BUS_H__

#include "fsm.h"

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

struct dram_bus_t {
    /* Clock Lines (Clocks, Enable & Reset) */
    unsigned ck0;
    unsigned ck1;
    unsigned cke;     /* Clock Enable */
    unsigned reset_n; /* Hardware Reset */

    /* Control & Command Bus */
    unsigned cs_n;    /* Chip Select */
    unsigned ras_n;   /* Row Address Strobe */
    unsigned cas_n;   /* Column Address Strobe */
    unsigned we_n;    /* Write Enable */
    unsigned odt;     /* On-Die Termination */

    /* Bank & Address Bus */
    struct bank_addr_t ba;     /* Bank Address: BA[0..2] (3 bits) */
    struct addr_lines_t addr;  /* Address Lines: A[0..15] (16 bits) */

    /* 32-bit Data Bus (4 Byte Lanes: DQ[0..31]) as a struct */
    struct {
        unsigned int word32;         /* 32-bit word */
        struct byte_lane_t bytes[4]; /* 4 individual byte lanes */
    } dq;

    /* 4 Data Strobes (DQS[0..3]) */
    unsigned dqs0;
    unsigned dqs1;
    unsigned dqs2;
    unsigned dqs3;

    /* 4 Data Masks (DM[0..3]) */
    unsigned dm0;
    unsigned dm1;
    unsigned dm2;
    unsigned dm3;
};

// Decoder: Bus Pins -> FSM Event (JEDEC Truth Table)
enum event_t dram_bus_decode(const struct dram_bus_t *bus);

// Encoder: FSM Event -> Bus Pins
void dram_bus_encode(struct dram_bus_t *bus, enum event_t event, unsigned bank, unsigned short addr);

// Dump CSV trace for Pygame logic analyzer
void dram_bus_dump_trace(unsigned long long cycle, const struct dram_bus_t *bus);

// Step clock half-cycle and dump trace
void dram_bus_step_clock(unsigned long long *cycle, struct dram_bus_t *bus);

// Stream data burst across DQS strobes and byte lanes
void dram_bus_stream_burst(unsigned long long *cycle, struct dram_bus_t *bus, const unsigned char *pattern, unsigned length, unsigned char dm0, unsigned char dm1, unsigned char dm2, unsigned char dm3);

#endif /* __BUS_H__ */
