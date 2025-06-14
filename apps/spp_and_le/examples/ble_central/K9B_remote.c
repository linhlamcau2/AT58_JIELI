/*
 * K9B_remote.c
 *
 *  Created on: Jul 31, 2024
 *      Author: HaoPC
 */
#include "K9B_remote.h"
#include "system/includes.h"
#include "system/debug.h"
#include "rd_light_common.h"
#include "rd_queue.h"
volatile sw_working_stt_t Sw_Woring_K9B_Val;
const uint32_t CODE_CHECK_FLASH = 0x55AA55AA;

volatile rd_Flash_K9BData_t rd_Flash_K9BData_Val = {0};

rd_queue_t queue_mess;
volatile message_t queue_message[MAX_QUEUE_SIZE];

/*--------------------------Remote K9B ---------------------------------*/

void rd_K9B_flash_save_default()
{
	memset((void *)&rd_Flash_K9BData_Val, 0x00, sizeof(rd_Flash_K9BData_Val));
	rd_Flash_K9BData_Val.Factory_Check = CODE_CHECK_FLASH;
	int ret2 = syscfg_write(CFG_VM_RD_K9B, &rd_Flash_K9BData_Val.Factory_Check, sizeof(rd_Flash_K9BData_t));
	// printf("flash K9B save default [write:%d]\n", ret2);
	// int ret = syscfg_read(CFG_VM_RD_K9B, &rd_Flash_K9BData_Val.Factory_Check, sizeof(rd_Flash_K9BData_Val));
}

void rd_K9B_flash_init(void)
{
	int ret_read = syscfg_read(CFG_VM_RD_K9B, &rd_Flash_K9BData_Val.Factory_Check, sizeof(rd_Flash_K9BData_Val));
	printf("flash K9B read [write:%d]", ret_read);

	if (CODE_CHECK_FLASH != rd_Flash_K9BData_Val.Factory_Check)
	{
		rd_K9B_flash_save_default();
	}

	for (int i = 0; i < MAX_NUM_K9ONOFF; i++)
	{
		printf("mac-%08X \n", rd_Flash_K9BData_Val.MacK9B[i]);
	}
}

/*
 * save Mac K9B to next array
 */
uint8_t rd_K9B_flash_save(uint32_t Mac_K9B_save)
{
	if (RD_K9B_SaveOnOff_yet(Mac_K9B_save) != -1)
	{
		printf("this MAC K9B saved before don't over save\n");
		return 0;
	}
	if (rd_Flash_K9BData_Val.total < MAX_NUM_K9ONOFF)
	{
		rd_Flash_K9BData_Val.MacK9B[rd_Flash_K9BData_Val.total++] = Mac_K9B_save;
	}
	else
	{
		for (int i = 1; i < MAX_NUM_K9ONOFF; i++)
		{
			rd_Flash_K9BData_Val.MacK9B[i - 1] = rd_Flash_K9BData_Val.MacK9B[i];
		}
		rd_Flash_K9BData_Val.MacK9B[MAX_NUM_K9ONOFF - 1] = Mac_K9B_save;
	}
	int ret2 = syscfg_write(CFG_VM_RD_K9B, &rd_Flash_K9BData_Val.Factory_Check, sizeof(rd_Flash_K9BData_Val));
	printf("flash K9B save [write:%d]\n", ret2);
	return 1;
}

uint8_t rd_K9B_flash_delete(uint32_t Mac_K9B_save)
{
	int check_saved = RD_K9B_SaveOnOff_yet(Mac_K9B_save);
	if (check_saved == -1)
	{
		printf("don't find this mac to delete \n");
		return 0;
	}
	for (int i = check_saved; i < MAX_NUM_K9ONOFF - 1; i++)
	{
		rd_Flash_K9BData_Val.MacK9B[i] = rd_Flash_K9BData_Val.MacK9B[i + 1];
	}
	rd_Flash_K9BData_Val.total--;
	int ret2 = syscfg_write(CFG_VM_RD_K9B, &rd_Flash_K9BData_Val.Factory_Check, sizeof(rd_Flash_K9BData_Val));
	printf("flash K9B save [write:%d]\n", ret2);
	return 1;
}

/*

** check mac and return aray location if this mac saved
return -1 when this MAC don't save yet
*/
static int RD_K9B_SaveOnOff_yet(uint32_t mac_check)
{
	for (int i = rd_Flash_K9BData_Val.total - 1; i >= 0; i--)
	{
		if (rd_Flash_K9BData_Val.MacK9B[i] == mac_check)
		{
			return i;
		}
	}
	return -1; // this key don't save yet
}

/**
 * delete data K9B onOff in this button
 * relay_index: 0-> relay_index in RD switch
 */
static uint16_t next_dim_level(uint16_t dim_level_last)
{
	switch (dim_level_last)
	{
	case DIM_LEVEL3:
		return DIM_LEVEL2;
		break;
	case DIM_LEVEL2:
		return DIM_LEVEL1;
	case DIM_LEVEL1:
		return DIM_LEVEL3;
	default:
		return DIM_LEVEL3;
		break;
	}
}
static uint16_t get_dim_level_by_counter(uint32_t counter)
{
	uint8_t counter_level = counter % 3;
	switch (counter_level)
	{
	case 0:
		return DIM_LEVEL1;
		break;
	case 1:
		return DIM_LEVEL2;
	case 2:
		return DIM_LEVEL3;
	default:
		return DIM_LEVEL3;
		break;
	}
}

static void rd_K9B_set_ctrl_V2(uint8_t button_id, uint32_t counter)
{
	static uint8_t init_flag = 0;
	static uint8_t type_but =0;
	static uint8_t button_id_last = 0;
	static uint8_t level_dim_set = DIM_LEVEL3;
	static uint8_t level_cct_set = CCT_LEVEL1;

	if(!init_flag)
	{
		init_flag = 1;
		uint8_t cct_flash = rd_flash_get_cct();
		if(cct_flash == CCT_LEVEL1)
		{
			button_id_last = 0b001;
		}
		else if(cct_flash == CCT_LEVEL2)
		{
			button_id_last = 0b010;
			
		}
		else if(cct_flash == CCT_LEVEL3)
		{
			button_id_last = 0b100;
		}
	}

	if ((0b100 == button_id) || (0b010 == button_id) || (0b001 == button_id))
	{ // single button press
		/*----------------- set cct by button id ------------------------------*/
		switch (button_id)
		{
		case 0b100:
			level_cct_set = CCT_LEVEL3;
			break;
		case 0b010:
			level_cct_set = CCT_LEVEL2;
			break;
		case 0b001:
			level_cct_set = CCT_LEVEL1;
			break;
		default:
			level_cct_set = CCT_LEVEL3;
			break;
		}
		/*----------------- set dim by counter of K9B mess------------------------------*/
		// if(button_id != button_id_last)	counter_keep ++;

		if(type_but == 1)
			level_dim_set = DIM_LEVEL3;
		else if (button_id == button_id_last)
			level_dim_set = get_dim_level_by_counter(counter);
		rd_light_set_dim_cct100(level_dim_set, level_cct_set);
		type_but = 0;
	}
	else
	{ // 2 or 3 button press
		// counter_keep++;
		type_but = 1;
		rd_light_set_dim100(DIM_LEVEL0);
	}
	printf("k9b set dim:%d - cct:%d", level_dim_set, level_cct_set);
	button_id_last = button_id;
}

void rd_init_queue_message(void)
{
	rd_initQueue(&queue_mess, MAX_QUEUE_SIZE, QUEUE_SIZE, queue_message);
}

void rd_send_message_to_queue(uint32_t macDevice, uint8_t key, uint32_t counter, uint8_t type)
{
	message_t message;
	message.macDevice = macDevice;
	message.key = key;
	message.counter = counter;
	message.type = type;

	if (rd_enqueue(&queue_mess, (void *)&message) == 0)
	{
		printf("enqueue success\n");
	}
	else
	{
		printf("enqueue fail\n");
	}
}

void add_k9b_handle(message_t message)
{
	printf("add k9b handle\n");
	rd_K9B_flash_save(message.macDevice);
	rd_set_blink();
}

void delete_k9b_handle(message_t message)
{
	printf("delete k9b handle\n");
	rd_K9B_flash_delete(message.macDevice);
	rd_light_set_dim100(DIM_LEVEL3);
}

void ctrl_k9b_handle(message_t message)
{
	printf("ctrl k9b handle\n");
	rd_K9B_set_ctrl_V2(message.key, message.counter);
}

typedef struct
{
	uint8_t type;
	handle_event_t handle_event;
} event_handle_t;

event_handle_t event_handle[] = {
	{MSG_ADD_K9B, add_k9b_handle},
	{MSG_DELETE_K9B, delete_k9b_handle},
	{MSG_CTRL_K9B, ctrl_k9b_handle},
	{MSG_NULL, NULL},
};

message_t message_last;
uint8_t count_add = 0;
uint8_t count_del = 0;
static uint32_t lastTick_addOrDel = 0;

uint8_t rd_filter_mess(message_t message)
{
	uint32_t mac = message.macDevice;
	uint8_t key = message.key;
	uint8_t type = message.type;

	if (mac != message_last.macDevice || (key != 0b110 && key != 0b011))
	{
		count_add = 0;
		count_del = 0;
	}

	int index = RD_K9B_SaveOnOff_yet(mac);
	if (index == -1)
	{
		if (sys_timer_get_ms() < 60 * 1000)
		{
			if ((3 == type) && (0b011 == key))
			{
				count_add++;
				if (count_add == NUM_PRESS_TO_PAIR)
				{
					lastTick_addOrDel = sys_timer_get_ms();
					count_add = 0;
					return MSG_ADD_K9B;
				}
			}
		}
	}
	else
	{
		if (sys_timer_get_ms() < 60 * 1000)
		{
			if ((3 == type) && (0b110 == key))
			{
				count_del++;
				if (count_del == NUM_PRESS_TO_DELETE)
				{
					lastTick_addOrDel = sys_timer_get_ms();
					count_del = 0;
					return MSG_DELETE_K9B;
				}
			}
		}

		if (sys_timer_get_ms() - lastTick_addOrDel >= 2000)
		{
			return MSG_CTRL_K9B;
		}
	}
	return MSG_NULL;
}

void rd_handle_queue_message()
{
	message_t message;
	if (rd_dequeue(&queue_mess, &message) == 0)
	{
		printf("dequeue success\n");
		uint8_t type = rd_filter_mess(message);
		for (int i = 0; event_handle[i].type != MSG_NULL; i++)
		{
			if (event_handle[i].type == type)
			{
				event_handle[i].handle_event(message);
				break;
			}
		}
		memcpy((void *)&message_last, (void *)&message, sizeof(message_t));
	}
	else
	{
		printf("dequeue fail\n");
	}
}