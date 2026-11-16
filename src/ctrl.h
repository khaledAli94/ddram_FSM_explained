#ifndef __CTRL_H__
#define __CTRL_H__

#include <stdbool.h>
#include "fsm.h"
#include "chip.h"

#define MAX_RANKS 2
#define DDR_MAX_BURST_LENGTH 8

// JEDEC Timing, Configuration, and AC Specifications for DDR3-1600 (11-11-11)
struct ddr_timing_t {
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

// Per-bank timing tracker in the controller (indexed [rank][bank])
struct bank_timing_t {
    unsigned long long last_act_cycle;
    unsigned long long last_pre_cycle;
};

struct rank_t {
    bool enabled;          /* Whether this rank is physically populated */
    struct chip_t chip;    /* The device definition + bank states for this rank */
};

// Memory Controller (The Master / Arbiter)
struct ctrl_t {
    unsigned long long current_cycle;

    // All timing, specs, and configuration packed into one struct
    struct ddr_timing_t timing;

    // Tracks when each bank (per rank) was last activated/precharged
    struct bank_timing_t bank_timing[MAX_RANKS][NUM_BANKS];

    // Global refresh timestamp
    unsigned long long last_ref_cycle;

    // Ranks populated on this controller's channel
    unsigned num_ranks;
    struct rank_t rank[MAX_RANKS];
};

void ddr_init_controller(struct ctrl_t *ctrl, unsigned num_ranks);

// Controller Command Ownership Handlers (rank-aware)
void ctrl_boot_sequence(struct ctrl_t *ctrl, struct phy_pins_t *pins);
void ctrl_trace_pins(unsigned long long cycle, const struct phy_pins_t *pins);

void ctrl_handle_status(const struct ctrl_t *ctrl);
void ctrl_handle_refresh(struct ctrl_t *ctrl, struct phy_pins_t *pins, unsigned rank);
void ctrl_handle_activate(struct ctrl_t *ctrl, struct phy_pins_t *pins, unsigned rank, unsigned bank, unsigned short row);
void ctrl_handle_precharge(struct ctrl_t *ctrl, struct phy_pins_t *pins, unsigned rank, unsigned bank);

// Single-word WRITE: data_word lands on beat0, dm[][4] gives TRUE per-beat,
// per-lane masking control over all DDR_MAX_BURST_LENGTH beats.
void ctrl_handle_write(struct ctrl_t *ctrl, struct phy_pins_t *pins,
                        unsigned rank, unsigned bank, unsigned short col,
                        unsigned int data_word,
                        const unsigned char dm[DDR_MAX_BURST_LENGTH][4]);

// READ (data phase is a STUB - see chip_drive_read_beat())
void ctrl_handle_read(struct ctrl_t *ctrl, struct phy_pins_t *pins,
                       unsigned rank, unsigned bank, unsigned short col);

#endif /* __CTRL_H__ */