#include <fsm.h>

void process_event(struct ddr3_controller_t *ctrl, enum ddr3_event_t event){
    enum ddr3_state_t previous_state = ctrl->current_state;

    switch (ctrl->current_state) {

        case STATE_RESET:
            if (event == EVENT_POWER_ON) {
                /* Power-up sequence: CKE low, RESET# assertion, wait time */
                ctrl->current_state = STATE_INITIALIZATION;
            }
            break;

        case STATE_INITIALIZATION:
            if (event == EVENT_INIT_DONE) {
                /* Bring up CKE, send ZQ Calibration, load Mode Registers (MR0-MR3) */
                ctrl->current_state = STATE_IDLE;
            }
            break;

        case STATE_IDLE:
            if (event == EVENT_CMD_ACT) {
                /* Open a specific Bank and Row (tRCD delay starts here) */
                ctrl->current_state = STATE_BANK_ACTIVE;
            } else if (event == EVENT_CMD_REF) {
                /* Refresh all banks (tRFC delay applies) */
                ctrl->current_state = STATE_REFRESHING;
            }
            break;

        case STATE_BANK_ACTIVE:
            if (event == EVENT_CMD_READ) {
                /* Issue READ command (CAS Latency tCL applies) */
                ctrl->current_state = STATE_READING;
            } else if (event == EVENT_CMD_WRITE) {
                /* Issue WRITE command (Write Latency tCWL applies) */
                ctrl->current_state = STATE_WRITING;
            } else if (event == EVENT_CMD_PRE) {
                /* Close the active row (tRP delay applies) */
                ctrl->current_state = STATE_PRECHARGING;
            }
            break;

        case STATE_READING:
            if (event == EVENT_BURST_DONE) {
                /* Return to active bank after burst read completed */
                ctrl->current_state = STATE_BANK_ACTIVE;
            }
            break;

        case STATE_WRITING:
            if (event == EVENT_BURST_DONE) {
                /* Return to active bank after burst write completed (tWR delay applies) */
                ctrl->current_state = STATE_BANK_ACTIVE;
            }
            break;

        case STATE_PRECHARGING:
            /* Row deactivated, bank back to IDLE */
            ctrl->current_state = STATE_IDLE;
            break;

        case STATE_REFRESHING:
            /* Refresh cycle complete */
            ctrl->current_state = STATE_IDLE;
            break;

        default:
            printf("[DDR3 Error] Invalid state encountered!\n");
            break;
    }

    printf("[DDR3 Transition] Event: %d | State: %s -> %s\n", event, state_to_str(previous_state), state_to_str(ctrl->current_state));
}