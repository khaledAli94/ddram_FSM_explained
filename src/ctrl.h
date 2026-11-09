#ifndef __CTRL_H__
#define __CTRL_H__

#include <stdbool.h>
#include "fsm.h"

#define NUM_BANKS 8

// JEDEC Timing Parameters for DDR3-1600 (11-11-11)
struct ddr3_timing_t {
    unsigned tRCD;  // Row-to-Column Delay (11 cycles)
    unsigned tRP;   // Row Precharge Time (11 cycles)
    unsigned tCL;   // CAS Latency (11 cycles)
    unsigned tWR;   // Write Recovery Time (12 cycles)
    unsigned tRFC;  // Refresh Cycle Time (e.g., ~74 cycles)
};

// Per-bank timing tracker in the controller
struct bank_timing_t {
    unsigned long long last_act_cycle;
    unsigned long long last_pre_cycle;
};

// Memory Controller (The Master / Arbiter)
struct ddr_ctrl_t {
    unsigned long long current_cycle;
    struct ddr3_timing_t timing;

    // Tracks when each bank was last activated/precharged
    struct bank_timing_t banks[NUM_BANKS];

    // Global refresh timestamp
    unsigned long long last_ref_cycle;
};

void ddr3_init_controller(struct ddr_ctrl_t *ctrl);

#endif /* __CTRL_H__ */
