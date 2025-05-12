/*
 * K9B_remote.h
 *
 *  Created on: Jul 31, 2024
 *      Author: HaoPC
 */

#ifndef VENDOR_RANG_DONG_K9B_REMOTE_H_
#define VENDOR_RANG_DONG_K9B_REMOTE_H_
#include <stdint.h>

#define TIME_PAIR_K9B_HC_MS      10000 // 10s
#define TIME_PAIR_K9B_LOCAL_MS   60000 // 60s
#define MAX_NUM_K9ONOFF          5
#define NUM_OF_ELEMENT 			 1

#define NUM_PRESS_TO_PAIR        5
#define NUM_PRESS_TO_DELETE		 5


#	define MAX_NUM_K9BHC				5
#	define MAX_NUM_K9ONOFF				5
#	define MAX_MUM_K9BPRESSTYPE			12 //Ver 2 7 // 3 button -> 7 type press 001-010-100-110-011-101-111

#	define K9B_RD_MESS_EN				0		// new format of RD

#define K9B_CTR_LOCAL_ONOFF				1	// Mode V1 toggle anf offall button 6
#define K9B_CTR_LOCAL_SCENE				2	// Mode V2 control as Scene offall button 6

#	define K9B_TYPE_CTR_LOCAL_DF		K9B_CTR_LOCAL_ONOFF
#	define K9B_BLE_CID_ATM				0x09E2 // atomosic CID ble
#	define K9B_BLE_CID_RD				0x2804


#define 	CLOCK_TIME_OUT_PRESS_K9B_MS	 500

typedef enum{
    NONE_FUN = 0x00,
    TOGGLE_FUN  = 0x01,
    OFF_FUN     = 0x02,
    ON_FUN      = 0x03
} K9B_fun_button_e;

typedef struct
{
	uint8_t Pair_K9BOnOff_Flag;
	uint8_t Button_K9BOnOff_Pair;
	uint8_t Pair_K9BHc_Flag;
	uint16_t Add_K9B_HCSet;			// HC set add for K9B remote
	uint32_t ClockTimeSetHc_ms;
	uint32_t Clock_BtK9BStartPress_ms[3];
	uint8_t  Bt_K9B_CountPress[3];
    uint32_t Clock_time_start_pair_onOff_ms; 
} sw_working_stt_t;

typedef struct
{
	uint32_t MacOld;
	uint32_t MacNow;
	uint8_t  Button_ID;
	uint8_t  ButtonPressCounter;
	uint32_t  ClockTimePress_ms;
} sw_press_K9BHC_t;

typedef struct{
	uint32_t countLastOfMacID[MAX_NUM_K9ONOFF];
} K9bOnOff_countLast_st;

typedef struct{
	uint32_t countLastOfMacID[MAX_NUM_K9BHC];
} K9bHc_countLast_st;


typedef struct
{
	uint32_t Factory_Check;
	uint32_t MacK9B[MAX_NUM_K9BHC];
	uint32_t arraySave_Next;
} rd_Flash_K9BData_t;
void rd_K9B_flash_init(void);




/*
 * uint8_t Button_ID  1->
 */
void RD_K9B_Pair_OnOffSetFlag(uint8_t Button_ID);
/*
 * return Button_ID 1->4 if Pair_Flag on,
 */
uint8_t RD_K9B_Pair_OnOffGetFlag(void);
void RD_K9B_Pair_OnOffClearFlag(void);
/*
 * uint8_t Button_ID 1-->4
 */
void RD_K9B_TimeOutScan(void);
void RD_K9B_SaveOnOff(uint32_t macDevice, uint8_t key);
void RD_Flash_DeleteAllK9BOnOff(uint8_t relay_index);
uint8_t RD_K9B_ScanOnOff(uint32_t macDevice, uint8_t key, uint32_t counter);
uint8_t RD_K9B_check_saveAndDelete(uint32_t macDevice, uint32_t counter, uint8_t type_k9b,uint8_t button_id);

/*----------------------------------------K9B and HC mode in Rang Dong smart home--------------------- */
/*
void RD_K9B_PairHCSet(uint8_t pairStt, uint16_t K9BAdd);
void RD_K9B_CheckScanK9BHc(uint32_t K9BMac_Buff, uint8_t Num_Of_Button,  signed char  rssi);
uint8_t RD_K9B_ScanPress2HC(uint32_t macDevice, uint8_t key, uint32_t par_signature);
*/
#endif /* VENDOR_RANG_DONG_K9B_REMOTE_H_ */
