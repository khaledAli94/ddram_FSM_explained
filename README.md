# ddram_FSM_explained
this Repo is for demonstrating how ddr finite state machine works in principle, so dont mix between real hardware DDR-controller with physical layer with this FSM concpt 

FSM data struct:

```c
enum ddr3_state_t {
    RESET,
    INITIALIZATION,
    IDLE,
    BANK_ACTIVE,
    READING,
    WRITING,
    PRECHARGING,
    REFRESHING
};

enum ddr3_event_t {
    POWER_ON,
    INIT_DONE,
    CMD_ACT,      /* ACTIVATE command */
    CMD_READ,     /* READ command */
    CMD_WRITE,    /* WRITE command */
    CMD_PRE,      /* PRECHARGE command */
    CMD_REF,      /* REFRESH command */
    BURST_DONE    /* Burst Read/Write completed */
};
```

## Simplified State Diagram 
![](DRAM_FSM.png)


correct read/write operation sequence:

``` mermaid
sequenceDiagram
    autonumber
    actor CPU as CPU / Bus Master
    participant Ctrl as DDR3 Controller (FSM)
    participant DRAM as DDR3 SDRAM Chip

    %% 1. Initialization Sequence
    rect rgb(240, 240, 240)
        note over CPU, DRAM: 1. Power-Up & Initialization Phase
        CPU->>Ctrl: Power-On Signal (POWER_ON)
        Ctrl->>DRAM: RESET# & CKE Initialization Signals
        Ctrl->>DRAM: Mode Register Set (MR0-MR3) & ZQ Calibration
        Ctrl-->>CPU: Controller Ready (INIT_DONE)
    end

    %% 2. Activate Row Sequence
    rect rgb(230, 245, 230)
        note over CPU, DRAM: 2. Row Activation (Row Miss Scenario)
        CPU->>Ctrl: Read Request (Address: Bank 0, Row A, Col X)
        Ctrl->>DRAM: CMD_ACT (Bank 0, Row A)
        note over DRAM: Open Row A into Sense Amplifiers<br/>Must wait tRCD timing delay
    end

    %% 3. Read / Write Burst Sequence
    rect rgb(230, 230, 250)
        note over CPU, DRAM: 3. Read/Write Column Access (Row Hit)
        Ctrl->>DRAM: CMD_READ (Bank 0, Col X)
        note over DRAM: Wait CAS Latency (tCL)
        DRAM-->>Ctrl: Data Burst (DQ Lines)
        Ctrl-->>CPU: Return Requested Data

        CPU->>Ctrl: Write Request (Address: Bank 0, Row A, Col Y)
        Ctrl->>DRAM: CMD_WRITE (Bank 0, Col Y) + Data Burst
        note over DRAM: Write Data into Open Row Buffer<br/>Must wait tWR timing delay
    end

    %% 4. Precharge Sequence
    rect rgb(255, 235, 235)
        note over CPU, DRAM: 4. Row Deactivation (Precharge)
        CPU->>Ctrl: Request Access to Row B (Different Row)
        Ctrl->>DRAM: CMD_PRE (Bank 0)
        note over DRAM: Close Row A & Recharge Capacitors<br/>Must wait tRP timing delay
        Ctrl->>DRAM: CMD_ACT (Bank 0, Row B)
        note over Ctrl, DRAM: State machine continues with Row B access...
    end
```

ref: JEDEC SOLID STATE TECHNOLOGY ASSOCIATION
JEDEC standard (JESD79-3C)
