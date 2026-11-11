#include <stdio.h>
#include "fsm.h"

enum state_t process_event(enum state_t current_state, enum event_t event) {
    if (event == CMD_NOP) {
        return current_state; // NOP does not change state
    }

    enum state_t next_state = current_state;

    switch (current_state) {

        case RESET:
            if (event == POWER_ON) {
                /* Power-up sequence: CKE low, RESET# assertion, wait time */
                next_state = INITIALIZATION;
            }
            break;

        case INITIALIZATION:
            if (event == INIT_DONE) {
                /* Bring up CKE, send ZQ Calibration, load Mode Registers (MR0-MR3) */
                next_state = IDLE;
            }
            break;

        case IDLE:
            if (event == CMD_ACT) {
                /* Open a specific Bank and Row (tRCD delay starts here) */
                next_state = BANK_ACTIVE;
            } else if (event == CMD_REF) {
                /* Refresh all banks (tRFC delay applies) */
                next_state = REFRESHING;
            }
            break;

        case BANK_ACTIVE:
            if (event == CMD_READ) {
                /* Issue READ command (CAS Latency tCL applies) */
                next_state = READING;
            } else if (event == CMD_WRITE) {
                /* Issue WRITE command (Write Latency tCWL applies) */
                next_state = WRITING;
            } else if (event == CMD_PRE) {
                /* Close the active row (tRP delay applies) */
                next_state = PRECHARGING;
            }
            break;

        case READING:
            if (event == BURST_DONE) {
                /* Return to active bank after burst read completed */
                next_state = BANK_ACTIVE;
            }
            break;

        case WRITING:
            if (event == BURST_DONE) {
                /* Return to active bank after burst write completed (tWR delay applies) */
                next_state = BANK_ACTIVE;
            }
            break;

        case PRECHARGING:
            if (event == PRE_DONE) {
                /* Row deactivated, bank back to IDLE */
                next_state = IDLE;
            }
            break;

        case REFRESHING:
            if (event == REF_DONE) {
                /* Refresh cycle complete */
                next_state = IDLE;
            }
            break;

        default:
            printf("[Fatal] Invalid state encountered!\n");
            break;
    }

    return next_state;
}
