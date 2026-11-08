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

```mermaid
flowchart TD
    %% Define Node Styles
    classDef stateFill fill:#2d3748,stroke:#4a5568,stroke-width:2px,color:#fff
    classDef eventFill fill:#1a365d,stroke:#2b6cb0,stroke-width:2px,color:#fff

    %% State Nodes (Rectangles)
    RESET[RESET State]:::stateFill
    INIT[INITIALIZATION State]:::stateFill
    IDLE[IDLE State]:::stateFill
    BANK_ACT[BANK_ACTIVE State]:::stateFill
    READING[READING State]:::stateFill
    WRITING[WRITING State]:::stateFill
    PRE[PRECHARGING State]:::stateFill
    REF[REFRESHING State]:::stateFill

    %% Event Nodes (Ellipses)
    E_POWER(((POWER_ON Event))):::eventFill
    E_INIT_DONE(((INIT_DONE Event))):::eventFill
    E_ACT(((CMD_ACT Event))):::eventFill
    E_READ(((CMD_READ Event))):::eventFill
    E_WRITE(((CMD_WRITE Event))):::eventFill
    E_BURST_R(((BURST_DONE Event))):::eventFill
    E_BURST_W(((BURST_DONE Event))):::eventFill
    E_PRE(((CMD_PRE Event))):::eventFill
    E_REF(((CMD_REF Event))):::eventFill

    %% Flow Sequence
    RESET --> E_POWER --> INIT
    INIT --> E_INIT_DONE --> IDLE

    %% Idle Transitions
    IDLE --> E_ACT --> BANK_ACT
    IDLE --> E_REF --> REF
    REF --> IDLE

    %% Bank Active Operations
    BANK_ACT --> E_READ --> READING
    READING --> E_BURST_R --> BANK_ACT

    BANK_ACT --> E_WRITE --> WRITING
    WRITING --> E_BURST_W --> BANK_ACT

    BANK_ACT --> E_PRE --> PRE
    PRE --> IDLE
```

ref: JEDEC SOLID STATE TECHNOLOGY ASSOCIATION
JEDEC standard (JESD79-3C)
