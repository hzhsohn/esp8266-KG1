#ifndef __USER_DATA_STRUCT_H__

///////////////////////////
//wifi led״̬��
typedef enum _EzhLedMethod
{
	ezhLedMethodInit = 0 ,
	ezhLedMethodRouterConnectedOK = 1,
	ezhLedMethodWebConfig = 2,
	ezhLedMethodSmartConfig = 3,
	ezhLedMethodRouterConnecting = 4
}EzhLedMethod;

//�Ƿ�ɹ����ӵ�·����
typedef enum _EzhRouterSTA
{
	ezhRouterSTAUnknow=0,
	ezhRouterSTAFail=1,
	ezhRouterSTASuccess=2
}EzhRouterSTA;

/*
����:��ȡ�û��ṹ
TzhEEPRomUserFixedInfo userInfo;
spi_flash_read(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,
				(uint32*)&userInfo,
				sizeof(TzhEEPRomUserFixedInfo));

����:��ȡ���ýṹ
TzhEEPRomDevConfig config;
spi_flash_read(EEPROM_DATA_ADDR_DevConfig * SPI_FLASH_SEC_SIZE,
				(uint32*)&config,
				sizeof(TzhEEPRomDevConfig));
*/

///////////////////////////
//���±�ʶλ���� 111
//EEPROM_DATA_ADDR_BIN_VERSION							111			//  �̼��汾��Ϣ
//ģ����Ϣ
#define EEPROM_DATA_ADDR_TMP_SSID						46          //SSID����
#define EEPROM_DATA_ADDR_DevConfig						47          //�������ø߼�����ģʽ
#define EEPROM_DATA_ADDR_UserFix						48          //��һ���û����õ�WIFI��Ϣ 
//WEBҳ���ַ
#define EEPROM_DATA_ADDR_JSON							49			//  JSON�ĳ�������

#define EEPROM_DATA_ADDR_Web0		50			//index		
#define EEPROM_DATA_ADDR_Web2		52			//info		
#define EEPROM_DATA_ADDR_Web3		54			//config	

//�ϵ��ָ���״̬
#define EEPROM_DATA_ADDR_DevStatus	56          //�豸��ǰ�Ŀ���״̬

///////////////////////////
typedef enum _EzhWorkMode
{
	ezhWorkModeUnkonw		=0,
	ezhWorkModeWebConfig	=1,		//��׼WEBҳ����������·��
	ezhWorkModeSmartConfig	=2,     //һ����������·��
	ezhWorkModeUWork		=3,		//��������ģʽ
}EzhWorkMode;

//ģ������configҳ��ṹ
//�˲���Ӳ����λ���ݲ��ᱻ���
typedef struct _TzhEEPRomDevConfig
{
	//�汾��У��
	int jsonCrc16;
	//�������ñ�ʶ
	char factoryHxKongFlag[16];
	//-�˺źͰ���Ϣ
	char devUUID[68];  //�豸��Ψһ���к�
	
}TzhEEPRomDevConfig;

/*
Ӳ����λ��ṹ���ݻᱻ���
*/
typedef struct _TzhEEPRomUserFixedInfo
{
	EzhWorkMode work_mode;
	char sta_ssid[48]; //˫ģʽ�õ��Žӵ�SSID
	char sta_passwd[32]; //˫ģʽ�Žӵ�����
	//
	char dev_id[36]; //�豸����
	char mqtt_host[96];
	int mqtt_port;
	char mqtt_subscr[64];	//���ı���
	char mqtt_publish[64];	//���ⷢ��
	char mqtt_client_id[48];
	char mqtt_user[36];
	char mqtt_passwd[36];
	
	//��������,KG1 ����ģʽ ,TK1 ��ʱ����
	char dev_flag[36];

}TzhEEPRomUserFixedInfo;

//---------------------------------------------------------
//Ӳ����ǰ����״̬
typedef struct _TzhEEPRomDevStatus
{
	int kg1; //�ṹ���ֽ�һ��Ҫ4�ֽڶ���
}TzhEEPRomDevStatus;

#define __USER_DATA_STRUCT_H__
#endif

