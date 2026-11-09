#include "ctrl.h"

void ddr3_init_controller(struct ddr_ctrl_t *ctrl) {
    ctrl->current_cycle = 0;
    ctrl->timing.tRCD = 11;
    ctrl->timing.tRP  = 11;
    ctrl->timing.tCL  = 11;
    ctrl->timing.tWR  = 12;
    ctrl->timing.tRFC = 74;

    for (int i = 0; i < NUM_BANKS; i++) {
        ctrl->banks[i].last_act_cycle = 0;
        ctrl->banks[i].last_pre_cycle = 0;
    }
    ctrl->last_ref_cycle = 0;
}
