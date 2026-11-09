#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ctrl.h"
#include "chip.h"
#include "fsm.h"
#include "bus.h"

static void usage(void) {
    printf("\n=== DDR3 Bus-Centric Simulator (Data & Strobe Phase) ===\n"
           "JEDEC Command Truth Table Usage:\n"
           "  act <bank> <row>     - ACTIVATE command (Opens Row)\n"
           "  read <bank> <col>    - READ command (Read Burst + Strobes)\n"
           "  write <bank> <col>   - WRITE command (Write Burst + Strobes)\n"
           "  pre <bank>           - PRECHARGE command (Closes Row)\n"
           "  refresh              - REFRESH command (All Banks)\n"
           "  status               - Show all banks status\n"
           "=======================================================\n");
}

int main(void) {
    usage();
    
    struct ddr_ctrl_t ctrl;
    struct dram_chip_t dram;
    struct dram_bus_t bus;

    ddr3_init_controller(&ctrl);
    dram_chip_init(&dram);

    unsigned long long cycle = 0;

    // Initialize all banks in DRAM chip via the shared bus
    for (int i = 0; i < NUM_BANKS; i++) {
        dram_bus_encode(&bus, POWER_ON, i, 0);
        dram_bus_dump_trace(++cycle, &bus);
        process_event(&dram.banks[i], i, POWER_ON);

        dram_bus_encode(&bus, INIT_DONE, i, 0);
        dram_bus_dump_trace(++cycle, &bus);
        process_event(&dram.banks[i], i, INIT_DONE);
    }

    char cmd[16];
    unsigned b, r, c;

    while (printf("ddr3> ") && scanf("%15s", cmd) == 1) {
        if (strcmp(cmd, "status") == 0) {
            for (int i = 0; i < NUM_BANKS; i++) {
                printf("Bank %u \033[32m%s\033[0m, Active Row: %u\n", 
                       i, state_to_str(dram.banks[i].state), dram.banks[i].open_row);
            }
        } else if (strcmp(cmd, "refresh") == 0) {
            bool all_idle = true;
            for (int i = 0; i < NUM_BANKS; i++) {
                if (dram.banks[i].state != IDLE) {
                    all_idle = false;
                    break;
                }
            }
            if (!all_idle) {
                printf("[Protocol Error] \033[33mCannot issue refresh. All banks must be IDLE.\033[0m\n");
            } else {
                for (int i = 0; i < NUM_BANKS; i++) {
                    dram_bus_encode(&bus, CMD_REF, i, 0);
                    dram_bus_dump_trace(++cycle, &bus);
                    enum event_t ev = dram_bus_decode(&bus);
                    process_event(&dram.banks[i], i, ev);

                    dram_bus_encode(&bus, REF_DONE, i, 0);
                    dram_bus_dump_trace(++cycle, &bus);
                    process_event(&dram.banks[i], i, REF_DONE);
                }
            }
        } else if (strcmp(cmd, "act") == 0) {
            if (scanf("%u %u", &b, &r) != 2 || b >= NUM_BANKS) {
                printf("Error: act requires <bank (0-%d)> <row>\n", NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            struct dram_bank_t *bank = &dram.banks[b];
            if (bank->state != IDLE) {
                printf("[Protocol Error] Cannot ACTIVATE Bank %u from state \033[32m%s\033[0m (Bank must be \033[33mIDLE\033[0m)\n",
                       b, state_to_str(bank->state));
                continue;
            }
            bank->open_row = r;

            ctrl_encode_activate(&bus, b, (unsigned short)r);
            dram_bus_dump_trace(++cycle, &bus);
            enum event_t ev = dram_bus_decode(&bus);
            process_event(bank, b, ev);
        } else if (strcmp(cmd, "pre") == 0) {
            if (scanf("%u", &b) != 1 || b >= NUM_BANKS) {
                printf("Error: pre requires <bank (0-%d)>\n", NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            struct dram_bank_t *bank = &dram.banks[b];
            if (bank->state != BANK_ACTIVE) {
                printf("[Protocol Error] Cannot PRECHARGE Bank %u from state \033[32m%s\033[0m (Bank must be \033[33mBANK_ACTIVE\033[0m)\n",
                       b, state_to_str(bank->state));
                continue;
            }
            dram_bus_encode(&bus, CMD_PRE, b, 0);
            dram_bus_dump_trace(++cycle, &bus);
            enum event_t ev = dram_bus_decode(&bus);
            process_event(bank, b, ev);
            process_event(bank, b, PRE_DONE);
        } else if (strcmp(cmd, "read") == 0 || strcmp(cmd, "write") == 0) {
            if (scanf("%u %u", &b, &c) != 2 || b >= NUM_BANKS) {
                printf("Error: %s requires <bank (0-%d)> <col>\n", cmd, NUM_BANKS - 1);
                while (getchar() != '\n');
                continue;
            }
            struct dram_bank_t *bank = &dram.banks[b];
            if (bank->state != BANK_ACTIVE) {
                printf("[Protocol Error] Cannot %s Bank %u from state \033[32m%s\033[0m (Bank must be \033[33mBANK_ACTIVE\033[0m)\n",
                       cmd, b, state_to_str(bank->state));
                continue;
            }

            // 1. Command Phase
            enum event_t cmd_ev = (strcmp(cmd, "read") == 0) ? CMD_READ : CMD_WRITE;
            ctrl_encode_column_cmd(&bus, cmd_ev, b, (unsigned short)c, false);
            dram_bus_dump_trace(++cycle, &bus);
            enum event_t ev = dram_bus_decode(&bus);
            process_event(bank, b, ev);

            // 2. Latency Phase (tCL for read, tCWL for write)
            unsigned latency = (cmd_ev == CMD_READ) ? ctrl.timing.tCL : ctrl.timing.tCWL;
            ctrl_wait_cycles(&cycle, &bus, latency);

            // 3. Data Phase (Strobe toggling & byte streaming for BL8)
            unsigned char dummy_pattern[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
            dram_bus_stream_burst(&cycle, &bus, dummy_pattern, ctrl.timing.burst_length);

            // Finish burst
            process_event(bank, b, BURST_DONE);
        } else {
            printf("Unknown command. Type act, read, write, pre, refresh, or status.\n");
            while (getchar() != '\n');
        }
    }
    return 0;
}
