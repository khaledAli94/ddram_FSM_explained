#include <stdio.h>

int main(int argc, char **argv) {

    struct ddr3_controller_t ddr3;

    ddr3_init_controller(&ddr3);

    /* Simulate sequence */
    ddr3_fsm_process_event(&ddr3, POWER_ON);
    ddr3_fsm_process_event(&ddr3, INIT_DONE);

    ddr3_fsm_process_event(&ddr3, CMD_ACT);
    ddr3_fsm_process_event(&ddr3, CMD_READ);
    ddr3_fsm_process_event(&ddr3, BURST_DONE);
    ddr3_fsm_process_event(&ddr3, CMD_PRE);

    ddr3_fsm_process_event(&ddr3, CMD_REF);

    return 0;
}