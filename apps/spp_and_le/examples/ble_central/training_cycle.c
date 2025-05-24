#include "training_cycle.h"
#include "rd_light_common.h"
#include "system/includes.h"

#define SAVE_INTERVAL_MS            60000     // Save state every 1 minute
#define FACTORY_CHECK               0x55aa55aa

static training_cycle_status_t training_status;
static u32 tick_start_training = 0;
static u32 tick_count_save_ms = 0;

static const training_cycle_config_t cycle_configs[] = {
    // Cycle 1: 5s white 100%, 3s neutral 50%
    {100, CCT_LEVEL1, 5000},  // White 100%
    {50, CCT_LEVEL2, 3000},    // Neutral 50%
    
    // Cycle 2: 10min dim 10->100% white to yellow
    {10, 100, 600000}, // 10 minutes
    
    // Cycle 3: 15min white 100%
    {100, CCT_LEVEL1, 900000}, // 15 minutes
    
    // Cycle 4: 15min neutral 100%
    {100, CCT_LEVEL2, 900000},  // 15 minutes
    
    // Cycle 5: 15min yellow 100%
    {100, CCT_LEVEL3, 900000},   // 15 minutes
    
    // End state: neutral 10%
    {10, CCT_LEVEL2, 0}         // End state
};

void training_cycle_save_state(void)
{
    syscfg_write(CFG_VM_RD_TRAINING, &training_status, sizeof(training_status));
}

void training_cycle_load_state(void)
{
    syscfg_read(CFG_VM_RD_TRAINING, &training_status, sizeof(training_status));
}

void training_cycle_init(void)
{
    memset(&training_status, 0, sizeof(training_status));
    training_cycle_load_state();
    if(training_status.fac_check != FACTORY_CHECK)
    {
        training_status.fac_check = FACTORY_CHECK;
        training_status.is_running = 1;
        syscfg_write(CFG_VM_RD_TRAINING, &training_status, sizeof(training_status));
    }
    tick_start_training = training_status.cycle_start_time;
}

void training_cycle_stop()
{
    training_status.is_running = 0;
    training_status.cycle_start_time = 0;
    set_timer_pwm_duty(CCT_PWM_TIMER, dim_cct_2_pwm(CCT_LEVEL2));
    set_timer_pwm_duty(DIM_PWM_TIMER, dim_cct_2_pwm(10));
    training_cycle_save_state();
}

static uint16_t count  = 0;
static void update_light_state(u32 state)
{
    
    const training_cycle_config_t *config = &cycle_configs[state];
    
    if (state == TRAINING_CYCLE_2) {
        count ++;
        // cycle 10 -> 100 -> 10 dim
        u8 brightness = 100 - abs((count % 181) - 90);
        // cycle 0 -> 100 -> 0 cct
        u8 color_temp = 100 - abs((count % 201) - 100);
        
        set_timer_pwm_duty(CCT_PWM_TIMER, dim_cct_2_pwm(color_temp));
        set_timer_pwm_duty(DIM_PWM_TIMER, dim_cct_2_pwm(brightness));

    } else {
        set_timer_pwm_duty(CCT_PWM_TIMER, dim_cct_2_pwm(config->color_temp));
        set_timer_pwm_duty(DIM_PWM_TIMER, dim_cct_2_pwm(config->brightness));
    }
}


void training_cycle_task(void)
{
    static u32 lastick = 0;
    if (!training_status.is_running) {
        return;
    }

    u32 current_time = sys_timer_get_ms();
    u32 elapsed_time = current_time - lastick;
    
    // Check if need to save state
    if (current_time - tick_count_save_ms >= SAVE_INTERVAL_MS) {
        training_status.cycle_start_time = elapsed_time + tick_start_training;
        training_cycle_save_state();
        tick_count_save_ms = current_time;
    }
    
    // Process current cycle
    u32 current_state = training_status.current_state;
    const training_cycle_config_t *config = &cycle_configs[current_state];
    
    if (elapsed_time >= (config->duration_ms - tick_start_training)) {
        // Move to next cycle
        lastick = current_time;
        tick_start_training = 0;
        training_status.current_state++;
        
        if (training_status.current_state >= TRAINING_CYCLE_END) {
            training_status.current_state = TRAINING_CYCLE_END;
            training_cycle_stop();
            return;
        }
        
        // Update light state for new cycle
        update_light_state(training_status.current_state);
    } else {
        // Update light state for current cycle
        update_light_state(current_state);
    }
}

bool training_cycle_is_ended(void)
{
#if(TRAINING_MODE)
    return (!training_status.is_running || training_status.current_state >= TRAINING_CYCLE_END);
#else 
    return 1;
#endif
} 

