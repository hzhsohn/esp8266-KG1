/*****************************************************************
ϵͳ�߼���ϵͼ
Han.zhihong  2016

user_main.c-->|--��������
              |--���ڳ�ʼ��
              |--factoryAP.c��������AP����·����
              |--uwork.c��������
			  |--trans_realtime.c���ݽ����ӿ�

******************************************************************/

#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <driver/uart.h>
#include <driver/spi.h>
#include <driver/key.h>
#include "uwork.h"
#include "data_struct.h"
#include "mem.h"
#include "gpio.h"
#include "smartconfig.h"
#include "hxnet-protocol.h"
#include "json_do.h"

#include "trans_realtime.h"
#include "JsonErrAP.h"
#include "webPageAP.h"


///////////////////////////////////
//JSON��������Ϣ
TzhJsonDo g_jsdo;

///////////////////////////////////
//�ָ�����
LOCAL os_timer_t g_ResetLimitTimer; //�ϵ�ָ�����,�ϵ�5���ڲ�������Ӳ��
LOCAL bool g_isResetAllow=false;
void ICACHE_FLASH_ATTR freeUserSettingWeb();
void ICACHE_FLASH_ATTR freeFactorySetting();

//WIFI״̬
LOCAL os_timer_t g_WifiStateTimer;
void ICACHE_FLASH_ATTR wifiStateTimerHandler();
LOCAL bool g_WifiStateLed_b=false;
LOCAL int g_WifiStateLedDelay=0;
EzhWorkMode g_curWorkMode=ezhWorkModeUnkonw;//��ǰ����ģʽ

///////////////////////////////////
//����
void user_plug_init(void);
#define PLUG_KEY_NUM          1
#define PLUG_KEY_0_IO_MUX     PERIPHS_IO_MUX_GPIO0_U
#define PLUG_KEY_0_IO_NUM     0
#define PLUG_KEY_0_IO_FUNC    FUNC_GPIO0

#define GPIO4_IO_MUX     PERIPHS_IO_MUX_GPIO4_U
#define GPIO4_IO_NUM     4
#define GPIO4_IO_FUNC    FUNC_GPIO4

//GPIO12
#define GP12_IO_MUX     PERIPHS_IO_MUX_MTDI_U
#define GP12_IO_NUM     12
#define GP12_IO_FUNC    FUNC_GPIO12

//GPIO13ʹ��
#define GP13_IO_MUX     PERIPHS_IO_MUX_MTCK_U
#define GP13_IO_NUM     13
#define GP13_IO_FUNC    FUNC_GPIO13
//GPIO14ʹ��
#define GP14_IO_MUX     PERIPHS_IO_MUX_MTMS_U
#define GP14_IO_NUM     14
#define GP14_IO_FUNC    FUNC_GPIO14

//GPIO2 ���
bool gp2_isOn;
#define GP2_IO_MUX     PERIPHS_IO_MUX_GPIO2_U
#define GP2_IO_NUM     2
#define GP2_IO_FUNC    FUNC_GPIO2

//LED WIFI�źŵ�
#define WIFI_LED_IO_MUX     GPIO4_IO_MUX
#define WIFI_LED_IO_NUM     GPIO4_IO_NUM
#define WIFI_LED_IO_FUNC    GPIO4_IO_FUNC

//----------------------------------------------------------------------
//
LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[PLUG_KEY_NUM];

//////////////////////////////////////////////////////////////////////////////
void ICACHE_FLASH_ATTR user_plug_init(void);
LOCAL void ICACHE_FLASH_ATTR user_led_init(void);
LOCAL void ICACHE_FLASH_ATTR user_plug_short_press(void);
LOCAL void ICACHE_FLASH_ATTR user_plug_long_press(void);

//////////////////////////////////////////////////////////////////////////////
void ICACHE_FLASH_ATTR setWifiLed(EzhLedMethod mode);

void ICACHE_FLASH_ATTR setGP2_OnOff(bool b)
{
	gp2_isOn=b;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(GP2_IO_NUM), b);
}
//
LOCAL void ICACHE_FLASH_ATTR user_gpio_init(void)
{
	//--------------------------------------------------------------
	//GPIO2���,������ΪTTL������Կ�
    PIN_FUNC_SELECT(WIFI_LED_IO_MUX, WIFI_LED_IO_FUNC);
    setWifiLed(ezhLedMethodInit); //��ʼ��ʱ�ر�

	PIN_FUNC_SELECT(GP2_IO_MUX, GP2_IO_FUNC);
	setGP2_OnOff(true); //��ʼ����ƽ

	//
	//PIN_FUNC_SELECT(GP12_IO_MUX, GP12_IO_FUNC);
	//GPIO_OUTPUT_SET(GPIO_ID_PIN(GP12_IO_NUM), true); //��ʼ����ƽ
	//
	//PIN_FUNC_SELECT(GP13_IO_MUX, GP13_IO_FUNC);
	//GPIO_OUTPUT_SET(GPIO_ID_PIN(GP13_IO_NUM), true); //��ʼ����ƽ
	//
	//PIN_FUNC_SELECT(GP14_IO_MUX, GP14_IO_FUNC);
	//GPIO_OUTPUT_SET(GPIO_ID_PIN(GP14_IO_NUM), true); //��ʼ����ƽ
}
void ICACHE_FLASH_ATTR wifiStateTimerHandler()
{
	g_WifiStateLed_b=!g_WifiStateLed_b;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(WIFI_LED_IO_NUM), g_WifiStateLed_b);

	os_timer_disarm(&g_WifiStateTimer);
	os_timer_setfn(&g_WifiStateTimer, (os_timer_func_t *)wifiStateTimerHandler, NULL);
	os_timer_arm(&g_WifiStateTimer, g_WifiStateLedDelay, 0);
}

void ICACHE_FLASH_ATTR setWifiLed(EzhLedMethod mode)
{
	switch (mode)
	{
	case ezhLedMethodRouterConnectedOK:
		//LED��
		os_timer_disarm(&g_WifiStateTimer);
		GPIO_OUTPUT_SET(GPIO_ID_PIN(WIFI_LED_IO_NUM), false);
		break;
	case ezhLedMethodWebConfig:
		//������
		g_WifiStateLedDelay=600;
		os_timer_disarm(&g_WifiStateTimer);
		os_timer_setfn(&g_WifiStateTimer, (os_timer_func_t *)wifiStateTimerHandler, NULL);
		os_timer_arm(&g_WifiStateTimer, g_WifiStateLedDelay, 0);
		break;
	case ezhLedMethodSmartConfig:		
		//������
		g_WifiStateLedDelay=100;
		os_timer_disarm(&g_WifiStateTimer);
		os_timer_setfn(&g_WifiStateTimer, (os_timer_func_t *)wifiStateTimerHandler, NULL);
		os_timer_arm(&g_WifiStateTimer, g_WifiStateLedDelay, 0);
		break;
	case ezhLedMethodRouterConnecting:
		//LED��
		os_timer_disarm(&g_WifiStateTimer);
		GPIO_OUTPUT_SET(GPIO_ID_PIN(WIFI_LED_IO_NUM), true);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////
//GPIO0�����ʼ��Ϊ��ť
void ICACHE_FLASH_ATTR user_plug_init(void)
{
	////os_printf("gpio0 set key init\n");
    single_key[0] = key_init_single(PLUG_KEY_0_IO_NUM, PLUG_KEY_0_IO_MUX, PLUG_KEY_0_IO_FUNC,
                                    user_plug_long_press, user_plug_short_press);

    keys.key_num = PLUG_KEY_NUM;
    keys.single_key = single_key;
    key_init(&keys);
}
//GPIO0 �̰�
LOCAL void ICACHE_FLASH_ATTR user_plug_short_press(void)
{
	setGP2_OnOff(!gp2_isOn);
}
//GPIO0 ����
LOCAL void ICACHE_FLASH_ATTR user_plug_long_press(void)
{
	//os_printf("user_plug_long_press\n");
	if(g_isResetAllow)
	{
		//�ָ��û�����
		freeUserSettingWeb();
		//----------------------------------
		//����ģ��
		system_restore();
		system_restart();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void ICACHE_FLASH_ATTR resetLimitHandler()
{
	g_isResetAllow=true;
}

void user_init(void)
{
	//����JSON������
	BOOL bLoadJson=useJsonData(&g_jsdo);
	if(FALSE==bLoadJson)
	{
		uart_init(BIT_RATE_74880 , BIT_RATE_74880);
		os_printf("\n\n--------------------\n");
		os_printf("--load cfg.json files error.\n");
		os_printf("--------------------\n");
		JsonErrAP_init();
		return;
	}

	//��ʼ���������	
	if(g_jsdo.uart0_bit<=0)
	{
		uart_init(BIT_RATE_74880, BIT_RATE_74880);
	}
	else
	{
		uart_init(g_jsdo.uart0_bit, g_jsdo.uart0_bit);
	}

	///////////////////////////////////
	TzhEEPRomDevConfig cfgRom={0};
	TzhEEPRomUserFixedInfo fixRom={0};

	//printf init message 
	os_printf("-------------------------\n");
	os_printf("MQTT-HXK Firmware: v1.0 \n");
	os_printf("Core Version:%s\n",system_get_sdk_version());
	os_printf("Copyright:www.hx-kong.com\n\n");


	//�����ϵ����ƻָ�
	os_timer_disarm(&g_ResetLimitTimer);
	os_timer_setfn(&g_ResetLimitTimer, (os_timer_func_t *)resetLimitHandler, NULL);
	os_timer_arm(&g_ResetLimitTimer, 2000, 0);
	
	//��ȡ
	if(SPI_FLASH_RESULT_OK==spi_flash_read(EEPROM_DATA_ADDR_DevConfig * SPI_FLASH_SEC_SIZE,(uint32*)&cfgRom,sizeof(TzhEEPRomDevConfig))
		&&SPI_FLASH_RESULT_OK==spi_flash_read(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,(uint32*)&fixRom,sizeof(TzhEEPRomUserFixedInfo))
	)
	{
					char *eeprom_err="default All Setting\n";
					//��֤�̼�����
					if(cfgRom.jsonCrc16!=g_jsdo.jsonCrc16)	
					{
							//��֤���ݲ�����,�����������
							//�ָ���������
							os_printf(eeprom_err);
							freeFactorySetting(); //����Ϊ��������
							//----------------------------------
							//����ģ��
							system_restore();
							system_restart();
							return;
					}
				    //---------------------------------------------------------
					//��ӡ������Ϣ
					g_curWorkMode=fixRom.work_mode;
					if(ezhWorkModeUnkonw==fixRom.work_mode)
					{
						//����Ĭ�Ϲ���ģʽ
						g_curWorkMode=ezhWorkModeWebConfig;
					}
					//�ص�WIFI�¼�
					wifi_set_event_handler_cb(wifi_handle_event_cb);
					switch (g_curWorkMode)
					{
					case ezhWorkModeWebConfig: //WEB��������·��
						{
							char tmp_ssid[36]={0};
							char chipid[16]={0};//оƬID
							os_sprintf(chipid,"%02X",system_get_chip_id());
							os_sprintf(tmp_ssid,"%s%s",g_jsdo.ap_ssid,chipid);
							os_printf("method=ezhWorkModeWebConfig\n");
							os_printf("ap_ssid=%s\n",tmp_ssid);
							os_printf("devUUID=%s\n",cfgRom.devUUID);
							setWifiLed(ezhLedMethodWebConfig); //WIFI�źŵƳ���
							factory_main(&fixRom);
							os_printf("mcu-ready\n");//��ӡ��������������
						}
						break;
					case ezhWorkModeUWork://��������ģʽ
							os_printf("method=ezhWorkModeUWork\n");
							os_printf("sta_ssid=%s\n",fixRom.sta_ssid);
							os_printf("devUUID=%s\n",cfgRom.devUUID);
							os_printf("mqtt=%s:%d\n",fixRom.mqtt_host,fixRom.mqtt_port);
							os_printf("mqtt_subcr=%s\n",fixRom.mqtt_subscr);
							os_printf("mqtt_publish=%s\n",fixRom.mqtt_publish);
							setWifiLed(ezhLedMethodRouterConnecting); //WIFI�źŵƿ���˸.δ���ӵ�·��
							if(FALSE==uwork_main(&fixRom))
							{
								//�ָ���������
								freeUserSettingWeb();
							}
							os_printf("mcu-ready\n");//��ӡ��������������
						break;
					default:
							//��֤���ݲ�����,�����������
							//�ָ���������
							os_printf(eeprom_err);
							freeFactorySetting(); //����Ϊ��������
							//----------------------------------
							//����ģ��
							system_restore();
							system_restart();
							return;
						break;
					}
					//---------------------------------------------------------
					//���س�ʼ��
					user_plug_init();
					user_gpio_init();
					if(0==g_jsdo.uart_show_debug)
					{
						//�ر�ϵͳ��ӡ����,��ֹ�����ں���Ϣ
						system_set_os_print(0);
					}
	}
	else
	{
		//os_printf("eeprom flash read fail!!\n");
		os_delay_us(100);
	}

	////////////////////////////////////////////////////////
	//���
	sysTrans_RealtimeInit();
}

void ICACHE_FLASH_ATTR freeUserSettingWeb()
{
	//�û���������
	TzhEEPRomUserFixedInfo usrRom={0};
	spi_flash_read(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,
											(uint32*)&usrRom,
											sizeof(TzhEEPRomUserFixedInfo));
	usrRom.work_mode=ezhWorkModeWebConfig;
	spi_flash_erase_sector(EEPROM_DATA_ADDR_UserFix);
	spi_flash_write(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,(uint32*)&usrRom,sizeof(TzhEEPRomUserFixedInfo));
}

/*��������*/
void ICACHE_FLASH_ATTR freeFactorySetting()
{
	BOOL bLoadJson=useJsonData(&g_jsdo);
	if(FALSE==bLoadJson)
	{
		return;
	}
	//�߼���������//////////////////////////////////////////////////////////////
	TzhEEPRomDevConfig tmpCfgRom={0};
	//jsonCrc16
	tmpCfgRom.jsonCrc16=g_jsdo.jsonCrc16;
	
	//Ӳ����Ϣ
	char mac[10]={0};
	wifi_get_macaddr(SOFTAP_IF,mac);
	os_sprintf(tmpCfgRom.devUUID,"%02X-%02X%02X%02X%02X%02X%02X",system_get_chip_id(),
					mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	//
	spi_flash_erase_sector(EEPROM_DATA_ADDR_DevConfig);
	spi_flash_write(EEPROM_DATA_ADDR_DevConfig * SPI_FLASH_SEC_SIZE,(uint32*)&tmpCfgRom,sizeof(TzhEEPRomDevConfig));
	//
	//�û���������//////////////////////////////////////////////////////////////
	TzhEEPRomUserFixedInfo tmpFixRom={0};
	spi_flash_erase_sector(EEPROM_DATA_ADDR_UserFix);
	spi_flash_write(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,(uint32*)&tmpFixRom,sizeof(TzhEEPRomUserFixedInfo));
}
