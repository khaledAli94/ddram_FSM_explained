#include "main.h"

static unsigned current_rank = 0; // defaults to rank 0

void usage(void) {
    printf("\n=== JEDEC DDR3 Simulation Engine ===\n"
           "  Usage:\n"
           "  rank <0|1>                      - Select default rank for subsequent commands (default 0)\n"
           "  act <bank> <row>                - ACTIVATE command (Opens Row)\n"
           "  read <bank> <col>               - READ command (Read Burst + Strobes) [STUB data]\n"
           "  write <bank> <col> <hex32>      - WRITE command (word on beat0, rest masked)\n"
           "  mwrite <bank> <col> <hex32> <dm0> <dm1> <dm2> <dm3> - Masked WRITE (mask applied every beat)\n"
           "  pre <bank>                      - PRECHARGE command (Closes Row)\n"
           "  refresh                         - REFRESH command (All Banks, current rank)\n"
           "  status                          - Show all ranks/banks status\n"
           "===========================================================\n");
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    setvbuf(stdout, NULL, _IONBF, 0);
    usage();

    struct ctrl_t ctrl = {0};
    struct phy_pins_t pins = {0};

    ddr_init_controller(&ctrl, MAX_RANKS); // both ranks enabled by default (no argv config system yet)
    ctrl_boot_sequence(&ctrl, &pins);
    ctrl_trace_pins(0, &pins); // idle cycle trace placeholder

    char cmd[16];
    unsigned b, r, c;

    while (printf("ddr3> ") && scanf("%15s", cmd) == 1) {
        if (strcmp(cmd, "status") == 0) {
            ctrl_handle_status(&ctrl);

        } else if (strcmp(cmd, "rank") == 0) {
            unsigned new_rank;
            if (scanf("%u", &new_rank) != 1 || new_rank >= MAX_RANKS) {
                printf("Error: rank requires <0|1>\n");
                while (getchar() != '\n');
                continue;
            }
            current_rank = new_rank;
            printf("Default rank set to %u\n", current_rank);

        } else if (strcmp(cmd, "refresh") == 0) {
            ctrl_handle_refresh(&ctrl, &pins, current_rank);

        } else if (strcmp(cmd, "act") == 0) {
            if (scanf("%u %u", &b, &r) != 2 || b >= NUM_BANKS) {
                printf("Error: act requires <bank (0-%d)> <row>\n", NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            ctrl_handle_activate(&ctrl, &pins, current_rank, b, (unsigned short)r);

        } else if (strcmp(cmd, "pre") == 0) {
            if (scanf("%u", &b) != 1 || b >= NUM_BANKS) {
                printf("Error: pre requires <bank (0-%d)>\n", NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            ctrl_handle_precharge(&ctrl, &pins, current_rank, b);

        } else if (strcmp(cmd, "read") == 0) {
            if (scanf("%u %u", &b, &c) != 2 || b >= NUM_BANKS) {
                printf("Error: read requires <bank (0-%d)> <col>\n", NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            ctrl_handle_read(&ctrl, &pins, current_rank, b, (unsigned short)c);

        } else if (strcmp(cmd, "write") == 0) {
            unsigned data_word;
            if (scanf("%u %u %x", &b, &c, &data_word) != 3 || b >= NUM_BANKS) {
                printf("Error: write requires <bank (0-%d)> <col> <hex32>\n", NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            // beat0 unmasked for Convenience, beats[1-7] fully masked
            unsigned char dm[DDR_MAX_BURST_LENGTH][4];
            for (unsigned beat = 0; beat < DDR_MAX_BURST_LENGTH; beat++) {
                unsigned char masked = (beat == 0) ? 0 : 1;
                for (int lane = 0; lane < 4; lane++) dm[beat][lane] = masked;
            }
            ctrl_handle_write(&ctrl, &pins, current_rank, b, (unsigned short)c, data_word, dm);

        } else if (strcmp(cmd, "mwrite") == 0) {
            unsigned data_word, dm0, dm1, dm2, dm3;
            if (scanf("%u %u %x %u %u %u %u", &b, &c, &data_word, &dm0, &dm1, &dm2, &dm3) != 7 || b >= NUM_BANKS) {
                printf("Error: mwrite requires <bank (0-%d)> <col> <hex32> <dm0> <dm1> <dm2> <dm3>\n", NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            // The underlying API (ctrl_handle_write) supports a TRUE
            unsigned char dm[DDR_MAX_BURST_LENGTH][4];
            for (unsigned beat = 0; beat < DDR_MAX_BURST_LENGTH; beat++) {
                dm[beat][0] = (unsigned char)dm0;
                dm[beat][1] = (unsigned char)dm1;
                dm[beat][2] = (unsigned char)dm2;
                dm[beat][3] = (unsigned char)dm3;
            }
            ctrl_handle_write(&ctrl, &pins, current_rank, b, (unsigned short)c, data_word, dm);

        } else {
            printf("Unknown command '%s'. Type 'help' for usage.\n", cmd);
            while (getchar() != '\n');
        }
    }
    return 0;
}