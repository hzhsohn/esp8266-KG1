#include "trans_realtime.h"
#include <ets_sys.h>
#include <c_types.h>
#include <mem.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <driver/uart.h>
#include <driver/spi.h>
#include "data_struct.h"
#include "hxnet-protocol.h"
#include "gpio.h"
#include "json_do.h"
#include "hxk_ota.h"
#include "mqtt/mqtt.h"
#include "json/cJSON.h"
#include "c_base64.h"
#include "sbufhex.h"

typedef struct _TzhNetCache_Recv{
	uchar cache[513];
	int len;
}TzhNetCache_Recv;

//////////////////////////////////////////////////////////////////
//当前的工作模式
extern EzhWorkMode g_curWorkMode;
//STA模式下的IP
extern Event_StaMode_Got_IP_t g_gotSTAIp;
//JSON的配置信息
extern TzhJsonDo g_jsdo;
//////////////////////////////////////////////////////////////////
//内部指令处理
TzhNetFrame_Cmd g_coreHxnetCmd={0};
uchar g_coreHxnetCmdOk;
///////////////////////////////////////////////////////////
//搜索返回的临时json和字符串信息
char* research_m_str=NULL;

//////////////////////////////////////////////////////////////////
//串口防止冲突变量
bool g_transUartIsUsed=false;

//////////////////////////////////////////////////////////////////
//配置WIFI路由连接状态
EzhRouterSTA g_isRouterSTA=ezhRouterSTAUnknow;
//开关状态
int g_kg1=0;

//MQTT的发布信息的ID,设备名称,设备唯一ID
LOCAL char g_dev_flag[36]={0};
LOCAL char g_tmpMQTTSubscr[48]={0};
LOCAL char g_tmpMQTTPublish[48]={0};
LOCAL char g_tmpDevName[38]={0};
LOCAL char g_tmpDevUUID[48]={0};
//////////////////////////////////////////////////////////////////
//功能函数
void ICACHE_FLASH_ATTR getUserIp();
void ICACHE_FLASH_ATTR getUserRSSI();
void ICACHE_FLASH_ATTR getChipID();
void ICACHE_FLASH_ATTR setDevname(char*dname);
void ICACHE_FLASH_ATTR getWorkMethod();
void ICACHE_FLASH_ATTR searchFrameData();

//开关操作
void ICACHE_FLASH_ATTR kgDevStatus();
void ICACHE_FLASH_ATTR kg_setOnOff(int b);
////////////////////////////////////////////////////////
//
MQTT_Client mqttClient;
//
void ICACHE_FLASH_ATTR mqttConnectedCb(uint32_t *args);
void ICACHE_FLASH_ATTR mqttDisconnectedCb(uint32_t *args);
void ICACHE_FLASH_ATTR mqttPublishedCb(uint32_t *args);
void ICACHE_FLASH_ATTR mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len);
void ICACHE_FLASH_ATTR mqtt_data_trans_to_mcu(const char*buf,int len);
//
void ICACHE_FLASH_ATTR mqttSendBuff(char*buf,int buflen);
void ICACHE_FLASH_ATTR mcu_proc(char*flag,int param_len,uchar* param);


void ICACHE_FLASH_ATTR mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	//os_printf("MQTT: Connected %s:%d\n",client->host,client->port);
	MQTT_Subscribe(client, "MSD/A", 0);
	MQTT_Subscribe(client, g_tmpMQTTSubscr, 0);
}

void ICACHE_FLASH_ATTR mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	//os_printf("MQTT: Disconnected\n");
}

void ICACHE_FLASH_ATTR mqttPublishedCb(uint32_t *args)
{
	//MQTT_Client* client = (MQTT_Client*)args;
	//os_printf("MQTT: Published\n");
}

void ICACHE_FLASH_ATTR mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	MQTT_Client* client = (MQTT_Client*)args;

	//os_printf("Receive topic: %s, data: %s \r\n", topic, dataBuf);
	
	//应用层数据处理
	if(0==os_memcmp(topic,"MSD/A",5))
	{
		if(data_len >= 4)
		{
			//搜索
			if(0==os_memcmp(data,"FMSD,",5))
			{
				searchFrameData();
			}
		}
	}
	else if(0==os_memcmp(topic,g_tmpMQTTSubscr,topic_len))
	{
		//只允许JSON传输到下位机
		if(data_len>0)
		{
			mqtt_data_trans_to_mcu(data,data_len);
		}
	}
}

///////////////////////////////////////////////////////////////
//重启
LOCAL os_timer_t g_trRestartTimer;
LOCAL os_timer_t g_trRecvSerialTimer;

//
void ICACHE_FLASH_ATTR sysTrans_RealtimeInit()
{
	TzhEEPRomDevConfig config;
	TzhEEPRomUserFixedInfo fixedInfo={0};
	spi_flash_read(EEPROM_DATA_ADDR_DevConfig * SPI_FLASH_SEC_SIZE,
					(uint32*)&config,
					sizeof(TzhEEPRomDevConfig));
	spi_flash_read(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,
					(uint32*)&fixedInfo,
					sizeof(TzhEEPRomUserFixedInfo));
	strcpy(g_tmpDevUUID,config.devUUID);
	strcpy(g_dev_flag,fixedInfo.dev_flag);
	strcpy(g_tmpDevName,fixedInfo.dev_id);
	strcpy(g_tmpMQTTSubscr,fixedInfo.mqtt_subscr);
	strcpy(g_tmpMQTTPublish,fixedInfo.mqtt_publish);
	//上电恢复状态
	TzhEEPRomDevStatus currentDevStatus={0};	
	spi_flash_read(EEPROM_DATA_ADDR_DevStatus * SPI_FLASH_SEC_SIZE,(uint32*)&currentDevStatus,sizeof(TzhEEPRomDevStatus));
	g_kg1=(currentDevStatus.kg1==0)?0:1;
	kgDevStatus();
	
}
void ICACHE_FLASH_ATTR trRestartHandler()
{
	//重启模块
	system_restore();
	system_restart();
}
void ICACHE_FLASH_ATTR trRestart()
{
	os_timer_disarm(&g_trRestartTimer);
	os_timer_setfn(&g_trRestartTimer, (os_timer_func_t *)trRestartHandler, NULL);
	os_timer_arm(&g_trRestartTimer, 1000, 0);
}

//////////////////////////////////////////////////////////////////
//将数据输到下位机
void ICACHE_FLASH_ATTR mqtt_data_trans_to_mcu(const char*buf,int len)
{	
	//
	//解释
	//
	char*debuf=(char*)os_malloc(len+1);
	int debaseLen=0;
	
	//
	os_printf("mqtt recv buff=");
	uart0_tx_buffer((uint8*)buf,(uint16)len);
	os_printf("\n");
	//
	memset(debuf,0,len+1);
	debaseLen=sbufDecode((char*)buf,debuf);
	if(debaseLen>0)
	{		
		uchar mqttCmdOk;
		//发送到BUFF
		hxNetGetFrame(debuf,debaseLen,&g_coreHxnetCmd,&mqttCmdOk);
		//处理指令
		if(mqttCmdOk)
		{			
			//os_printf("\nflag=%s\n",g_coreHxnetCmd.flag);
			mcu_proc(g_coreHxnetCmd.flag, g_coreHxnetCmd.parameter_len, g_coreHxnetCmd.parameter);
		}
	}
	os_free(debuf);
	
	//发送到透传
	
}


/////////////////////////////////////////////////////////////////
void ICACHE_FLASH_ATTR mqttSendBuff(char*buf,int buflen)
{
		int ba16len=0;
		char* ba16=sbufEncode(buf,buflen,&ba16len);
		//MQTT回传MSD服务信息
		MQTT_Publish(&mqttClient, g_tmpMQTTPublish, ba16,ba16len, 0, 0);
		os_printf("mqttSendBuff =%s\n",ba16);
}


void ICACHE_FLASH_ATTR kgDevStatus()
{
	uchar gen_buf[128]={0};
	int gen_len=0;
	char parm[1]={0};
	
	parm[0]=g_kg1;
	//os_printf("kgDevStatus g_kg1=%d   parm=%d \n",g_kg1,parm[1]);
	gen_len=hxNetCreateFrame("kg#a", 1,parm,true,gen_buf);
	mqttSendBuff(gen_buf,gen_len);
}

void ICACHE_FLASH_ATTR kg_setOnOff(int b)
{
	TzhEEPRomDevStatus currentDevStatus={0};
	currentDevStatus.kg1=b;
	g_kg1=b;
	//保存开关状态--
	spi_flash_erase_sector(EEPROM_DATA_ADDR_DevStatus);
	spi_flash_write(EEPROM_DATA_ADDR_DevStatus * SPI_FLASH_SEC_SIZE,(uint32*)&currentDevStatus,sizeof(TzhEEPRomDevStatus));
	//
	GPIO_OUTPUT_SET(GPIO_ID_PIN(GP2_IO_NUM), b);
	//os_printf("kg_setOnOff g_kg1=%d\n",g_kg1);
}

//////////////////////////////////////////////////////////////////
//上位机给MCU的逻辑处理
void ICACHE_FLASH_ATTR mcu_proc(char*flag,int param_len,uchar* param)
{
	if(0==strcmp(flag,"kg#0"))
	{
		kgDevStatus();
	}
	else if(0==strcmp(flag,"kg#1"))
	{
		//循环[uchar 通道号码0~通道数量n排列的状态] ::::::::::::: 批量通道开关
		kg_setOnOff(param[0]);
		kgDevStatus();
	}
	else if(0==strcmp(flag,"kg#2"))
	{
		//uchar 通道号码,uchar 开或关0_1 ::::::::::::: 单通道开关控制
		if(0==param[0])
		{
			kg_setOnOff(param[1]);
			kgDevStatus();
		}
	}
}
//数据来源信息
void ICACHE_FLASH_ATTR trans_uart_recv(char* data,int len)
{
	
}

//////////////////////////////////////////////////////////////////
//回复搜索硬件消息
void ICACHE_FLASH_ATTR searchFrameData()
{
	unsigned char * sendBuf;
	int sendLen=0;
	int dsrlen=0;
	unsigned char* gen_json;

	sendBuf=(unsigned char*)os_malloc(600);
	gen_json=(unsigned char*)os_malloc(500);

	sendBuf[0]=0x00;
	gen_json[0]=0x00;

	//回复标志
	sendBuf[sendLen]='R';
	sendLen++;
	sendBuf[sendLen]='M';
	sendLen++;
	sendBuf[sendLen]='S';
	sendLen++;
	sendBuf[sendLen]='D';
	sendLen++;
	
	//逗号
	sendBuf[sendLen]=',';
	sendLen++;

	//全球唯一标识
	dsrlen=strlen(g_tmpDevUUID);
	memcpy(&sendBuf[sendLen],g_tmpDevUUID,dsrlen);
	sendLen+=dsrlen;

	//逗号
	sendBuf[sendLen]=',';
	sendLen++;

	//硬件标识
	strcat(gen_json,"{\"f\":\"");
	strcat(gen_json,g_dev_flag);
	//硬件名称
	strcat(gen_json,"\",\"n\":\"");
	strcat(gen_json,g_tmpDevName);
	//订阅的主题
	strcat(gen_json,"\",\"s\":\"");
	strcat(gen_json,g_tmpMQTTSubscr);
	//发布的主题
	strcat(gen_json,"\",\"p\":\"");
	strcat(gen_json,g_tmpMQTTPublish);
	//
	strcat(gen_json,"\"}");

	//JSON参数
	dsrlen=strlen(gen_json)+1;
	memcpy(&sendBuf[sendLen],gen_json,dsrlen);
	sendLen+=dsrlen;

	//MQTT回传MSD服务信息
	MQTT_Publish(&mqttClient, "MSD/B", sendBuf,sendLen, 0, 0);
	
	os_free(sendBuf);
	os_free(gen_json);
}

void ICACHE_FLASH_ATTR getUserIp()
{
	unsigned char gen_buf[128]={0};
	int gen_len=0;
	char sendBuf[64]={0};
	char ip[20]={0};
	int sendLen=0,iplen=0;
	switch (g_curWorkMode)
	{
	case ezhWorkModeWebConfig: //在网页配置模式时点按
			os_strcpy(ip,"192.168.1.10");
			iplen=strlen(ip)+1;
			memcpy(&sendBuf[sendLen],ip,iplen);
			sendLen+=iplen;
			//
			os_strcpy(ip,"255.255.255.0");
			iplen=strlen(ip)+1;
			memcpy(&sendBuf[sendLen],ip,iplen);
			sendLen+=iplen;
			//
			os_strcpy(ip,"192.168.1.10");
			iplen=strlen(ip)+1;
			memcpy(&sendBuf[sendLen],ip,iplen);
			sendLen+=iplen;
			//
			gen_len=hxNetCreateFrame("#ip", sendLen,sendBuf,true,gen_buf);
			uart0_tx_buffer(gen_buf,gen_len);
		break;
	case ezhWorkModeUWork: //正常工作模式
			os_sprintf(ip,IPSTR,IP2STR(&g_gotSTAIp.ip));
			iplen=strlen(ip)+1;
			memcpy(&sendBuf[sendLen],ip,iplen);
			sendLen+=iplen;
			//
			os_sprintf(ip,IPSTR,IP2STR(&g_gotSTAIp.mask));
			iplen=strlen(ip)+1;
			memcpy(&sendBuf[sendLen],ip,iplen);
			sendLen+=iplen;
			//
			os_sprintf(ip,IPSTR,IP2STR(&g_gotSTAIp.gw));
			iplen=strlen(ip)+1;
			memcpy(&sendBuf[sendLen],ip,iplen);
			sendLen+=iplen;
			//
			gen_len=hxNetCreateFrame("#ip", sendLen,sendBuf,true,gen_buf);
			uart0_tx_buffer(gen_buf,gen_len);
		break;
	}	
}

void ICACHE_FLASH_ATTR getUserRSSI()
{
	//< 10 ：查询成功，返回信号强度
	//31 ：查询失败，返回错误码
	
	unsigned char gen_buf[64]={0};
	int gen_len=0;
	uchar rssi=0;
	
	rssi=wifi_station_get_rssi();
	gen_len=hxNetCreateFrame("#rssi", 1,&rssi,true,gen_buf);
	uart0_tx_buffer(gen_buf,gen_len);
}

//WIFI的回调事件
void ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt)
{
	//os_printf("---hzh-----------event %x\n", evt->event);
	switch (evt->event) 
	{
		case EVENT_STAMODE_CONNECTED:
			/*os_printf("--------------connect to ssid %s, channel %d\n",
			evt->event_info.connected.ssid,
			evt->event_info.connected.channel);*/
			MQTT_Disconnect(&mqttClient);
		break;
		case EVENT_STAMODE_DISCONNECTED:
			{
				/*os_printf("---------------disconnect from ssid %s, reason %d\n",
				evt->event_info.disconnected.ssid,
				evt->event_info.disconnected.reason);*/
				setWifiLed(ezhLedMethodRouterConnecting);
				//
				if(REASON_HANDSHAKE_TIMEOUT==evt->event_info.disconnected.reason || 
				   REASON_4WAY_HANDSHAKE_TIMEOUT==evt->event_info.disconnected.reason)
				{
					g_isRouterSTA=ezhRouterSTAFail;
				}
				MQTT_Disconnect(&mqttClient);
			}
		break;
		case EVENT_STAMODE_AUTHMODE_CHANGE:
					/*os_printf("---------------mode: %d -> %d\n",
					evt->event_info.auth_change.old_mode,
					evt->event_info.auth_change.new_mode);*/
		break;
		case EVENT_STAMODE_GOT_IP:
			{
					os_printf("STAMODE_GOT_IP---ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR"\n",
					IP2STR(&evt->event_info.got_ip.ip),
					IP2STR(&evt->event_info.got_ip.mask),
					IP2STR(&evt->event_info.got_ip.gw));
					memcpy(&g_gotSTAIp,&evt->event_info.got_ip,sizeof(Event_StaMode_Got_IP_t));
					setWifiLed(ezhLedMethodRouterConnectedOK);
					//让判断是否连接路由成功过,供检测WIFI密码使用
					g_isRouterSTA=ezhRouterSTASuccess;
					//
					//
					TzhEEPRomUserFixedInfo fixedInfo={0};
					spi_flash_read(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,
									(uint32*)&fixedInfo,
									sizeof(TzhEEPRomUserFixedInfo));
					if(ezhWorkModeUWork==fixedInfo.work_mode)
					{
						//os_printf("MQTT_InitConnection host=%s:%d\n",fixedInfo.mqtt_host, fixedInfo.mqtt_port);
						MQTT_InitConnection(&mqttClient, fixedInfo.mqtt_host, fixedInfo.mqtt_port, 0);
						if(strlen(fixedInfo.mqtt_client_id)>0)
						{
							MQTT_InitClient(&mqttClient, fixedInfo.mqtt_client_id , fixedInfo.mqtt_user , fixedInfo.mqtt_passwd , 120, 1);
						}
						else
						{
							MQTT_InitClient(&mqttClient, g_tmpDevUUID , fixedInfo.mqtt_user , fixedInfo.mqtt_passwd , 120, 1);
						}
						//
						//MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
						MQTT_OnConnected(&mqttClient, mqttConnectedCb);
						MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
						MQTT_OnPublished(&mqttClient, mqttPublishedCb);
						MQTT_OnData(&mqttClient, mqttDataCb);	
						MQTT_Connect(&mqttClient);
					}
			}
		break;
		case EVENT_SOFTAPMODE_STACONNECTED:
			/*os_printf("---------------station: " MACSTR "join, AID = %d\n",
			MAC2STR(evt->event_info.sta_connected.mac),
			evt->event_info.sta_connected.aid);*/
			MQTT_Disconnect(&mqttClient);
		break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
			/*os_printf("---------------station: " MACSTR "leave, AID = %d\n",
				MAC2STR(evt->event_info.sta_disconnected.mac),
				evt->event_info.sta_disconnected.aid);*/
			MQTT_Disconnect(&mqttClient);
		break;
		default:
		break;
	}
}

