#include <stdio.h>
#include "fsm.h"
#include "ctrl.h"

void process_event(struct ddr_ctrl_t *ctrl, enum event_t event){
    enum state_t previous_state = ctrl->current_state;

    switch (ctrl->current_state) {

        case RESET:
            if (event == POWER_ON) {
                /* Power-up sequence: CKE low, RESET# assertion, wait time */
                ctrl->current_state = INITIALIZATION;
            }
            break;

        case INITIALIZATION:
            if (event == INIT_DONE) {
                /* Bring up CKE, send ZQ Calibration, load Mode Registers (MR0-MR3) */
                ctrl->current_state = IDLE;
            }
            break;

        case IDLE:
            if (event == CMD_ACT) {
                /* Open a specific Bank and Row (tRCD delay starts here) */
                ctrl->current_state = BANK_ACTIVE;
            } else if (event == CMD_REF) {
                /* Refresh all banks (tRFC delay applies) */
                ctrl->current_state = REFRESHING;
            }
            break;

        case BANK_ACTIVE:
            if (event == CMD_READ) {
                /* Issue READ command (CAS Latency tCL applies) */
                ctrl->current_state = READING;
            } else if (event == CMD_WRITE) {
                /* Issue WRITE command (Write Latency tCWL applies) */
                ctrl->current_state = WRITING;
            } else if (event == CMD_PRE) {
                /* Close the active row (tRP delay applies) */
                ctrl->current_state = PRECHARGING;
            }
            break;

        case READING:
            if (event == BURST_DONE) {
                /* Return to active bank after burst read completed */
                ctrl->current_state = BANK_ACTIVE;
            }
            break;

        case WRITING:
            if (event == BURST_DONE) {
                /* Return to active bank after burst write completed (tWR delay applies) */
                ctrl->current_state = BANK_ACTIVE;
            }
            break;

        case PRECHARGING:
            /* Row deactivated, bank back to IDLE */
            ctrl->current_state = IDLE;
            break;

        case REFRESHING:
            if (event == REF_DONE) {
                /* Refresh cycle complete */
                ctrl->current_state = IDLE;
            }
            break;

        default:
            printf("[Fatal] Invalid state encountered!\n");
            break;
    }

    printf("[Transition] \033[31m%s\033[0m -> \033[32m%s\033[0m\n", state_to_str(previous_state), state_to_str(ctrl->current_state));
}
