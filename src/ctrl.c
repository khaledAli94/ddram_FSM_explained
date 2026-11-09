#include "ctrl.h"

void ddr3_init_controller(struct ddr_ctrl_t *ctrl) {
    ctrl->current_cycle = 0;

    // 1. Initialize cycle-count latencies
    ctrl->timing.tRCD = 11;
    ctrl->timing.tRP  = 11;
    ctrl->timing.tCL  = 11;
    ctrl->timing.tCWL = 8;
    ctrl->timing.tWR  = 12;
    ctrl->timing.tRFC = 74;

    // 2. Configure default Burst Length
    ctrl->timing.burst_length = 8; // BL8

    // 3. Initialize AC Timing specifications (DDR3-1600 @ 800MHz -> tCK = 1.25 ns)
    ctrl->timing.tCK = 1.25;
    ctrl->timing.tDQSCK = 0.45;
    ctrl->timing.tQSH = 0.45 * ctrl->timing.tCK;
    ctrl->timing.tQSL = 0.45 * ctrl->timing.tCK;
    ctrl->timing.tBURST = (ctrl->timing.burst_length / 2.0) * ctrl->timing.tCK;

    // 4. Initialize bank timestamps
    for (int i = 0; i < NUM_BANKS; i++) {
        ctrl->banks[i].last_act_cycle = 0;
        ctrl->banks[i].last_pre_cycle = 0;
    }
    ctrl->last_ref_cycle = 0;
}

void ctrl_encode_activate(struct dram_bus_t *bus, unsigned bank, unsigned short row) {
    dram_bus_encode(bus, CMD_ACT, bank, row);
    bus->addr.val = row & 0x3FFF; // 14-bit Row address on A[0..13]
}

void ctrl_encode_column_cmd(struct dram_bus_t *bus, enum event_t cmd, unsigned bank, unsigned short col, bool auto_precharge) {
    dram_bus_encode(bus, cmd, bank, col);
    unsigned short col_bits = col & 0x03FF;                  // Column address on A[0..9]
    unsigned short ap_bit   = (auto_precharge ? 1 : 0) << 10;// Auto-Precharge on A10
    bus->addr.val = col_bits | ap_bit;
}

void ctrl_wait_cycles(unsigned long long *cycle, struct dram_bus_t *bus, unsigned count) {
    for (unsigned c = 0; c < count; c++) {
        (*cycle)++;
        dram_bus_encode(bus, CMD_NOP, 0, 0);
        dram_bus_dump_trace(*cycle, bus);
    }
}
