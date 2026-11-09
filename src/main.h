#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ctrl.h"
#include "chip.h"
#include "bus.h"

// Terminal ANSI Color Macros
#define RST   "\033[0m"
#define RED   "\033[31m"
#define GRN   "\033[32m"
#define YEL   "\033[33m"

static void usage(void) {
    printf("\n=== JEDEC DDR3 Simulator ===\n"
           "  Usage:\n"
           "  act <bank> <row>     - ACTIVATE command (Opens Row)\n"
           "  read <bank> <col>    - READ command (Read Burst + Strobes)\n"
           "  write <bank> <col>   - WRITE command (Write Burst + Strobes)\n"
           "  pre <bank>           - PRECHARGE command (Closes Row)\n"
           "  refresh              - REFRESH command (All Banks)\n"
           "  status               - Show all banks status\n"
           "===========================================================\n");
}

#endif /* __MAIN_H__ */
