#ifndef __FACTORY_H__

#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <driver/uart.h>
#include <driver/spi.h>
#include "data_struct.h"

/*****************************************************************************
 * FunctionName : web_sent_cb
 * Description  : data sent callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR web_send_cb(void *arg);

/******************************************************************************
 * FunctionName : web_recv_cb
 * Description  : receive callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR web_recv_cb(void *arg, char *pusrdata, unsigned short length);

/******************************************************************************
 * FunctionName : web_discon_cb
 * Description  : disconnect callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR web_discon_cb(void *arg);

/******************************************************************************
 * FunctionName : web_recon_cb
 * Description  : reconnect callback, error occured in TCP connection.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR web_recon_cb(void *arg, sint8 err);

/******************************************************************************
 * FunctionName : web_listen
 * Description  : TCP server listened a connection successfully
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR web_listen(void *arg);

/******************************************************************************
* FunctionName : user_tcpserver_init
* Description  : parameter initialize as a TCP server
* Parameters   : port -- server port
* Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR factory_tcpserver_init(uint32 port);

//factory main
void ICACHE_FLASH_ATTR factory_main(TzhEEPRomUserFixedInfo* user);

#define __FACTORY_H__
#endif

