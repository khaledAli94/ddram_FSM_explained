#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ctrl.h"
#include "fsm.h"

struct ddr_ctrl_t ddr3;

void usage() {
    printf("\n=== DDR3 Simulator ===\n"
           "Usage:\n"
           "  read <row> <col>\n"
           "  write <row> <col>\n"
           "  ref\n"
           "  stat\n"
           "======================\n");
}

int main(void) {
    usage();
    
    ddr3_init_controller(&ddr3);
    process_event(&ddr3, POWER_ON);
    process_event(&ddr3, INIT_DONE);

    char cmd[16];
    unsigned r, c;

    while (printf("ddr3> ") && scanf("%15s", cmd) == 1)
    {
        if (strcmp(cmd, "stat") == 0) {
            printf("State: %s, Active Row: %u\n", state_to_str(ddr3.current_state), ddr3.active_row);
        } else if (strcmp(cmd, "ref") == 0) {
            if (ddr3.current_state != IDLE) {
                printf("[Protocol Error] \033[33mCannot issue REF from state %s (Bank must be IDLE)\033[0m\n", state_to_str(ddr3.current_state));
            } else {
                process_event(&ddr3, CMD_REF);
                process_event(&ddr3, REF_DONE);
            }
        } else if (strcmp(cmd, "read") == 0 || strcmp(cmd, "write") == 0) {
            if (scanf("%u %u", &r, &c) != 2) continue;

            if (ddr3.current_state != IDLE && ddr3.current_state != BANK_ACTIVE) {
                printf("[Protocol Error] \033[33mController is busy (%s). Cannot issue %s\033[0m\n", state_to_str(ddr3.current_state), cmd);
                continue;
            }

            ddr3.active_row = r;
            process_event(&ddr3, CMD_ACT);
            process_event(&ddr3, strcmp(cmd, "read") == 0 ? CMD_READ : CMD_WRITE);
            process_event(&ddr3, BURST_DONE);
        } else {
            printf("Unknown command!\n");
        }
    }
    return 0;
}
