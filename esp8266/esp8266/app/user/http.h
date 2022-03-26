#ifndef __HTTP_H__

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _TzhHttp
{
	char cacheBuf[2048];
	int cacheLen;
}TzhHttp;

//解析
int httpReslovURL(TzhHttp* http,char* recvbuf,int recvlen,char*dstURL);

//字符串获取参数的函数---------------------------------------
/*
    char *sd="qq=123&bb=66&cc";
    char dd[100];
    zhHttpGetParameter(sd,"bb",dd);
    printf("%s",dd);
*/
int httpGetParameter(const char*str,const char*parameter,char*value);

/*
获取文件名和参数
*/
void httpUrlSplit(const char*szUrl,char*dst_file,char* dst_parm);

/*
//生成响应
content 返回到浏览器内容
http_rspBuf 网络发送的内容	
jumpUrl 跳转的页面
*/
int httpResponeOK(const char* content,char*http_rspBuf);
int httpResponeNoFound(char*http_rspBuf);
int httpResponeJumpUrl(const char* jumpUrl,char*http_rspBuf);

#ifdef  __cplusplus
}
#endif

#define __HTTP_H__
#endif
