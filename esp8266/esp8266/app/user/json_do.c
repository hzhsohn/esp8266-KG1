#include "json_do.h"
#include "json/cJSON.h"
#include "hx-crc.h"

//////////////////////////////////////////////////////////////////////////
BOOL ICACHE_FLASH_ATTR useJsonData(TzhJsonDo *jd)
{
	char *buffer;
	int jsonlen=0;
	BOOL ret=FALSE;

	buffer=(char*)os_malloc(4096);
	memset(buffer,0,4096);
	spi_flash_read(EEPROM_DATA_ADDR_JSON * SPI_FLASH_SEC_SIZE,(uint32*)buffer,4094);
	for(jsonlen=0;jsonlen<4094;jsonlen++)
	{
		if(buffer[jsonlen]==0x00 || buffer[jsonlen]==0xFF)
		{ 
			buffer[jsonlen]=0;
			buffer[jsonlen+1]=0;
			break; 
		}
	}
	
	//调试默认输出
	//uart_init(BIT_RATE_9600, BIT_RATE_9600);
	////os_printf("-\n--------------------\n");
	////os_printf("useJsonData len=%d =%s\n",jsonlen,buffer);
	//////////////////////////////////
	////os_printf("----json_parse----\n");
	//解释
	cJSON *json;
	json=cJSON_Parse(buffer);
	if(json)
	{
		cJSON *jsonVal;

		jsonVal=cJSON_GetObjectItem(json,"uart_show_debug");
		if(jsonVal)
		{
			jd->uart_show_debug=jsonVal->valueint;
		}

		jsonVal=cJSON_GetObjectItem(json,"uart0_bit");
		if(jsonVal)
		{
			jd->uart0_bit=jsonVal->valueint;
			////os_printf("%d\n",jsonVal->valueint);
		}

		jsonVal=cJSON_GetObjectItem(json,"ap_ssid");
		if(jsonVal)
		{
			strncpy(jd->ap_ssid,jsonVal->valuestring,sizeof(jd->ap_ssid)-1);
			////os_printf("%s\n",jsonVal->valuestring);
		}
		
		jsonVal=cJSON_GetObjectItem(json,"ap_passwd");
		if(jsonVal)
		{
			strncpy(jd->ap_passwd,jsonVal->valuestring,sizeof(jd->ap_passwd)-1);
			////os_printf("%s\n",jsonVal->valuestring);
		}
		
		jsonVal=cJSON_GetObjectItem(json,"dev_flag");
		if(jsonVal)
		{
			strncpy(jd->dev_flag,jsonVal->valuestring,sizeof(jd->dev_flag)-1);
			////os_printf("%s\n",jsonVal->valuestring);
		}

		cJSON_Delete(json);

		ret=TRUE;
	}
	////os_printf("----json_parse----\n");
	//////////////////////////////////
	
	jd->jsonCrc16=hxCRC16(buffer,jsonlen);

	//////////////////////////////////
	if(buffer)
	{
		os_free(buffer);
		buffer=NULL;
	}
	return ret;
}

