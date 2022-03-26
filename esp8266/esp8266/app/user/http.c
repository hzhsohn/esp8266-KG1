#include "http.h"
#include <mem.h>
#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>

//����HTTPЭ��ͷβ
char*getbegnstr(char*buf);
char*rnrnstr(char*buf);

// ������һ��end����������������ȡ���ַ����������޳�����
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

// ������һ��end����������������ȡ���ַ����������޳�����
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
                if(p==ptn_len){ // p==ptn_len ��ʾ����ƥ����
                        if(c=='\r' || c=='\n'  || (end !='\0' && c==end) ) p++; // ƥ�����
                        else dstBuf[b++]=c; // ƥ�䵽���ַ� 
                }else if(p<ptn_len){ // Ϊ�ﵽƥ��Ҫ��
                        if(c==pattern[p]) p++;
                        else p=0;
                }
        }         
        dstBuf[b]=0;
}

//�ַ�����ȡ�����ĺ���---------------------------------------
/*
    char *sd="qq=123&bb=66&cc";
    char dd[100];
    httpGetParameter(sd,"bb",dd);
    printf("%s",dd);

	����ֵ
	1�ɹ�
	0ʧ��
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

����ֵ
	1�ɹ�
	0ʧ��

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
		 
		//��ȡ�ļ�·�� 
		bufsize=0;
		while(szUrl[i]!='?' && szUrl[i]!=0) {
		dst_file[bufsize]=szUrl[i];
		bufsize++;
		i++;
		}
		dst_file[bufsize]=0;
 
		//��ȡ�ʺź���Ĳ���
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

	����ֵ:
	=0 �ɹ�����
	=1 δ������
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

		//��ȡ��ͷ
		ppheader=http->cacheBuf;
		pheadstr=getbegnstr(http->cacheBuf);

		//��ȡ��β		
		pstr=(char *)rnrnstr(http->cacheBuf);
		if(NULL==pstr)
		{
			return 0;
		}
		pstr+=4;

		//��ȡGET
		zhMatchString(pheadstr, "GET ", 0x20,dstURL); // ��ȡ�ո�֮ǰ

		//ɾ��û�õ�����
		tmp=pstr-ppheader;
		os_memcpy(http->cacheBuf,&http->cacheBuf[tmp],tmp);
		http->cacheLen-=tmp;
		return 0;
	}
	http->cacheLen=0;
	return 1;
}

//������Ӧ
int httpResponeOK(const char* content,char*http_rspBuf)
{
	//////////////////////////////////////////
	//�齨���ͻ�ȥHTTP������
	char http_rsp[]="HTTP/1.1 200 OK\r\n\
Server��hx-httpd (hx-mcu)\r\n\
Content-Length��%d\r\n\
Content-Type: text/html\r\n\
Keep-Alive: timeout=5, max=100\r\n\
Connection��close\r\n\r\n\
%s";
	/*200��Ӧ�ɹ�*/
	os_sprintf(http_rspBuf,http_rsp,strlen(content)+1,content);
	return strlen(http_rspBuf);
}

//������Ӧ
int httpResponeNoFound(char*http_rspBuf)
{
	//////////////////////////////////////////
	//�齨���ͻ�ȥHTTP������
	char http_rsp[]="HTTP/1.1 404 Not Found\r\n\
Server��hx-httpd (hx-mcu)\r\n\
Content-Length��%d\r\n\
Content-Type: text/html\r\n\
Keep-Alive: timeout=5, max=100\r\n\
Connection��close\r\n\r\n\
%s";

	char content[]="404 not found";
	/*200��Ӧ�ɹ�*/
	os_sprintf(http_rspBuf,http_rsp,strlen(content)+1,content);
	return strlen(http_rspBuf);
}

//����ҳ����ת
int httpResponeJumpUrl(const char* jumpUrl,char*http_rspBuf)
{
	//////////////////////////////////////////
	//�齨���ͻ�ȥHTTP������
	char http_rsp[]="HTTP/1.1 302 Found\r\n\
Server��hx-httpd (hx-mcu)\r\n\
Content-Length��0\r\n\
Content-Type: text/html\r\n\
Keep-Alive: timeout=5, max=100\r\n\
Location: %s\r\n\
Connection��close\r\n\r\n\
";
	/*200��Ӧ�ɹ�*/
	os_sprintf(http_rspBuf,http_rsp,jumpUrl);
	return strlen(http_rspBuf);
}