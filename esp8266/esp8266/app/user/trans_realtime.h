#ifndef __USER_TRANS_REALTIME_H__

#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <ip_addr.h>
#include <mem.h>
#include <driver/uart.h>
#include <driver/spi.h>
#include "data_struct.h"

void ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt);
void ICACHE_FLASH_ATTR trans_yun_keep_timer_cb();
void ICACHE_FLASH_ATTR trans_getNetTimeRecv(char* data,int len);
void ICACHE_FLASH_ATTR sysTrans_RealtimeInit();

#define __USER_TRANS_REALTIME_H__
#endif

