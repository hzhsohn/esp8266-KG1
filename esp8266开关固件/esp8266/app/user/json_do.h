#ifndef __USER_JSON_DO_H__

#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <driver/uart.h>
#include <driver/spi.h>
#include <mem.h>
#include "data_struct.h"

typedef  struct _TzhJsonDo{	
	//�汾��У��
	int jsonCrc16;

	//��ʾ�������
	int uart_show_debug;
	//������
	int uart0_bit;
	
	//
	char ap_ssid[36];
	char ap_passwd[24];

}TzhJsonDo;

//��JSON�����ݽ��뵽ϵͳ��
BOOL ICACHE_FLASH_ATTR useJsonData(TzhJsonDo *jd);

#define __USER_JSON_DO_H__
#endif

