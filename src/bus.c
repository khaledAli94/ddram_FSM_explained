#include "bus.h"
#include <stdio.h>

enum event_t dram_bus_decode(const struct dram_bus_t *bus) {
    if (bus->cs_n == 1) return CMD_NOP; // Deselect / NOP

    // JEDEC Command Truth Table Decoding (sampled on rising edge of CK0)
    if (bus->ras_n == 0 && bus->cas_n == 1 && bus->we_n == 1) return CMD_ACT;
    if (bus->ras_n == 1 && bus->cas_n == 0 && bus->we_n == 1) return CMD_READ;
    if (bus->ras_n == 1 && bus->cas_n == 0 && bus->we_n == 0) return CMD_WRITE;
    if (bus->ras_n == 0 && bus->cas_n == 1 && bus->we_n == 0) return CMD_PRE;
    if (bus->ras_n == 0 && bus->cas_n == 0 && bus->we_n == 0) return CMD_REF;

    return CMD_NOP;
}

void dram_bus_encode(struct dram_bus_t *bus, enum event_t event, unsigned bank, unsigned short addr) {
    // Default bus state: Deselect (all control lines high)
    bus->ck0 = 1;
    bus->ck1 = 1;
    bus->cke = 1;
    bus->reset_n = 1;
    bus->cs_n = 1;
    bus->ras_n = 1;
    bus->cas_n = 1;
    bus->we_n = 1;
    bus->odt = 0;
    bus->ba.val = (unsigned char)bank;
    bus->addr.val = addr;
    bus->dq.word32 = 0;
    bus->dqs0 = 0;
    bus->dqs1 = 0;
    bus->dqs2 = 0;
    bus->dqs3 = 0;
    bus->dm0 = 0;
    bus->dm1 = 0;
    bus->dm2 = 0;
    bus->dm3 = 0;

    switch (event) {
        case CMD_ACT:
            bus->cs_n = 0; bus->ras_n = 0; bus->cas_n = 1; bus->we_n = 1;
            break;
        case CMD_READ:
            bus->cs_n = 0; bus->ras_n = 1; bus->cas_n = 0; bus->we_n = 1;
            break;
        case CMD_WRITE:
            bus->cs_n = 0; bus->ras_n = 1; bus->cas_n = 0; bus->we_n = 0;
            break;
        case CMD_PRE:
            bus->cs_n = 0; bus->ras_n = 0; bus->cas_n = 1; bus->we_n = 0;
            break;
        case CMD_REF:
            bus->cs_n = 0; bus->ras_n = 0; bus->cas_n = 0; bus->we_n = 0;
            break;
        case CMD_NOP:
        default:
            bus->cs_n = 1; bus->ras_n = 1; bus->cas_n = 1; bus->we_n = 1;
            break;
    }
}

void dram_bus_dump_trace(unsigned long long cycle, const struct dram_bus_t *bus) {
    // Format: TRACE,CYCLE,CK,CS#,RAS#,CAS#,WE#,BA,ADDR
    printf("TRACE,%llu,%u,%u,%u,%u,%u,%u,%u\n",
           cycle, bus->ck0, bus->cs_n, bus->ras_n,
           bus->cas_n, bus->we_n, bus->ba.val, bus->addr.val);
    fflush(stdout);
}

void dram_bus_stream_burst(unsigned long long *cycle, struct dram_bus_t *bus, const unsigned char *pattern, unsigned length) {
    for (unsigned beat = 0; beat < length; beat++) {
        // --- Rising Strobe Edge ---
        (*cycle)++;
        bus->ck0 = 1;
        bus->dqs0 = 1;
        bus->dq.bytes[0].byte = pattern[beat];
        bus->dq.bytes[1].byte = pattern[beat] + 1;
        dram_bus_dump_trace(*cycle, bus);

        // --- Falling Strobe Edge ---
        (*cycle)++;
        bus->ck0 = 0;
        bus->dqs0 = 0;
        dram_bus_dump_trace(*cycle, bus);
    }
}
