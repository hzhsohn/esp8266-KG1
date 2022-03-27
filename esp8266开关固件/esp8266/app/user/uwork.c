#include "uwork.h"
#include "json_do.h"

//作为STA模式的IP
Event_StaMode_Got_IP_t g_gotSTAIp={0};
//
extern TzhJsonDo g_jsdo;
//
//////////////////////////////////////////////////////////////////////////
int ICACHE_FLASH_ATTR uwork_set_station_config(char*SSID,char*wifipwd)
{
	// Wifi configuration 
	char ssid[32]={0}; 
	char password[64]={0}; 
	struct station_config stationConf={0}; 

	if(0==strlen(SSID))
	{
		//os_printf("uwork_set_ap_config SSID is NULL\n");
		return 1; 
	}

	 //Set  station mode 
    wifi_set_opmode(STATION_MODE);

	strcpy(ssid,SSID);
	strcpy(password,wifipwd);

	//need not mac address
	stationConf.bssid_set = 0; 
	//stationConf.bssid is mac addr to WDS

	//Set ap settings 
	os_memcpy(&stationConf.ssid, ssid, 32); 
	os_memcpy(&stationConf.password, password, 64);
	wifi_station_set_config(&stationConf);

	return 0;
}

/*
void ICACHE_FLASH_ATTR uwork_service_init()
{
	//os_printf("uwork_service_init\n");
}
*/

BOOL ICACHE_FLASH_ATTR uwork_main(TzhEEPRomUserFixedInfo* eepinfo)
{	
	if(0==os_strcmp(eepinfo->sta_ssid,""))
	{return FALSE;}
	uwork_set_station_config(eepinfo->sta_ssid,eepinfo->sta_passwd);

	//初始化服务由OTA检测完之后才打开
	//system_init_done_cb(uwork_service_init);	
	
	return TRUE;
}

