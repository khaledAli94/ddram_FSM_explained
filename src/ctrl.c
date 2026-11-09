#include "ctrl.h"

void ddr3_init_controller(struct ddr_ctrl_t *ctrl) {
    ctrl->current_cycle = 0;

    // 1. Initialize cycle-count latencies
    ctrl->timing.tRCD = 11;
    ctrl->timing.tRP  = 11;
    ctrl->timing.tCL  = 11;
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
