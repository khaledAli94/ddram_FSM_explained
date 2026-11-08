#ifndef __CTRL_H__
#define __CTRL_H__

#include <stdio.h>
#include "fsm.h"

/* DDR3 Controller Context */
struct ddr3_controller_t {
    enum state_t current_state;
    unsigned active_bank;
    unsigned active_row;
};

static inline void ddr3_init_controller(struct ddr3_controller_t *ctrl) {
    ctrl->current_state = RESET;
    ctrl->active_bank = 0;
    ctrl->active_row = 0;
    printf("[init] ctrl goes to state: %s\n", state_to_str(ctrl->current_state));
}

#endif /* __CTRL_H__ */