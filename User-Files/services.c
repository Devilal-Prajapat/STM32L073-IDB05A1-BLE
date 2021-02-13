#include "services.h"
#include "hci.h"
#include "hci_le.h"
#include "bluenrg_gap.h"
#include "bluenrg_hal_aci.h"
#include "bluenrg_gap_aci.h"
#include "bluenrg_gatt_aci.h"
#include "hci_const.h"
#include "gpio.h"

const uint8_t led_service_uuid[] = {0x0a,0x75, 0xe7,0x80, 0x61, 0xa0, 0x4a, 0xea,0x99, 0xde, 0x97,0x6e, 0xed, 0xa1,0x5d, 0x05};
const uint8_t led_char_uuid[] = {0x0a,0x75, 0xe7,0x80, 0x61, 0xa0, 0x4a, 0xea,0x99, 0xde, 0x97,0x6e, 0xed, 0xa1,0x5d, 0x06};
uint16_t led_service_handle, led_char_handle;

const uint8_t time_service_uuid[] = {0x0a,0x75, 0xe7,0x80, 0x61, 0xa0, 0x4a, 0xea,0x99, 0xde, 0x97,0x6e, 0xed, 0xa1,0x5d, 0x07};
const uint8_t time_sec_char_uuid[] = {0x0a,0x75, 0xe7,0x80, 0x61, 0xa0, 0x4a, 0xea,0x99, 0xde, 0x97,0x6e, 0xed, 0xa1,0x5d, 0x08};
const uint8_t time_min_char_uuid[] = {0x0a,0x75, 0xe7,0x80, 0x61, 0xa0, 0x4a, 0xea,0x99, 0xde, 0x97,0x6e, 0xed, 0xa1,0x5d, 0x09};

uint16_t time_service_handle, time_sec_char_handle, time_min_char_handle;


tBleStatus Add_Led_Service(void)
{
	tBleStatus ret;
	ret = aci_gatt_add_serv(UUID_TYPE_128, led_service_uuid, PRIMARY_SERVICE, 0x07,&led_service_handle);
	
		/* 
	function proto for ref
	tBleStatus aci_gatt_add_char(uint16_t serviceHandle,
			     uint8_t charUuidType,
			     const uint8_t* charUuid, 
			     uint8_t charValueLen, 
			     uint8_t charProperties,
			     uint8_t secPermissions,
			     uint8_t gattEvtMask,
			     uint8_t encryKeySize,
			     uint8_t isVariable,
			     uint16_t* charHandle)  
	*/
	ret = aci_gatt_add_char(led_service_handle, 
										UUID_TYPE_128, 
										led_char_uuid, 
										4,
										CHAR_PROP_WRITE| CHAR_PROP_WRITE_WITHOUT_RESP, 
										ATTR_PERMISSION_NONE,
										GATT_NOTIFY_ATTRIBUTE_WRITE,
										16, 
										1, 
										&led_char_handle
										);
	return ret;

}

tBleStatus Add_Time_Service(void)
{
	tBleStatus ret;
	ret = aci_gatt_add_serv(UUID_TYPE_128, time_service_uuid, PRIMARY_SERVICE, 0x07,&time_service_handle);
	
	ret = aci_gatt_add_char(time_service_handle, 
										UUID_TYPE_128,
										time_sec_char_uuid, 
										4,
										CHAR_PROP_READ, 
										ATTR_PERMISSION_NONE, 
										0,
										16, 
										0, 
										&time_sec_char_handle);
		
	ret = aci_gatt_add_char(time_service_handle, 
										UUID_TYPE_128, 
										time_min_char_uuid, 
										4,
										CHAR_PROP_NOTIFY|CHAR_PROP_READ, 
										ATTR_PERMISSION_NONE, 
										0,
										16, 
										0, 
										&time_min_char_handle);
	return ret;
}


tBleStatus Seconds_Update(void)
{
	uint32_t  val;
	tBleStatus ret;

	/* Obtain system tick value in milliseconds, and convert it to seconds. */
	val = HAL_GetTick();
	val = val/1000;

	/* create a time[] array to pass as last argument of aci_gatt_update_char_value() API*/
	const uint8_t time[4] = {(val >> 24)&0xFF, (val >> 16)&0xFF, (val >> 8)&0xFF, (val)&0xFF};
	ret = aci_gatt_update_char_value(time_service_handle, time_sec_char_handle, 0, 4,	time);
	return ret;
}

uint32_t previousMinuteValue;

tBleStatus Minutes_Notify(void)
{
	 uint32_t val;
	 uint32_t minuteValue;
	 tBleStatus ret;
	 /* Obtain system tick value in milliseconds */
	 val = HAL_GetTick();
	/* update "Minutes characteristic" value iff it has changed w.r.t. previous
	 * "minute" value.
	 */

	 minuteValue = val/(60*1000);
	if(minuteValue != previousMinuteValue) 
	{
		/* memmorize this "minute" value for future usage */
		previousMinuteValue = minuteValue;

		/* create a time[] array to pass as last argument of aci_gatt_update_char_value() API*/
		const uint8_t time[4] = {(previousMinuteValue >> 24)&0xFF, (previousMinuteValue >> 16)&0xFF, (previousMinuteValue >> 8)&0xFF, (previousMinuteValue)&0xFF};

		ret = aci_gatt_update_char_value(time_service_handle, time_min_char_handle, 0, 4, time);
	}
	 return ret;
}

void Update_Time_Characteristics(void) 
{
	/* update "seconds and minutes characteristics" of time service */
	Seconds_Update();
	Minutes_Notify();
}


void Attribute_Modified_CB(uint16_t handle, uint8_t data_length, uint8_t * attr_data)
{
	if(handle == (led_char_handle + 1))
	{
		if(attr_data[0] == 1)
		{
			HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);
		}
		else if(attr_data[0] == 0)
		{
			HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_RESET);
		}
	
	}
}

void GAP_ConnectionComplete_Callback(uint8_t addr[6], uint16_t handle)
{

}

void GAP_DisconnectionComplete_Callback(void)
{

}


void user_notify(void *pdata)
{	
	hci_uart_pckt *hci_pkt = (hci_uart_pckt *)pdata;
	
	hci_event_pckt *event_pkt = (hci_event_pckt *)hci_pkt->data;
	
	if(hci_pkt->type != HCI_EVENT_PKT)
		return;
	
	switch(event_pkt->evt)
	{
		/* disconnect callback*/
		case EVT_DISCONN_COMPLETE:
		{
			GAP_DisconnectionComplete_Callback();
			break;	
		}
		
		case EVT_LE_META_EVENT:
		{
			evt_le_meta_event *evt = (void *)event_pkt->data;
			switch(evt->subevent)
			{
				case EVT_LE_CONN_COMPLETE:
				{
					evt_le_connection_complete *cc = (void *)evt->data;
					GAP_ConnectionComplete_Callback(cc->peer_bdaddr, cc->handle);
					break;
				}
			}
			break;
		}
		
		case EVT_VENDOR:
		{
			evt_blue_aci *blue_evt = (void *)event_pkt->data;
			switch(blue_evt->ecode)
			{
				case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED:
				{
					evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1 *)blue_evt->data;
					Attribute_Modified_CB(evt->attr_handle, evt->data_length, evt->att_data);
					break;
				}
			}
			break;
		}
	}
}

