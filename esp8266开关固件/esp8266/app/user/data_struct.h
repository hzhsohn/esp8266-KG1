#ifndef __USER_DATA_STRUCT_H__

///////////////////////////
//wifi led状态灯
typedef enum _EzhLedMethod
{
	ezhLedMethodInit = 0 ,
	ezhLedMethodRouterConnectedOK = 1,
	ezhLedMethodWebConfig = 2,
	ezhLedMethodSmartConfig = 3,
	ezhLedMethodRouterConnecting = 4
}EzhLedMethod;

//是否成功连接到路由器
typedef enum _EzhRouterSTA
{
	ezhRouterSTAUnknow=0,
	ezhRouterSTAFail=1,
	ezhRouterSTASuccess=2
}EzhRouterSTA;

/*
例子:读取用户结构
TzhEEPRomUserFixedInfo userInfo;
spi_flash_read(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,
				(uint32*)&userInfo,
				sizeof(TzhEEPRomUserFixedInfo));

例子:读取配置结构
TzhEEPRomDevConfig config;
spi_flash_read(EEPROM_DATA_ADDR_DevConfig * SPI_FLASH_SEC_SIZE,
				(uint32*)&config,
				sizeof(TzhEEPRomDevConfig));
*/

///////////////////////////
//更新标识位置在 111
//EEPROM_DATA_ADDR_BIN_VERSION							111			//  固件版本信息
//模块信息
#define EEPROM_DATA_ADDR_TMP_SSID						46          //SSID缓存
#define EEPROM_DATA_ADDR_DevConfig						47          //出厂设置高级配置模式
#define EEPROM_DATA_ADDR_UserFix						48          //第一次用户配置的WIFI信息 
//WEB页面地址
#define EEPROM_DATA_ADDR_JSON							49			//  JSON的出厂配置

#define EEPROM_DATA_ADDR_Web0		50			//index		
#define EEPROM_DATA_ADDR_Web2		52			//info		
#define EEPROM_DATA_ADDR_Web3		54			//config	

//断电后恢复的状态
#define EEPROM_DATA_ADDR_DevStatus	56          //设备当前的开关状态

///////////////////////////
typedef enum _EzhWorkMode
{
	ezhWorkModeUnkonw		=0,
	ezhWorkModeWebConfig	=1,		//标准WEB页面配置连接路由
	ezhWorkModeSmartConfig	=2,     //一键配置连接路由
	ezhWorkModeUWork		=3,		//正常工作模式
}EzhWorkMode;

//模块配置config页面结构
//此部分硬件复位数据不会被清空
typedef struct _TzhEEPRomDevConfig
{
	//版本的校验
	int jsonCrc16;
	//出厂设置标识
	char factoryHxKongFlag[16];
	//-账号和绑定信息
	char devUUID[68];  //设备的唯一序列号
	
}TzhEEPRomDevConfig;

/*
硬件复位这结构数据会被清除
*/
typedef struct _TzhEEPRomUserFixedInfo
{
	EzhWorkMode work_mode;
	char sta_ssid[48]; //双模式用到桥接的SSID
	char sta_passwd[32]; //双模式桥接的密码
	//
	char dev_id[36]; //设备名称
	char mqtt_host[96];
	int mqtt_port;
	char mqtt_subscr[64];	//订阅标题
	char mqtt_publish[64];	//主题发布
	char mqtt_client_id[48];
	char mqtt_user[36];
	char mqtt_passwd[36];
	
	//设置类型,KG1 开关模式 ,TK1 延时开关
	char dev_flag[36];

}TzhEEPRomUserFixedInfo;

//---------------------------------------------------------
//硬件当前开关状态
typedef struct _TzhEEPRomDevStatus
{
	int kg1; //结构体字节一定要4字节对齐
}TzhEEPRomDevStatus;

#define __USER_DATA_STRUCT_H__
#endif

