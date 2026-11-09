#ifndef __CTRL_H__
#define __CTRL_H__

#include <stdbool.h>
#include "fsm.h"
#include "bus.h"
#include "chip.h"

#define NUM_BANKS 8

// JEDEC Timing, Configuration, and AC Specifications for DDR3-1600 (11-11-11)
struct ddr3_timing_t {
    // Cycle-Count Timestamps & Latencies (Integer Clocks)
    unsigned tRCD;      // Row-to-Column Delay (11 cycles)
    unsigned tRP;       // Row Precharge Time (11 cycles)
    unsigned tCL;       // CAS Latency (11 cycles)
    unsigned tCWL;      // CAS Write Latency (8 cycles)
    unsigned tWR;       // Write Recovery Time (12 cycles)
    unsigned tRFC;      // Refresh Cycle Time (74 cycles)

    // Configuration Options
    unsigned burst_length; // e.g., 8 (BL8) or 4 (BL4)

    // AC Timing Specifications (Nanoseconds)
    double tCK;         // Clock cycle time (1.25 ns for DDR3-1600)
    double tDQSCK;      // DQS Output Access Time from CK (~0.45 ns)
    double tQSH;        // DQS High Pulse Width (min 0.45 * tCK)
    double tQSL;        // DQS Low Pulse Width (min 0.45 * tCK)
    double tBURST;      // Total Burst Duration ((BL / 2) * tCK)
};

// Per-bank timing tracker in the controller
struct bank_timing_t {
    unsigned long long last_act_cycle;
    unsigned long long last_pre_cycle;
};

// Memory Controller (The Master / Arbiter)
struct ddr_ctrl_t {
    unsigned long long current_cycle;
    
    // All timing, specs, and configuration packed into one struct
    struct ddr3_timing_t timing;

    // Tracks when each bank was last activated/precharged
    struct bank_timing_t banks[NUM_BANKS];

    // Global refresh timestamp
    unsigned long long last_ref_cycle;
};

void ddr3_init_controller(struct ddr_ctrl_t *ctrl);

// Controller Command Ownership Handlers
void ctrl_boot_sequence(struct ddr_ctrl_t *ctrl, struct dram_chip_t *dram, struct dram_bus_t *bus);
void ctrl_handle_status(const struct dram_chip_t *dram);
void ctrl_handle_refresh(struct ddr_ctrl_t *ctrl, struct dram_chip_t *dram, struct dram_bus_t *bus);
void ctrl_handle_activate(struct ddr_ctrl_t *ctrl, struct dram_chip_t *dram, struct dram_bus_t *bus, unsigned bank, unsigned short row);
void ctrl_handle_precharge(struct ddr_ctrl_t *ctrl, struct dram_chip_t *dram, struct dram_bus_t *bus, unsigned bank);
void ctrl_handle_data_command(struct ddr_ctrl_t *ctrl, struct dram_chip_t *dram, struct dram_bus_t *bus, const char *type, unsigned bank, unsigned short col);

#endif /* __CTRL_H__ */
