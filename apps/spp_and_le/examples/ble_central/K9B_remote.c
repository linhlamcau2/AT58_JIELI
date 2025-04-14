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
volatile  sw_working_stt_t   Sw_Woring_K9B_Val;
volatile  sw_press_K9BHC_t   Sw_Press_K9BHC_Val;
const uint32_t CODE_CHECK_FLASH  = 0x55AA55AA;

volatile  rd_Flash_K9BData_t rd_Flash_K9BData_Val = {0};

/*--------------------------Remote K9B ---------------------------------*/

void rd_K9B_flash_save_default(){
	rd_Flash_K9BData_t rd_Flash_K9BData_Val_DF = {0};
	rd_Flash_K9BData_Val_DF.Factory_Check = CODE_CHECK_FLASH;
	int ret2 = syscfg_write(CFG_VM_RD_K9B, &rd_Flash_K9BData_Val_DF.Factory_Check, sizeof(rd_Flash_K9BData_Val_DF));
    printf("flash K9B save default [write:%d]\n", ret2);
	int ret = syscfg_read(CFG_VM_RD_K9B, &rd_Flash_K9BData_Val.Factory_Check, sizeof(rd_Flash_K9BData_Val));

}

void rd_K9B_flash_init(void){
	int ret_read = syscfg_read(CFG_VM_RD_K9B, &rd_Flash_K9BData_Val.Factory_Check, sizeof(rd_Flash_K9BData_Val));
    printf("flash K9B read [write:%d]", ret_read);

	if(CODE_CHECK_FLASH != rd_Flash_K9BData_Val.Factory_Check){
		rd_K9B_flash_save_default();
	}

	for(int i=0; i< MAX_NUM_K9ONOFF; i++){
		printf("mac-%08X \n",rd_Flash_K9BData_Val.MacK9B[i]);
	}

	
}

/*
* save Mac K9B to next array
*/
void rd_K9B_flash_save(uint32_t Mac_K9B_save){
	uint32_t num_off_array_save = MIN(rd_Flash_K9BData_Val.arraySave_Next, MAX_NUM_K9ONOFF-1);

	if(-1 != RD_K9B_SaveOnOff_yet(Mac_K9B_save)){
		printf("this MAC K9B saved before don't over save\n");
	}
	else
	{
		rd_Flash_K9BData_Val.MacK9B[num_off_array_save] = Mac_K9B_save;
		rd_Flash_K9BData_Val.arraySave_Next++;

		if(rd_Flash_K9BData_Val.arraySave_Next >= (MAX_NUM_K9ONOFF)){
			rd_Flash_K9BData_Val.arraySave_Next = 0;  // return 0 when save over load 
		}

		int ret2 = syscfg_write(CFG_VM_RD_K9B, &rd_Flash_K9BData_Val.Factory_Check, sizeof(rd_Flash_K9BData_Val));
		printf("flash K9B save [write:%d]\n", ret2);
	}

}

static void rd_K9B_flash_delete(uint32_t Mac_K9B_save){
	int check_saved = RD_K9B_SaveOnOff_yet(Mac_K9B_save);
	if(-1 == check_saved){
		printf("don't find this mac to delete \n");
	}
	else
	{
		uint32_t array_of_MAC_delete = check_saved; // ayyray in flash struct

		if(array_of_MAC_delete < (MAX_NUM_K9ONOFF-1)){ // not end array
			for(int i= array_of_MAC_delete; i<(MAX_NUM_K9ONOFF-1); i++){
				rd_Flash_K9BData_Val.MacK9B[i] = rd_Flash_K9BData_Val.MacK9B[i+1];
			}
			rd_Flash_K9BData_Val.MacK9B[MAX_NUM_K9ONOFF-1]  = 0x00000000;
			rd_Flash_K9BData_Val.arraySave_Next--;
		}
		else{
			rd_Flash_K9BData_Val.MacK9B[array_of_MAC_delete] = 0x00000000;
			rd_Flash_K9BData_Val.arraySave_Next--;
		}

		printf("new flash data: %d remote", rd_Flash_K9BData_Val.arraySave_Next);
		for(int i=0; i< MAX_NUM_K9ONOFF; i++){
			printf("MAC: %08x", rd_Flash_K9BData_Val.MacK9B[i]);
		}
		int ret2 = syscfg_write(CFG_VM_RD_K9B, &rd_Flash_K9BData_Val.Factory_Check, sizeof(rd_Flash_K9BData_Val));
		printf("flash K9B save [write:%d]\n", ret2);
	}

}

/*
 * uint8_t Button_ID  1-2
 */
void RD_K9B_Pair_OnOffSetFlag(uint8_t Button_ID){
	Sw_Woring_K9B_Val.Clock_time_start_pair_onOff_ms = sys_timer_get_ms();
	Sw_Woring_K9B_Val.Button_K9BOnOff_Pair = Button_ID;
	Sw_Woring_K9B_Val.Pair_K9BOnOff_Flag   = 1;
}

/*
 * uint8_t Button_ID 1-->5
 */
void RD_K9B_Pair_OnOffClearFlag(void){
	Sw_Woring_K9B_Val.Button_K9BOnOff_Pair = 0x00;
	Sw_Woring_K9B_Val.Pair_K9BOnOff_Flag   = 0x00;
}

/*
 * return Button_ID 1->5 if Pair_Flag on,
 */
uint8_t RD_K9B_Pair_OnOffGetFlag(void)
{
	return Sw_Woring_K9B_Val.Button_K9BOnOff_Pair;
}
static inline void RD_K9B_TimeoutScan_OnOff(void){
	if((0x00 != RD_K9B_Pair_OnOffGetFlag()) &&  (clock_time_exceed_ms(Sw_Woring_K9B_Val.Clock_time_start_pair_onOff_ms, TIME_PAIR_K9B_LOCAL_MS)))
	{
		printf("Time out Pair K9B -- \n");
		//RD_Todo: send mess
		RD_K9B_Pair_OnOffClearFlag();
	}
}


void RD_K9B_TimeOutScan(void){
	RD_K9B_TimeoutScan_OnOff();
	//RD_K9B_TimeOutScanK9BHC();
}

/*
** check mac and return aray location if this mac saved
return -1 when this MAC don't save yet
*/
static int RD_K9B_SaveOnOff_yet(uint32_t mac_check)
{

	for(int i=0; i< MAX_NUM_K9ONOFF; i++)
	{
		 uint32_t mac_buf = rd_Flash_K9BData_Val.MacK9B[i];
		// uint8_t  key_buf = sw_flash_data_config_val.K9B_Data.OnOff[switch_button - 1].K9B_BtKey[i];
		if( mac_buf == mac_check)
		{
			return i;
		}
	}
	return -1; // this key don't save yet
}
void RD_K9B_SaveOnOff(uint32_t macDevice, uint8_t key)
{
	uint8_t keyFillterStt =0;

	if( ((1 == key) || (2 == key) || (4 == key) || (8 == key)|| (16 == key) || (32 == key)|| (40 == key) ) )  // only single button
	{
		keyFillterStt = 1;
	//	key = key + 0x80; // save for action when press button rising
	}
	if(0x00 != RD_K9B_Pair_OnOffGetFlag() && (1 == keyFillterStt))
	{
		// if(RD_K9B_SaveOnOff_yet(macDevice, key, RD_K9B_Pair_OnOffGetFlag()) == 0) // this key don't save yet
		// {
		// 	printf("save K9b on off \n");
		// 	RD_Flash_SaveK9BOnOff(macDevice, key, RD_K9B_Pair_OnOffGetFlag()-1);
		// 	RD_K9B_Pair_OnOffClearFlag();
		// 	led_button_blink_set(0xff, 1, 0);
		// }
		// else
		// {
		// 	RD_K9B_Pair_OnOffClearFlag();
		// 	led_button_blink_set(0xff, 1, 0);

		// }
	}
	//Sw_Flash_Data_Val
}

void RD_K9B_check_saveAndDelete(uint32_t macDevice, uint32_t counter, uint8_t type_k9b, uint8_t button_id)
{
	static u32 macDevice_last = 0;
	static u32 counter_last = 0;
	static u8  button_id_last = 0;
	static u8  count_pair   = 0;
	static u8  count_clear   = 0;
	static u8  first_time_check_flag =0;

	// if(0 == first_time_check_flag) {// first time auto pass (macDevice == macDevice_last) 
	// 	first_time_check_flag =1;
	// 	macDevice_last = macDevice;
	// }
	if(macDevice != macDevice_last){ // press other than last remote will reset count pair and delete
		count_clear=0;
		count_pair=0;
	}
	if(RD_K9B_Pair_OnOffGetFlag()) {
		/*--------------------------------Save---------------------------------*/
		if( (sys_timer_get_ms() - Sw_Woring_K9B_Val.Clock_time_start_pair_onOff_ms) <= TIME_PAIR_K9B_LOCAL_MS){
			if((counter != counter_last) && (3 == type_k9b)  && (0b011 == button_id)){
				count_pair++;
				printf("count pair:%d -MAC:%08x \n",count_pair , macDevice);
				count_clear = 0x00;
				printf("count delete :%d -MAC:%08x \n",count_clear , macDevice);
				if(count_pair >= (NUM_PRESS_TO_PAIR)){
					printf("save-MAC:%08x \n", macDevice);
					rd_K9B_flash_save(macDevice);
					RD_K9B_Pair_OnOffClearFlag();
				}
			}
			else{

			}
		}


		/*-----------------------------Delete---------------------------------------*/
		if( (sys_timer_get_ms() - Sw_Woring_K9B_Val.Clock_time_start_pair_onOff_ms) <= TIME_PAIR_K9B_LOCAL_MS){
			if( (counter != counter_last) && (3 == type_k9b)  && (0b110 == button_id)){
				count_clear++;
				printf("count delete:%d -MAC:%08x \n",count_clear , macDevice);
				count_pair = 0x00;
				printf("count pair:%d -MAC:%08x \n",count_pair , macDevice);
				if(count_clear >= (NUM_PRESS_TO_PAIR)){

					printf("DELETE-MAC:%08x \n", macDevice);
					rd_K9B_flash_delete(macDevice);
					RD_K9B_Pair_OnOffClearFlag();
					rd_light_set_dim_cct100(DIM_LEVEL3, CCT_LEVEL1);
				}
			}
			else{

			}
		}
		
	}


	if((button_id != 0b110) && (button_id != 0b011)){  // press button other than 1 and 3 will resset count clear and count pair
		count_clear=0;
		count_pair=0;
	}
	macDevice_last = macDevice;
	counter_last =counter;
	button_id_last= button_id;
}

/**
 * delete data K9B onOff in this button
 * relay_index: 0-> relay_index in RD switch
 */
void RD_Flash_DeleteAllK9BOnOff(uint8_t relay_index)
{
	// sw_flash_data_config_val.K9B_Data.OnOff[relay_index].NumSaveNext =0;  // next ID array to save

	// for(int i=0; i < MAX_NUM_K9ONOFF; i++)
	// {
	// 	sw_flash_data_config_val.K9B_Data.OnOff[relay_index].MacK9B[i] = 0x00;
	// 	sw_flash_data_config_val.K9B_Data.OnOff[relay_index].K9B_BtKey[i] = 0x00;
	// }
	// flash_erase_sector(FLASH_ADDR_CONFIG);
	// flash_write_page(FLASH_ADDR_CONFIG, sizeof(sw_flash_data_config_val), (uint8_t *) &sw_flash_data_config_val.Factory_Check );
}
static uint16_t next_dim_level(uint16_t dim_level_last){
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
static uint16_t get_dim_level_by_counter(uint32_t counter){
	uint8_t counter_level = counter%3;
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
static void rd_K9B_set_ctrl(uint8_t button_id){
	static uint8_t button_id_last =0;
	static uint16_t level_dim_last = DIM_LEVEL3;
	static uint16_t level_cct_last = CCT_LEVEL3;
	uint16_t level_dim_set = DIM_LEVEL3;
	uint16_t level_cct_set = CCT_LEVEL3;

	if( (0b100 ==button_id) || (0b010 == button_id) || (0b001 == button_id)){  // single button press
		//  control dim/cct

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

		/*----------------- set dim by last button id and stt------------------------------*/
		if(button_id_last != button_id){ // keep dim and set cct
			if(level_dim_last == DIM_LEVEL0){
				level_dim_set = DIM_LEVEL3;
			}
			else{
				level_dim_set = level_dim_last;
			}
		}else{							// next level dim 
			level_dim_set = next_dim_level(level_dim_last);
		}
	}
	else{ // 2 or 3 button press
		// controll off
		level_dim_set = DIM_LEVEL0;
		level_cct_set = level_cct_last;
	}
	rd_light_set_dim_cct100(level_dim_set, level_cct_set);
	
	printf("k9b set dim:%d - cct:%d", level_dim_set, level_cct_set);
	level_dim_last = level_dim_set;
	level_cct_last = level_cct_set;
	
	button_id_last = button_id;
}

static void rd_K9B_set_ctrl_V2(uint8_t button_id, uint32_t counter){
		static uint8_t button_id_last =0;
	static uint16_t level_dim_last = DIM_LEVEL3;
	static uint16_t level_cct_last = CCT_LEVEL3;
	uint16_t level_dim_set = DIM_LEVEL3;
	uint16_t level_cct_set = CCT_LEVEL3;

	if( (0b100 ==button_id) || (0b010 == button_id) || (0b001 == button_id)){  // single button press
		//  control dim/cct

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
		level_dim_set = get_dim_level_by_counter(counter);
	}
	else{ // 2 or 3 button press
		// controll off
		level_dim_set = DIM_LEVEL0;
		level_cct_set = level_cct_last;
	}
	rd_light_set_dim_cct100(level_dim_set, level_cct_set);
	
	printf("k9b set dim:%d - cct:%d", level_dim_set, level_cct_set);
	level_dim_last = level_dim_set;
	level_cct_last = level_cct_set;
	
	button_id_last = button_id;
}
uint8_t RD_K9B_ScanOnOff(uint32_t macDevice, uint8_t key, uint32_t counter)
{
	uint8_t scanSttReturn = 0;
	static uint32_t clockTime_toggleButton_ms[MAX_NUM_K9ONOFF] ={0};
	static uint32_t counter_last = 0;
	static uint32_t clockTime_ThisMacLast[MAX_NUM_K9ONOFF] ={0};
	static K9bOnOff_countLast_st OnOff_CountLast = {0};
	uint8_t thisMac_send_overtime_flag =1;
#if 1
	if( counter != counter_last)
	{
		printf("Scan K9b onOff :\n");

		for(int i=0; i < MAX_NUM_K9ONOFF; i++)
		{
			if( (rd_Flash_K9BData_Val.MacK9B[i] == macDevice) )
			{
				printf("key pass \n");
				if((sys_timer_get_ms() - clockTime_ThisMacLast[i]) >= 1000){
					thisMac_send_overtime_flag =1;
				
				} 
				else{
					printf("last send too short \n");
				}
				clockTime_ThisMacLast[i] = sys_timer_get_ms();


				if(((sys_timer_get_ms() - clockTime_toggleButton_ms[i]) >= CLOCK_TIME_OUT_PRESS_K9B_MS) &&  (thisMac_send_overtime_flag))
				{
					if(OnOff_CountLast.countLastOfMacID[i] != counter){
						OnOff_CountLast.countLastOfMacID[i] = counter;
						clockTime_toggleButton_ms[i] = sys_timer_get_ms();
						printf("set control MAC:%08x - button: %d \n", macDevice, key);
						//rd_K9B_set_ctrl(key);
						rd_K9B_set_ctrl_V2(key, counter);
						counter_last = counter;
					}
				}
				break; // out off this element of switch
			}
		}
		
	}
#endif
	return scanSttReturn;
}

uint8_t check_mac_seved(uint32_t Mac_check){
	for(int i=0; i < MAX_NUM_K9ONOFF; i++)
	{
		if( (rd_Flash_K9BData_Val.MacK9B[i] == Mac_check) )
		{	
			return 1;
		}
	}
	return 0;
}
/*----------------------------------------K9B and HC mode in Rang Dong smart home--------------------- */
/*
void RD_K9B_TimeOutScanK9BHC(void)
{
	// pair time out
	if( (0x01 == Sw_Woring_K9B_Val.Pair_K9BHc_Flag) && (clock_time_exceed_ms(Sw_Woring_K9B_Val.ClockTimeSetHc_ms, TIME_PAIR_K9B_HC_MS)))
	{
		printf("time out ScanK9BHC\n");
		RD_Mess_ScanK9BHC_Rsp(GateWay_Add, 0x00, 0x00, 0x00 );
		RD_K9B_PairHCSet(0x00, 0x0000); //clear phase working
		led_button_blink_set(0xff,3, 500);
	}
}

void RD_K9B_PairHCSet(uint8_t pairStt, uint16_t K9BAdd)
{
	Sw_Woring_K9B_Val.Add_K9B_HCSet = K9BAdd;
	Sw_Woring_K9B_Val.Pair_K9BHc_Flag = pairStt;
	Sw_Woring_K9B_Val.ClockTimeSetHc_ms = clock_time_ms();
}

void RD_K9B_CheckScanK9BHc(uint32_t K9BMac_Buff, uint8_t Num_Of_Button, signed char rssi)
{
	if(0x01 == Sw_Woring_K9B_Val.Pair_K9BHc_Flag)
	{
		log_info("send HC mac: %08x, Type: %d, rssi: %d--%x--%x--%x Rssi: %d",K9BMac_Buff, Num_Of_Button, rssi);

		RD_Mess_ScanK9BHC_Rsp(GateWay_Add, K9BMac_Buff, Num_Of_Button, rssi);
		RD_K9B_PairHCSet(0x00, 0x0000); //clear phase working
		led_button_blink_set(0xff, 3, 300);
	}
}


uint8_t RD_K9B_ScanPress2HC(uint32_t macDevice, uint8_t key, uint32_t par_signature)
{
	uint8_t scanSttReturn = 0;
	static uint32_t clockTime_toggleButton_ms[MAX_NUM_K9BHC] ={0};
	static uint32_t signatureLast = 0;

	static uint8_t K9BButton_ID=0;
	uint8_t press_access =0;

	static K9bHc_countLast_st Hc_CountLast = {0};
	if( (key>0) && (key <0x80)) // only rising press.
	{
		K9BButton_ID = key;
		press_access = 1;
	}
	if( ((0x00 == par_signature) || (par_signature != signatureLast)) && (1 == press_access) )
	{
		for(int i=0; i< MAX_NUM_K9BHC; i++)
		{

			if(macDevice == sw_flash_data_config_val.K9B_Data.Scene[i].MacK9B)
			{
				scanSttReturn = 1;
				if(clock_time_exceed_ms(clockTime_toggleButton_ms[i], CLOCK_TIME_OUT_PRESS_K9B_MS))
				{
					if(Hc_CountLast.countLastOfMacID[i]  != par_signature){
						Hc_CountLast.countLastOfMacID[i] = par_signature;
						clockTime_toggleButton_ms[i] = clock_time_ms();
						uint16_t K9BHC_Add = sw_flash_data_config_val.K9B_Data.Scene[i].AddK9B;
						uint16_t SceneID		= 0x0000;

						int ButtonPos_Buff = RD_CheckButtonPosK9BHC(sw_flash_data_config_val.K9B_Data.Scene[i].Button_ID, key);
						if(ButtonPos_Buff != -1) SceneID = sw_flash_data_config_val.K9B_Data.Scene[i].Scene_ID_OnePress[ButtonPos_Buff];


						RD_MessK9BHc_Press(K9BHC_Add, K9BButton_ID, 1, SceneID);

						if(0x0000 != SceneID) RD_Call_Scene(SceneID, (uint8_t) (K9BButton_ID + SceneID));

						signatureLast = par_signature;
						log_info("send K9B HC: 0x%08x, button: %d, Scene: 0x%04x",macDevice, K9BButton_ID, SceneID);
					}
				}
			}
		}
	}


	for(int i=0; i< MAX_NUM_K9BHC; i++)
	{
		if(macDevice == sw_flash_data_config_val.K9B_Data.Scene[i].MacK9B)
		{
			scanSttReturn = 1;
		}
	}
	return scanSttReturn;

}


*/