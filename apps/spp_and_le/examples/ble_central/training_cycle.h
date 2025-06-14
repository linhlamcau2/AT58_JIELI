#ifndef __TRAINING_CYCLE_H__
#define __TRAINING_CYCLE_H__

#define TRAINING_MODE 0
// Training cycle states
#include <stdint.h>
#include "system/includes.h"
enum {
    TRAINING_CYCLE_1 = 0,
    TRAINING_CYCLE_1_2,
    TRAINING_CYCLE_2,
    TRAINING_CYCLE_3,
    TRAINING_CYCLE_4,
    TRAINING_CYCLE_5,
    TRAINING_CYCLE_END,
} ;

// Training cycle configuration structure
typedef struct {
    u8 brightness;      // 0-100%
    u8 color_temp;      // 0-100% (0% = warm, 100% = cool)
    u32 duration_ms;    // Duration in milliseconds
} training_cycle_config_t;

// Training cycle status structure
typedef struct {
    u32 fac_check;
    u32 current_state;
    u32 cycle_start_time;
    u8 is_running;
} training_cycle_status_t;

// Function declarations
void training_cycle_init(void);
void training_cycle_task(void);
bool training_cycle_is_ended(void);

#endif /* __TRAINING_CYCLE_H__ */ 