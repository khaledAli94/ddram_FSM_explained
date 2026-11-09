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

## usage:
![](img/usage.gif)

## final goal "under construction":
![](img/effective_result.gif)

## Simplified State Diagram 
![](img/DRAM_FSM.png)


## correct read/write operation sequence:
```mermaid
stateDiagram-v2
    direction LR
    IDLE --> BANK_ACTIVE: 1. CMD_ACT
    BANK_ACTIVE --> READING_WRITING: 2. CMD_READ / CMD_WRITE
    READING_WRITING --> IDLE: 3. CMD_PRE
```

## FSM implm.
```mermaid
stateDiagram-v2
    [*] --> RESET
    RESET --> INITIALIZATION: POWER_ON
    INITIALIZATION --> IDLE: INIT_DONE

    IDLE --> REFRESHING: CMD_REF
    REFRESHING --> IDLE: BURST_DONE

    IDLE --> BANK_ACTIVE: CMD_ACT
    
    BANK_ACTIVE --> READING: CMD_READ
    READING --> BANK_ACTIVE: BURST_DONE

    BANK_ACTIVE --> WRITING: CMD_WRITE
    WRITING --> BANK_ACTIVE: BURST_DONE

    BANK_ACTIVE --> PRECHARGING: CMD_PRE
    PRECHARGING --> IDLE: BURST_DONE
```

## ref: JEDEC SOLID STATE TECHNOLOGY ASSOCIATION
JEDEC standard (JESD79-3C)
