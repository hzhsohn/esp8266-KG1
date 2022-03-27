#ifndef __HTTP_H__

#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <driver/uart.h>
#include <driver/spi.h>
#include "urlcode.h"

#ifdef  __cplusplus
extern "C" {
#endif

//发回给客户端的数据链表结构
typedef struct _TzhHttpServResponeData  
{
	char sendbuf[500]; //发送的数据.如果大于500字节.会被分割后再发送
	short sendlen;    //发送的长度
	struct _TzhHttpServResponeData *_next; //下一个节点的指针
}TzhHttpServResponeData;

//单个连接的数据结构
typedef struct _TzhHttpServ
{
	char cacheBuf[3000];
	int cacheLen;
	int listCount;
	TzhHttpServResponeData* sendDataList;
	TzhHttpServResponeData* pSendDataList_last;//listSendData链表最后一个元素
}TzhHttpServ;


/*
获取文件名和参数
*/
void ICACHE_FLASH_ATTR urlSplit(const char*szUrl,char*dst_file,char* dst_parm);


//字符串获取参数的函数---------------------------------------
/*
    char *sd="qq=123&bb=66&cc";
    char dd[100];
    zhHttpGetParameter(sd,"bb",dd);
    printf("%s",dd);
*/
int ICACHE_FLASH_ATTR urlGetParameter(const char*str,const char*parameter,char*value);


//--------------------------------------------------------------------------------------------------
//服务器处理函数

/*
//解析
返回 =0 成功
     非0 失败
*/
int ICACHE_FLASH_ATTR httpServReslovURL(TzhHttpServ* serv,char* recvbuf,int recvlen,char*dstURL);

/*
//生成响应
content 返回到浏览器内容
http_rspBuf 网络发送的内容	
jumpUrl 跳转的页面
*/
void ICACHE_FLASH_ATTR httpServResponeOK(TzhHttpServ* serv,const char* content,int content_len);
void ICACHE_FLASH_ATTR httpServResponeNoFound(TzhHttpServ* serv);
void ICACHE_FLASH_ATTR httpServResponeJumpUrl(TzhHttpServ* serv,const char* jumpUrl);

//发送回调时调用
TzhHttpServResponeData* ICACHE_FLASH_ATTR httpServFristData(TzhHttpServ* serv);
TzhHttpServResponeData* ICACHE_FLASH_ATTR httpServCurrentResumeData(TzhHttpServ* serv);


#ifdef  __cplusplus
}
#endif

#define __HTTP_H__
#endif
