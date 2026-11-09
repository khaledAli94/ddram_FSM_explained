#include "main.h"

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    usage();
    
    struct ddr_ctrl_t ctrl;
    struct dram_chip_t dram;
    struct dram_bus_t bus;

    ddr3_init_controller(&ctrl);
    dram_chip_init(&dram);
    ctrl_boot_sequence(&ctrl, &dram, &bus);

    char cmd[16];
    unsigned b, r, c;

    while (printf("ddr3> ") && scanf("%15s", cmd) == 1) {
        if (strcmp(cmd, "status") == 0) {
            ctrl_handle_status(&dram);
        } else if (strcmp(cmd, "refresh") == 0) {
            ctrl_handle_refresh(&ctrl, &dram, &bus);
        } else if (strcmp(cmd, "act") == 0) {
            if (scanf("%u %u", &b, &r) != 2 || b >= NUM_BANKS) {
                printf("Error: act requires <bank (0-%d)> <row>\n", NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            ctrl_handle_activate(&ctrl, &dram, &bus, b, (unsigned short)r);
        } else if (strcmp(cmd, "pre") == 0) {
            if (scanf("%u", &b) != 1 || b >= NUM_BANKS) {
                printf("Error: pre requires <bank (0-%d)>\n", NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            ctrl_handle_precharge(&ctrl, &dram, &bus, b);
        } else if (strcmp(cmd, "read") == 0 || strcmp(cmd, "write") == 0) {
            if (scanf("%u %u", &b, &c) != 2 || b >= NUM_BANKS) {
                printf("Error: %s requires <bank (0-%d)> <col>\n", cmd, NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            ctrl_handle_data_command(&ctrl, &dram, &bus, cmd, b, (unsigned short)c);
        } else {
            printf("Unknown command. Type act, read, write, pre, refresh, or status.\n");
            while (getchar() != '\n');
        }
    }
    return 0;
}
