#ifndef __FSM_H__
#define __FSM_H__


enum state_t {
    RESET,
    INITIALIZATION,
    IDLE,
    BANK_ACTIVE,
    READING,
    WRITING,
    PRECHARGING,
    REFRESHING
};

enum event_t {
    POWER_ON,
    INIT_DONE,
    CMD_ACT,      /* ACTIVATE command */
    CMD_READ,     /* READ command */
    CMD_WRITE,    /* WRITE command */
    CMD_PRE,      /* PRECHARGE command */
    CMD_REF,      /* REFRESH command */
    BURST_DONE    /* Burst Read/Write completed */
};

static const char *state_to_str(enum state_t state){
    switch (state) {
        case RESET:          return "RESET";
        case INITIALIZATION: return "INITIALIZATION";
        case IDLE:           return "IDLE";
        case BANK_ACTIVE:    return "BANK_ACTIVE";
        case READING:        return "READING";
        case WRITING:        return "WRITING";
        case PRECHARGING:    return "PRECHARGING";
        case REFRESHING:     return "REFRESHING";
        default:             return "UNKNOWN";
    }
}

#endif /* __FSM_H__ */