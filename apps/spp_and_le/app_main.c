/*********************************************************************************************
    *   Filename        : app_main.c

    *   Description     :

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "app_main.h"
#include "update.h"
#include "update_loader_download.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "asm/charge.h"

#include "examples/ble_central/rd_light_common.h"
#include "examples/ble_central/K9B_remote.h"
#include "examples/ble_central/training_cycle.h"

#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
#include "jl_kws/jl_kws_api.h"
#endif /* #if TCFG_KWS_VOICE_RECOGNITION_ENABLE */

#define LOG_TAG_CONST APP
#define LOG_TAG "[APP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

/*任务列表 */
const struct task_info task_info_table[] = {
    {"app_core", 1, 0, 640, 128},
    {"sys_event", 7, 0, 256, 0},
    {"btctrler", 4, 0, 512, 256},
    {"btencry", 1, 0, 512, 128},
    {"btstack", 3, 0, 768, 256},
    {"systimer", 7, 0, 128, 0},
    {"update", 1, 0, 512, 0},
    {"dw_update", 2, 0, 256, 128},
#if (RCSP_BTMATE_EN)
    {"rcsp_task", 2, 0, 640, 0},
#endif
#if (USER_UART_UPDATE_ENABLE)
    {"uart_update", 1, 0, 256, 128},
#endif
#if (XM_MMA_EN)
    {"xm_mma", 2, 0, 640, 256},
#endif
    {"usb_msd", 1, 0, 512, 128},
#if TCFG_AUDIO_ENABLE
    {"audio_dec", 3, 0, 768, 128},
    {"audio_enc", 4, 0, 512, 128},
#endif /*TCFG_AUDIO_ENABLE*/
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
    {"kws", 2, 0, 256, 64},
#endif /* #if TCFG_KWS_VOICE_RECOGNITION_ENABLE */
#if (TUYA_DEMO_EN)
    {"user_deal", 7, 0, 512, 512}, // 定义线程 tuya任务调度
#endif
#if (CONFIG_APP_HILINK)
    {"hilink_task", 2, 0, 1024, 0}, // 定义线程 hilink任务调度
#endif
    {0, 0},
};
static u8 rd_flash_data[512] __attribute__((aligned(4)));
APP_VAR app_var;

void app_var_init(void)
{
    app_var.play_poweron_tone = 1;

    app_var.auto_off_time = TCFG_AUTO_SHUT_DOWN_TIME;
    app_var.warning_tone_v = 350;
    app_var.poweroff_tone_v = 200;
}

__attribute__((weak))
u8
get_charge_online_flag(void)
{
    return 0;
}

void clr_wdt(void);
void check_power_on_key(void)
{
#if TCFG_POWER_ON_NEED_KEY

    u32 delay_10ms_cnt = 0;
    while (1)
    {
        clr_wdt();
        os_time_dly(1);

        extern u8 get_power_on_status(void);
        if (get_power_on_status())
        {
            log_info("+");
            delay_10ms_cnt++;
            if (delay_10ms_cnt > 70)
            {
                /* extern void set_key_poweron_flag(u8 flag); */
                /* set_key_poweron_flag(1); */
                return;
            }
        }
        else
        {
            log_info("-");
            delay_10ms_cnt = 0;
            log_info("enter softpoweroff\n");
            power_set_soft_poweroff();
        }
    }
#endif
}

static void central_timer_handle_test4(void)
{
    rd_handle_queue_message();
}

static void central_timer_handle_test3(void)
{
    rd_light_check_CCT_Pin();
}
static void central_timer_handle_test(void)
{

    rd_light_check_ctrl_pwm();
    rd_light_check_save_cct();

#if (CHANGE_CCT_BY_GPIO_EN)
//   rd_light_check_CCT_Pin(); // change cct by GPIO when power off
#endif
}

static void central_timer_handle_test2(void)
{
    static u8 led_stt_last = 2;
    static u16 dim_stt_last = 0;
    static u8 set_ctrl_df = 0;
    if ((set_ctrl_df == 0) && (sys_timer_get_ms() > 500))
    {
        set_ctrl_df = 1;
        rd_light_set_dim_cct100(DIM_POWERUP_DF, rd_flash_get_cct());
    }

    log_info("task main %d- Wdt:%d \n", sys_timer_get_ms(), wdt_get_time());

    rd_blink_led();
}
void app_main()
{
    struct intent it;

    app_var_init();
    if (!UPDATE_SUPPORT_DEV_IS_NULL())
    {
        int update = 0;
        update = update_result_deal();
    }

    printf(">>>>>>>>>>>>>>>>>app_main...\n");
    printf(">>> v220,2022-11-23 >>>\n");

    if (get_charge_online_flag())
    {
#if (TCFG_SYS_LVD_EN == 1)
        vbat_check_init();
#endif
    }
    else
    {
        check_power_on_voltage();
    }

#if TCFG_POWER_ON_NEED_KEY
    check_power_on_key();
#endif

#if TCFG_AUDIO_ENABLE
    extern int audio_dec_init();
    extern int audio_enc_init();
    audio_dec_init();
    audio_enc_init();
#endif /*TCFG_AUDIO_ENABLE*/

#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
    jl_kws_main_user_demo();
#endif /* #if TCFG_KWS_VOICE_RECOGNITION_ENABLE */

    init_intent(&it);

#if CONFIG_APP_SPP_LE
    it.name = "spp_le";
    it.action = ACTION_SPPLE_MAIN;

#elif CONFIG_APP_AT_COM || CONFIG_APP_AT_CHAR_COM
    it.name = "at_com";
    it.action = ACTION_AT_COM;

#elif CONFIG_APP_DONGLE
    it.name = "dongle";
    it.action = ACTION_DONGLE_MAIN;

#elif CONFIG_APP_MULTI
    it.name = "multi_conn";
    it.action = ACTION_MULTI_MAIN;

#elif CONFIG_APP_NONCONN_24G
    it.name = "nonconn_24g";
    it.action = ACTION_NOCONN_24G_MAIN;

#elif CONFIG_APP_HILINK
    it.name = "hilink";
    it.action = ACTION_HILINK_MAIN;

#elif CONFIG_APP_LL_SYNC
    it.name = "ll_sync";
    it.action = ACTION_LL_SYNC;

#elif CONFIG_APP_TUYA
    it.name = "tuya";
    it.action = ACTION_TUYA;

#elif CONFIG_APP_CENTRAL
    it.name = "central";
    it.action = ACTION_CENTRAL_MAIN;

#elif CONFIG_APP_DONGLE
    it.name = "dongle";
    it.action = ACTION_DONGLE_MAIN;

#elif CONFIG_APP_BEACON
    it.name = "beacon";
    it.action = ACTION_BEACON_MAIN;

#elif CONFIG_APP_IDLE
    it.name = "idle";
    it.action = ACTION_IDLE_MAIN;

#elif CONFIG_APP_CONN_24G
    it.name = "conn_24g";
    it.action = ACTION_CONN_24G_MAIN;
#else
    while (1)
    {
        printf("no app!!!");
    }
#endif

    log_info("run app>>> %s", it.name);
    log_info("%s,%s", __DATE__, __TIME__);
    start_app(&it);

    put_buf(rd_flash_data, 16);

    rd_light_init();

    if (training_cycle_is_ended())
    {
        sys_timer_add(NULL, central_timer_handle_test2, 1000);
        sys_timer_add(NULL, central_timer_handle_test, 10);
#if (CHANGE_CCT_BY_GPIO_EN)
        sys_timer_add(NULL, central_timer_handle_test3, 5);
#endif
        
        sys_timer_add(NULL, central_timer_handle_test4, 500);
    }
    else
    {
        sys_timer_add(NULL, training_cycle_task, 10);
    }
#if TCFG_CHARGE_ENABLE
    set_charge_event_flag(1);
#endif
}

/*
 * app模式切换
 */
void app_switch(const char *name, int action)
{
    struct intent it;
    struct application *app;

    log_info("app_exit\n");

    init_intent(&it);
    app = get_current_app();
    if (app)
    {
        /*
         * 退出当前app, 会执行state_machine()函数中APP_STA_STOP 和 APP_STA_DESTORY
         */
        it.name = app->name;
        it.action = ACTION_BACK;
        start_app(&it);
    }

    /*
     * 切换到app (name)并执行action分支
     */
    it.name = name;
    it.action = action;
    start_app(&it);
}

int eSystemConfirmStopStatus(void)
{
    /* 系统进入在未来时间里，无任务超时唤醒，可根据用户选择系统停止，或者系统定时唤醒(100ms) */
    // 1:Endless Sleep
    // 0:100 ms wakeup
    /* log_info("100ms wakeup"); */
    return 1;
}

__attribute__((used)) int *__errno()
{
    static int err;
    return &err;
}
