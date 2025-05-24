// #include "system/app_core.h"
#include "system/includes.h"
#include "rd_light_common.h"
#include "K9B_remote.h"
#include "training_cycle.h"

const uint16_t DIM_LUMEN_TO_PWM[101] = {
    0, 1, 4, 9, 16, 25, 36, 49, 64, 81,
    100, 121, 144, 169, 196, 225, 256, 289, 324, 361,
    400, 441, 484, 529, 576, 625, 676, 729, 784, 841,
    900, 961, 1024, 1089, 1156, 1225, 1296, 1369, 1444, 1521,
    1600, 1681, 1764, 1849, 1936, 2025, 2116, 2209, 2304, 2401,
    2500, 2601, 2704, 2809, 2916, 3025, 3136, 3249, 3364, 3481,
    3600, 3721, 3844, 3969, 4096, 4225, 4356, 4489, 4624, 4761,
    4900, 5041, 5184, 5329, 5476, 5625, 5776, 5929, 6084, 6241,
    6400, 6561, 6724, 6889, 7056, 7225, 7396, 7569, 7744, 7921,
    8100, 8281, 8464, 8649, 8836, 9025, 9216, 9409, 9604, 9801,
    10000};
volatile rd_light_ctrl_t rd_light_ctrl_val = {0};
static void rd_gpio_init(void);
void rd_flash_powerup_init(void);
static inline uint16_t dim_cct_2_pwm(uint8_t dim_cct100);
volatile rd_Flash_powerUp_data_t rd_Flash_powerup_Val = {0};

void rd_light_init(void)
{
    training_cycle_init();
    rd_gpio_init();
    rd_init_queue_message();
    if(training_cycle_is_ended())
    {
        rd_flash_powerup_init();
        rd_K9B_flash_init();       
    }
    
    printf("Rang Dong DownLight DM BLE V%02d.%02d.01", FIRMWARE_VER_H, FIRMWARE_VER_L);
}

static void rd_gpio_init(void)
{
    timer_pwm_init(DIM_PWM_TIMER, DIM_PIN, PWM_HZ, (u32)dim_cct_2_pwm(5)); // 1KHz 50%

    timer_pwm_init(CCT_PWM_TIMER, CCT_PIN, PWM_HZ, 0); // 1KHz 50%
    rd_light_ctrl_val.dim_present = 5;
    rd_light_ctrl_val.dim_target = 5;
#if (CHANGE_CCT_BY_GPIO_EN)
    usb_iomode(0); // 先执行此函数，切换usb成gpio模式
    gpio_set_pull_up(IO_PORT_DP, 1);
    gpio_set_pull_down(IO_PORT_DP, 0);
    gpio_set_direction(IO_PORT_DP, 1);
    gpio_set_die(IO_PORT_DP, 1);
    gpio_set_dieh(IO_PORT_DP, 0);

#endif
}

uint16_t dim_cct_2_pwm(uint8_t dim_cct100)
{
    dim_cct100 = LIMIT_RANGE(dim_cct100, 0, 100);
    return (dim_cct100 * 100);
}

static inline uint16_t dim_mapping_2_pwm(uint8_t dim_100)
{
    dim_100 = LIMIT_RANGE(dim_100, 0, 100);
    return (DIM_LUMEN_TO_PWM[dim_100]);
}
/**
 * set dim 100
 */
void rd_light_set_dim100(uint8_t dim100_set)
{
    dim100_set = LIMIT_RANGE(dim100_set, 0, 100);
    rd_light_ctrl_val.dim_target = dim100_set;
}

void rd_light_set_cct100(uint8_t cct100_set)
{
    cct100_set = LIMIT_RANGE(cct100_set, 0, 100);
    rd_light_ctrl_val.cct_target = cct100_set;
}

uint16_t rd_light_get_dim100(void)
{
    return rd_light_ctrl_val.dim_present;
}

uint16_t rd_light_get_cct100(void)
{
    return rd_light_ctrl_val.cct_present;
}

void rd_light_set_dim_cct100(uint8_t dim100_set, uint8_t cct100_set)
{
    dim100_set = LIMIT_RANGE(dim100_set, 0, 100);
    cct100_set = LIMIT_RANGE(cct100_set, 0, 100);
    rd_light_ctrl_val.dim_target = dim100_set;
    rd_light_ctrl_val.cct_target = cct100_set;
}

uint8_t count_blink = 0;

void rd_set_blink(void)
{
    count_blink = 2;
}

void rd_blink_led()
{
    if(count_blink == 0)
    {
        return;
    }
    uint16_t dim_stt = (count_blink % 2) ? 100 : 0;
    set_timer_pwm_duty(DIM_PWM_TIMER, dim_cct_2_pwm(dim_stt));
    count_blink--;
}

void rd_light_check_ctrl_pwm(void)
{
    const uint8_t DIM_STEP = 1; // %
    const uint8_t CCT_STEP = 1; // %

    // check cct pwm
    if (rd_light_ctrl_val.cct_present != rd_light_ctrl_val.cct_target)
    {
        if (rd_light_ctrl_val.cct_present < rd_light_ctrl_val.cct_target)
        {
            rd_light_ctrl_val.cct_present += CCT_STEP;
        }
        if (rd_light_ctrl_val.cct_present > rd_light_ctrl_val.cct_target)
        {
            rd_light_ctrl_val.cct_present -= CCT_STEP;
        }
        set_timer_pwm_duty(CCT_PWM_TIMER, dim_cct_2_pwm(rd_light_ctrl_val.cct_present));

        // set_timer_pwm_duty(JL_TIMER2, dim_stt_last/2);
    }

    // check dim pwm
    if (count_blink == 0 && rd_light_ctrl_val.dim_present != rd_light_ctrl_val.dim_target)
    {
        if (rd_light_ctrl_val.dim_present < rd_light_ctrl_val.dim_target)
        {
            rd_light_ctrl_val.dim_present += DIM_STEP;
        }
        if (rd_light_ctrl_val.dim_present > rd_light_ctrl_val.dim_target)
        {
            rd_light_ctrl_val.dim_present -= DIM_STEP;
        }
        set_timer_pwm_duty(DIM_PWM_TIMER, dim_cct_2_pwm(rd_light_ctrl_val.dim_present));
        // set_timer_pwm_duty(DIM_PWM_TIMER, dim_mapping_2_pwm(rd_light_ctrl_val.dim_present));

        // set_timer_pwm_duty(JL_TIMER2, dim_stt_last/2);
    }
}

#define NUM_CHECK_POWER 8
void rd_light_check_CCT_Pin(void)
{
    static u16 cct_stt = 0;
    static uint32_t clk_time_last_ms = 0;
    static uint8_t count = 0;
    if (sys_timer_get_ms() > 600)
    {
        if (gpio_read(DETECT_POWER_PIN))
        {
            count++;
            if (count == NUM_CHECK_POWER)               //mat nguon AC
            {
                if(sys_timer_get_ms() - clk_time_last_ms < 6000)
                {
                    cct_stt = get_next_cct_level(rd_light_ctrl_val.cct_target);
                    rd_light_set_cct100(cct_stt);
                    rd_flash_save_cct(cct_stt);
                }    
                clk_time_last_ms = sys_timer_get_ms();
            }
            else if (count > NUM_CHECK_POWER)
                count = NUM_CHECK_POWER + 1;
        }
        else
        {
            count = 0;
        }
    }

    // if(need_keep_ctt && rd_Flash_powerup_Val.cct_last == cct_stt && sys_timer_get_ms() - clk_time_last_ms > 6000)
    // {
    //     need_keep_ctt = 0;
    //     rd_flash_save_cct(cct_stt_pre);
    // }

    else
    {
        count = 0;
    }
}
void rd_light_check_save_cct(void)
{
    static uint32_t clk_time_last_ms = 0;
    static uint8_t change_cct_timeout = 0;
    if (clk_time_last_ms > sys_timer_get_ms())
    {
        clk_time_last_ms = sys_timer_get_ms();
    }

    if (sys_timer_get_ms() - clk_time_last_ms > 3000)
    {

        clk_time_last_ms = sys_timer_get_ms();
        if ((rd_Flash_powerup_Val.cct_last != rd_light_ctrl_val.cct_target))
        {
            rd_flash_save_cct(rd_light_ctrl_val.cct_target);
        }
    }

    /*----------------------Check timeout cct-----------------*/
    if ((change_cct_timeout == 0) && (sys_timer_get_ms() > TIMEOUT_CHANGE_CCT_MS))
    {
        change_cct_timeout = 1;
        rd_flash_save_timeOutFlag(0);
    }
}

uint8_t get_next_cct_level(uint8_t cct_level_last)
{
    switch (cct_level_last)
    {
    case CCT_LEVEL3:
        return CCT_LEVEL2;
    case CCT_LEVEL2:
        return CCT_LEVEL1;
    case CCT_LEVEL1:
        return CCT_LEVEL3;
    default:
        return CCT_LEVEL3;
    }
}

uint8_t get_next_dim_level(uint8_t dim)
{
    switch (dim)
    {
    case DIM_LEVEL1:
        return DIM_LEVEL2;
    case DIM_LEVEL2:
        return DIM_LEVEL3;
    case DIM_LEVEL3:
        return DIM_LEVEL1;
    default:
        return DIM_LEVEL1;
    }
}
/*----------------------flash powerup-----------------*/

void rd_flash_powerup_saveDF(void)
{
    rd_Flash_powerup_Val.Factory_Check = FLASH_INITED_CODE;
    rd_Flash_powerup_Val.cct_last = CCT_POWERUP_DF;
    rd_Flash_powerup_Val.eraser_counter = 0;
    rd_Flash_powerup_Val.timeout_powerup_flag = 0;
    syscfg_write(CFG_VM_RD_POWERUP, &rd_Flash_powerup_Val, sizeof(rd_Flash_powerup_Val));

    printf("flash powerup save default\n");
}

void rd_flash_powerup_init(void)
{
    int ret = syscfg_read(CFG_VM_RD_POWERUP, &rd_Flash_powerup_Val, sizeof(rd_Flash_powerup_Val));
    printf("flash powerup read [write:%d]", ret);

    if (FLASH_INITED_CODE != rd_Flash_powerup_Val.Factory_Check)
    {
        rd_flash_powerup_saveDF();
        syscfg_read(CFG_VM_RD_POWERUP, &rd_Flash_powerup_Val, sizeof(rd_Flash_powerup_Val));
    }
    printf("cct_last:%d - eraser_counter:%d\n", rd_Flash_powerup_Val.cct_last, rd_Flash_powerup_Val.eraser_counter);

    if(rd_Flash_powerup_Val.count_reset_continuous >= NUM_RESET_KEEP_CCT - 1)
    {
        rd_Flash_powerup_Val.count_reset_continuous = 0;
        rd_Flash_powerup_Val.cct_last = CCT_LEVEL1;
        rd_Flash_powerup_Val.timeout_powerup_flag = 0;
        rd_flash_save_cct(rd_Flash_powerup_Val.cct_last);
    }
    else 
    {
        if (rd_Flash_powerup_Val.timeout_powerup_flag)
        {
            rd_Flash_powerup_Val.count_reset_continuous ++;
            rd_Flash_powerup_Val.cct_last = get_next_cct_level(rd_Flash_powerup_Val.cct_last);
            printf("Set next cct level:%d\n", rd_Flash_powerup_Val.cct_last);
            rd_flash_save_cct(rd_Flash_powerup_Val.cct_last);
        }
        else
        {
            rd_flash_save_timeOutFlag(1);
        }
    }
}

void rd_flash_save_timeOutFlag(uint8_t flag)
{
    rd_Flash_powerup_Val.timeout_powerup_flag = flag;
    rd_Flash_powerup_Val.eraser_counter++;
    if(!flag) rd_Flash_powerup_Val.count_reset_continuous = 0;
    syscfg_write(CFG_VM_RD_POWERUP, &rd_Flash_powerup_Val, sizeof(rd_Flash_powerup_Val));
    printf("save time out flag:%d\n", flag);
}
void rd_flash_save_cct(uint8_t cct)
{
    rd_Flash_powerup_Val.cct_last = cct;
    rd_Flash_powerup_Val.eraser_counter++;
    syscfg_write(CFG_VM_RD_POWERUP, &rd_Flash_powerup_Val, sizeof(rd_Flash_powerup_Val));
    printf("save cct:%d\n", cct);
}

uint8_t rd_flash_get_cct(void)
{
    return rd_Flash_powerup_Val.cct_last;
}

