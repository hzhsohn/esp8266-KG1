#include "http.h"
#include <mem.h>
#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>

//查找HTTP协议头尾
char*getbegnstr(char*buf);
char*rnrnstr(char*buf);

// 增加了一个end参数，这样不必提取出字符串后，再做剔除处理
void zhMatchString(const char* src, const char* pattern, char end,char*dstBuf);

char*getbegnstr(char*buf)
{
	char *p;
	p=buf;
	while(*p!=0)
	{
		if(*p=='G' || *p=='g')
		if(*(p+1)=='E' || *(p+1)=='e')
		if(*(p+2)=='T' || *(p+2)=='t')
		if(*(p+3)==' ')
			return p;
		p++;
	}
	return NULL;
}
char*rnrnstr(char*buf)
{
	char *p;
	p=buf;
	while(*p!=0)
	{
		if(*p==0x0D)
		if(*(p+1)==0x0A)
		if(*(p+2)==0x0D)
		if(*(p+3)==0x0A)
			return p;

		p++;
	}
	return NULL;
}

// 增加了一个end参数，这样不必提取出字符串后，再做剔除处理
void zhMatchString(const char* src, const char* pattern, char end,char*dstBuf)
{
		int src_len,ptn_len;
		unsigned short b=0, p=0, i=0;
		char c;
		char *ret_p;
        src_len = strlen(src); 
        ptn_len = strlen(pattern);
        
        for(i=0; i<src_len; i++){
                c = src[i];
                if(p==ptn_len){ // p==ptn_len 表示正在匹配中
                        if(c=='\r' || c=='\n'  || (end !='\0' && c==end) ) p++; // 匹配结束
                        else dstBuf[b++]=c; // 匹配到的字符 
                }else if(p<ptn_len){ // 为达到匹配要求
                        if(c==pattern[p]) p++;
                        else p=0;
                }
        }         
        dstBuf[b]=0;
}

//字符串获取参数的函数---------------------------------------
/*
    char *sd="qq=123&bb=66&cc";
    char dd[100];
    httpGetParameter(sd,"bb",dd);
    printf("%s",dd);

	返回值
	1成功
	0失败
*/
int httpGetParameter(const char*str,const char*parameter,char*value)
{
 //format is "a=123&b=456"
 #define SPLIT_1         "&"
 #define SPLIT_2         "="
 int bRet;
 char *pSplit,*pSplit2;
 char *pPara;
 char *p1,*p2;
 char *pszStr;
 int nStrLen;
 
 bRet=0;
 nStrLen=strlen(str);
 
 if (0==nStrLen) {
  return bRet;
 }
 
 pszStr=(char*)os_malloc(nStrLen+1);
 os_memset(pszStr, 0, nStrLen+1);
 strcpy(pszStr,str);
 
 pPara=strtok_r(pszStr,SPLIT_1, &pSplit);
 
 do{
  p1=strtok_r(pPara, SPLIT_2, &pSplit2);
  p2=strtok_r(NULL, SPLIT_2, &pSplit2);
  if (0==strcmp(parameter, p1)) {
   if (p2) {
    strcpy(value, p2);
    bRet=1;
   }
   break;
  }
 }while ((pPara=strtok_r(NULL,SPLIT_1, &pSplit)));
 os_free(pszStr);
 pszStr=NULL;
 return bRet;
}


/*

返回值
	1成功
	0失败

char dst_host[512];
int dst_port;
char dst_file[512];
char dst_pram[2048];

*/
void httpUrlSplit(const char*szUrl,char*dst_file,char* dst_parm)
{
		int bufsize=0; 
		int i=0;
 
		i=0;
		 
		//获取文件路径 
		bufsize=0;
		while(szUrl[i]!='?' && szUrl[i]!=0) {
		dst_file[bufsize]=szUrl[i];
		bufsize++;
		i++;
		}
		dst_file[bufsize]=0;
 
		//获取问号后面的参数
		bufsize=0;
		i++;
		while(szUrl[i]!=0)
		{
		dst_parm[bufsize]=szUrl[i];
		bufsize++;
		i++;
		}
		dst_parm[bufsize]=0;
}

/*

	返回值:
	=0 成功解析
	=1 未解析到
*/
int httpReslovURL(TzhHttp* http,char* recvbuf,int recvlen,char*dstURL)
{
	int tmp;
	tmp=http->cacheLen+recvlen;
	if(tmp<sizeof(http->cacheBuf))
	{
		char *ppheader;
		char *pheadstr;
		char *pstr;

		os_memcpy(&http->cacheBuf[http->cacheLen],recvbuf,recvlen);
		http->cacheLen=tmp;

		//获取开头
		ppheader=http->cacheBuf;
		pheadstr=getbegnstr(http->cacheBuf);

		//获取结尾		
		pstr=(char *)rnrnstr(http->cacheBuf);
		if(NULL==pstr)
		{
			return 0;
		}
		pstr+=4;

		//获取GET
		zhMatchString(pheadstr, "GET ", 0x20,dstURL); // 提取空格之前

		//删除没用的数据
		tmp=pstr-ppheader;
		os_memcpy(http->cacheBuf,&http->cacheBuf[tmp],tmp);
		http->cacheLen-=tmp;
		return 0;
	}
	http->cacheLen=0;
	return 1;
}

//生成响应
int httpResponeOK(const char* content,char*http_rspBuf)
{
	//////////////////////////////////////////
	//组建发送回去HTTP的内容
	char http_rsp[]="HTTP/1.1 200 OK\r\n\
Server：hx-httpd (hx-mcu)\r\n\
Content-Length：%d\r\n\
Content-Type: text/html\r\n\
Keep-Alive: timeout=5, max=100\r\n\
Connection：close\r\n\r\n\
%s";
	/*200响应成功*/
	os_sprintf(http_rspBuf,http_rsp,strlen(content)+1,content);
	return strlen(http_rspBuf);
}

//生成响应
int httpResponeNoFound(char*http_rspBuf)
{
	//////////////////////////////////////////
	//组建发送回去HTTP的内容
	char http_rsp[]="HTTP/1.1 404 Not Found\r\n\
Server：hx-httpd (hx-mcu)\r\n\
Content-Length：%d\r\n\
Content-Type: text/html\r\n\
Keep-Alive: timeout=5, max=100\r\n\
Connection：close\r\n\r\n\
%s";

	char content[]="404 not found";
	/*200响应成功*/
	os_sprintf(http_rspBuf,http_rsp,strlen(content)+1,content);
	return strlen(http_rspBuf);
}

//生成页面跳转
int httpResponeJumpUrl(const char* jumpUrl,char*http_rspBuf)
{
	//////////////////////////////////////////
	//组建发送回去HTTP的内容
	char http_rsp[]="HTTP/1.1 302 Found\r\n\
Server：hx-httpd (hx-mcu)\r\n\
Content-Length：0\r\n\
Content-Type: text/html\r\n\
Keep-Alive: timeout=5, max=100\r\n\
Location: %s\r\n\
Connection：close\r\n\r\n\
";
	/*200响应成功*/
	os_sprintf(http_rspBuf,http_rsp,jumpUrl);
	return strlen(http_rspBuf);
}