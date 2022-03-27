/*

han.zhihong
连接到OTA服务器里面更新固件

*/

#ifndef __HXK_SERVICE_TCP_OTA_CLIENT_H__

#include <osapi.h>
#include <user_interface.h>

#define EEPROM_DATA_ADDR_BIN_VERSION					111			//  固件版本信息

//
void ICACHE_FLASH_ATTR hxkOTAConnectServer(char*user1_url,char*user2_url);

#define __HXK_SERVICE_TCP_OTA_CLIENT_H__
#endif

