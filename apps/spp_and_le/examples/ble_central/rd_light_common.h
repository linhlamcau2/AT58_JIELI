#ifndef _RD_LIGHT_COMMON_H
#define _RD_LIGHT_COMMON_H

// #include <stdint.h>

#define FIRMWARE_VER_H  1
#define FIRMWARE_VER_L  0
#define LIMIT_RANGE(var, min, max) ((var) < (min) ? (min) : ((var) > (max) ? (max) : (var)))


#define DIM_PIN         IO_PORT_DM 
#define CCT_PIN         IO_PORTA_09
#define DETECT_POWER_PIN       IO_PORT_DP // IO_PORTA_01
// IO_PORT_DP

#define DIM_PWM_TIMER   JL_TIMER2
#define CCT_PWM_TIMER   JL_TIMER3


#define CCT_LEVEL1     100 //6500k
#define CCT_LEVEL2     50  //4000k
#define CCT_LEVEL3     0   //2700k

#define DIM_LEVEL0     0
#define DIM_LEVEL1     25
#define DIM_LEVEL2     50
#define DIM_LEVEL3     100

#define START_DIM_AFTER_POWERUP_MS 500  // start set dim after power up
#define DIM_POWERUP_DF DIM_LEVEL3 
#define CCT_POWERUP_DF CCT_LEVEL1
#define PWM_HZ          2000

#define TIMEOUT_CHANGE_CCT_MS 		6000
#define FLASH_INITED_CODE 0x5A5A5A5A // code check flash inited


#define CHANGE_CCT_BY_GPIO_EN 0 // 0: disable, 1: enable
typedef struct 
{
    uint16_t dim_present;
    uint16_t dim_target;
    uint16_t cct_present;
    uint16_t cct_target;
} rd_light_ctrl_t;

typedef struct 
{
	uint32_t Factory_Check;
	uint32_t eraser_counter; // count number of erase
	uint32_t dim_last;
	uint32_t cct_last;
	uint32_t timeout_powerup_flag; 
	/* data */
} rd_Flash_powerUp_data_t;

void rd_light_init(void);
void rd_light_set_dim_cct100(uint8_t dim100_set, uint8_t cct100_set);
void rd_light_set_dim100(uint8_t dim100_set);
void rd_light_set_cct100(uint8_t cct100_set);
void rd_light_check_ctrl_pwm(void);
void rd_flash_save_cct(uint8_t cct);
void rd_light_check_save_cct(void);
void rd_light_check_CCT_Pin(void);
uint8_t get_next_cct_level(uint8_t cct_level_last);
uint16_t rd_light_get_dim100(void);
uint8_t rd_flash_get_cct(void);
void rd_flash_save_timeOutFlag(uint8_t flag);
#endif //RD_LIGHT_COMMON_H