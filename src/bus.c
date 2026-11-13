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
    // Note: CK0 and CK1 are NOT reset here so clock runs continuously
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
    printf("TRACE,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
           cycle,
           bus->ck0, bus->ck1,
           bus->cs_n, bus->ras_n, bus->cas_n, bus->we_n,
           bus->ba.ba0, bus->ba.ba1, bus->ba.ba2,
           bus->addr.a0, bus->addr.a1, bus->addr.a2, bus->addr.a3,
           bus->addr.a4, bus->addr.a5, bus->addr.a6, bus->addr.a7,
           bus->addr.a8, bus->addr.a9, bus->addr.a10, bus->addr.a11,
           bus->addr.a12, bus->addr.a13,
           bus->dq.bytes[0].b0, bus->dq.bytes[0].b1, bus->dq.bytes[0].b2, bus->dq.bytes[0].b3,
           bus->dq.bytes[0].b4, bus->dq.bytes[0].b5, bus->dq.bytes[0].b6, bus->dq.bytes[0].b7,
           bus->dq.bytes[1].b0, bus->dq.bytes[1].b1, bus->dq.bytes[1].b2, bus->dq.bytes[1].b3,
           bus->dq.bytes[1].b4, bus->dq.bytes[1].b5, bus->dq.bytes[1].b6, bus->dq.bytes[1].b7,
           bus->dqs0, bus->dqs1, bus->dm0, bus->dm1);
    fflush(stdout);
}

void dram_bus_step_clock(unsigned long long *cycle, struct dram_bus_t *bus) {
    (*cycle)++;
    bus->ck0 = !bus->ck0;
    bus->ck1 = !bus->ck0;
    dram_bus_dump_trace(*cycle, bus);
}

/* stub for simulation, not actual storage/retrive */
void dram_bus_stream_burst(unsigned long long *cycle, struct dram_bus_t *bus, const unsigned char *pattern, unsigned length, unsigned char dm0, unsigned char dm1, unsigned char dm2, unsigned char dm3) {
    for (unsigned beat = 0; beat < length; beat++) {
        bus->dqs0 = 1;
        bus->dq.bytes[0].byte = pattern[beat];
        bus->dq.bytes[1].byte = pattern[beat] + 1;
        bus->dm0 = dm0;
        bus->dm1 = dm1;
        bus->dm2 = dm2;
        bus->dm3 = dm3;
        dram_bus_step_clock(cycle, bus);

        // --- Falling Strobe Edge ---
        bus->dqs0 = 0;
        dram_bus_step_clock(cycle, bus);
    }
}
