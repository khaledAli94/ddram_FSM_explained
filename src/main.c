#include <stdio.h>

struct ddr3_controller_t ddr3;

int main(int argc, char **argv) {


    ddr3_init_controller(&ddr3);

    /* Simulate sequence */
    process_event(&ddr3, POWER_ON);
    process_event(&ddr3, INIT_DONE);

    process_event(&ddr3, CMD_ACT);
    process_event(&ddr3, CMD_READ);
    process_event(&ddr3, BURST_DONE);
    process_event(&ddr3, CMD_PRE);

    process_event(&ddr3, CMD_REF);

    return 0;
}