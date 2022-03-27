#ifndef __USER_WORK_H__

#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <driver/uart.h>
#include <driver/spi.h>
#include "data_struct.h"

/******************************************************************************
 * FunctionName : uwork_set_xxx_config
 * Description  : set the router info which ESP8266 station will connect to 
 * Parameters   : none
 * Returns      : 
 0=ok
 not 0=fail
*******************************************************************************/
int ICACHE_FLASH_ATTR uwork_set_station_config(char*SSID,char*wifipwd);

//user work main
BOOL ICACHE_FLASH_ATTR uwork_main(TzhEEPRomUserFixedInfo* eepinfo);


#define __USER_WORK_H__
#endif

