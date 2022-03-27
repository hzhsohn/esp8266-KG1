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

//���ظ��ͻ��˵���������ṹ
typedef struct _TzhHttpServResponeData  
{
	char sendbuf[500]; //���͵�����.�������500�ֽ�.�ᱻ�ָ���ٷ���
	short sendlen;    //���͵ĳ���
	struct _TzhHttpServResponeData *_next; //��һ���ڵ��ָ��
}TzhHttpServResponeData;

//�������ӵ����ݽṹ
typedef struct _TzhHttpServ
{
	char cacheBuf[3000];
	int cacheLen;
	int listCount;
	TzhHttpServResponeData* sendDataList;
	TzhHttpServResponeData* pSendDataList_last;//listSendData�������һ��Ԫ��
}TzhHttpServ;


/*
��ȡ�ļ����Ͳ���
*/
void ICACHE_FLASH_ATTR urlSplit(const char*szUrl,char*dst_file,char* dst_parm);


//�ַ�����ȡ�����ĺ���---------------------------------------
/*
    char *sd="qq=123&bb=66&cc";
    char dd[100];
    zhHttpGetParameter(sd,"bb",dd);
    printf("%s",dd);
*/
int ICACHE_FLASH_ATTR urlGetParameter(const char*str,const char*parameter,char*value);


//--------------------------------------------------------------------------------------------------
//������������

/*
//����
���� =0 �ɹ�
     ��0 ʧ��
*/
int ICACHE_FLASH_ATTR httpServReslovURL(TzhHttpServ* serv,char* recvbuf,int recvlen,char*dstURL);

/*
//������Ӧ
content ���ص����������
http_rspBuf ���緢�͵�����	
jumpUrl ��ת��ҳ��
*/
void ICACHE_FLASH_ATTR httpServResponeOK(TzhHttpServ* serv,const char* content,int content_len);
void ICACHE_FLASH_ATTR httpServResponeNoFound(TzhHttpServ* serv);
void ICACHE_FLASH_ATTR httpServResponeJumpUrl(TzhHttpServ* serv,const char* jumpUrl);

//���ͻص�ʱ����
TzhHttpServResponeData* ICACHE_FLASH_ATTR httpServFristData(TzhHttpServ* serv);
TzhHttpServResponeData* ICACHE_FLASH_ATTR httpServCurrentResumeData(TzhHttpServ* serv);


#ifdef  __cplusplus
}
#endif

#define __HTTP_H__
#endif
