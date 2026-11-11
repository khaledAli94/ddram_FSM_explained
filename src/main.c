#include "main.h"

void usage(void) {
    printf("\n=== JEDEC DDR3 Simulator ===\n"
           "  Usage:\n"
           "  act <bank> <row>     - ACTIVATE command (Opens Row)\n"
           "  read <bank> <col>    - READ command (Read Burst + Strobes)\n"
           "  write <bank> <col>   - WRITE command (Write Burst + Strobes)\n"
           "  mwrite <bank> <col> <dm0> <dm1> - Masked WRITE command (Mask Bytes)\n"
           "  pre <bank>           - PRECHARGE command (Closes Row)\n"
           "  refresh              - REFRESH command (All Banks)\n"
           "  status               - Show all banks status\n"
           "===========================================================\n");
}

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
            ctrl_handle_data_command(&ctrl, &dram, &bus, cmd, b, (unsigned short)c, 0, 0);
        } else if (strcmp(cmd, "mwrite") == 0) {
            unsigned dm0, dm1;
            if (scanf("%u %u %u %u", &b, &c, &dm0, &dm1) != 4 || b >= NUM_BANKS) {
                printf("Error: mwrite requires <bank (0-%d)> <col> <dm0> <dm1>\n", NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            ctrl_handle_data_command(&ctrl, &dram, &bus, "write", b, (unsigned short)c, (unsigned char)dm0, (unsigned char)dm1);
        } else {
            printf("Unknown command '%s'. Type 'help' for usage.\n", cmd);
            while (getchar() != '\n');
        }
    }
    return 0;
}
