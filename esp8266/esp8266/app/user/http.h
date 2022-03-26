#ifndef __HTTP_H__

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _TzhHttp
{
	char cacheBuf[2048];
	int cacheLen;
}TzhHttp;

//����
int httpReslovURL(TzhHttp* http,char* recvbuf,int recvlen,char*dstURL);

//�ַ�����ȡ�����ĺ���---------------------------------------
/*
    char *sd="qq=123&bb=66&cc";
    char dd[100];
    zhHttpGetParameter(sd,"bb",dd);
    printf("%s",dd);
*/
int httpGetParameter(const char*str,const char*parameter,char*value);

/*
��ȡ�ļ����Ͳ���
*/
void httpUrlSplit(const char*szUrl,char*dst_file,char* dst_parm);

/*
//������Ӧ
content ���ص����������
http_rspBuf ���緢�͵�����	
jumpUrl ��ת��ҳ��
*/
int httpResponeOK(const char* content,char*http_rspBuf);
int httpResponeNoFound(char*http_rspBuf);
int httpResponeJumpUrl(const char* jumpUrl,char*http_rspBuf);

#ifdef  __cplusplus
}
#endif

#define __HTTP_H__
#endif
