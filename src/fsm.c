#include <stdio.h>
#include "fsm.h"
#include "chip.h"

void process_event(struct dram_bank_t *bank, unsigned bank_idx, enum event_t event){
    if (event == CMD_NOP) {
        return; // NOP does not change state or trigger transitions
    }

    enum state_t previous_state = bank->state;

    switch (bank->state) {

        case RESET:
            if (event == POWER_ON) {
                /* Power-up sequence: CKE low, RESET# assertion, wait time */
                bank->state = INITIALIZATION;
            }
            break;

        case INITIALIZATION:
            if (event == INIT_DONE) {
                /* Bring up CKE, send ZQ Calibration, load Mode Registers (MR0-MR3) */
                bank->state = IDLE;
            }
            break;

        case IDLE:
            if (event == CMD_ACT) {
                /* Open a specific Bank and Row (tRCD delay starts here) */
                bank->state = BANK_ACTIVE;
            } else if (event == CMD_REF) {
                /* Refresh all banks (tRFC delay applies) */
                bank->state = REFRESHING;
            }
            break;

        case BANK_ACTIVE:
            if (event == CMD_READ) {
                /* Issue READ command (CAS Latency tCL applies) */
                bank->state = READING;
            } else if (event == CMD_WRITE) {
                /* Issue WRITE command (Write Latency tCWL applies) */
                bank->state = WRITING;
            } else if (event == CMD_PRE) {
                /* Close the active row (tRP delay applies) */
                bank->state = PRECHARGING;
            }
            break;

        case READING:
            if (event == BURST_DONE) {
                /* Return to active bank after burst read completed */
                bank->state = BANK_ACTIVE;
            }
            break;

        case WRITING:
            if (event == BURST_DONE) {
                /* Return to active bank after burst write completed (tWR delay applies) */
                bank->state = BANK_ACTIVE;
            }
            break;

        case PRECHARGING:
            if (event == PRE_DONE) {
                /* Row deactivated, bank back to IDLE */
                bank->state = IDLE;
            }
            break;

        case REFRESHING:
            if (event == REF_DONE) {
                /* Refresh cycle complete */
                bank->state = IDLE;
            }
            break;

        default:
            printf("[Fatal] Invalid state encountered!\n");
            break;
    }

    printf("[Transition](Bank %u) \033[31m%s\033[0m -> \033[32m%s\033[0m\n", bank_idx, state_to_str(previous_state), state_to_str(bank->state));
}
