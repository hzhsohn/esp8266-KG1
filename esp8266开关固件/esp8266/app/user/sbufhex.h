#ifndef _SBUF_HEX_H_

#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <ip_addr.h>
#include <mem.h>
#include <driver/uart.h>
#include <driver/spi.h>

#ifdef  __cplusplus
extern "C" {
#endif

int ICACHE_FLASH_ATTR sbufDecode(char *src, char *hex);
//用完后要释放
char* ICACHE_FLASH_ATTR sbufEncode(const unsigned char *p_pBuff, int p_pBuffLen,int* dstLen);

#ifdef  __cplusplus
}
#endif


#define _SBUF_HEX_H_
#endif
