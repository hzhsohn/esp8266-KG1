#ifndef __URLCODE_WORK_H__

#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <driver/uart.h>
#include <driver/spi.h>
#include <mem.h>

/***
×ª»»URL±àÂë
char p[1024];int k;
char szb[]="hahha hehehe";
urlencode(szb,strlen(szb),&k,p);
urldecode(p,k);
*/
void ICACHE_FLASH_ATTR urlencode(char const *s, int len, int *new_length,char *dst);
int ICACHE_FLASH_ATTR urldecode(char *str, int len);

int ICACHE_FLASH_ATTR replace_str(const char *pInput,const char *pSrc,const char *pDst,char *pOutput);


#define __URLCODE_WORK_H__
#endif

