#include "urlcode.h"

//
void ICACHE_FLASH_ATTR urlencode(char const *s, int len, int *new_length,char *dst)
{
	unsigned char const *from, *end;
	unsigned char c;
	unsigned char hexchars[] = "0123456789ABCDEF";
	unsigned char *to = ( unsigned char *) os_malloc(3 * len + 1);
	unsigned char *start = NULL;
	from = (unsigned char *)s;
	end =  (unsigned char *)s + len;
	start = to;
	
	while (from < end) {
	c = *from++;
	if (c == ' ') {
		*to++ = '+';
	}
	else 
	if ((c < '0' && c != '-' && c != '.') ||
	(c < 'A' && c > '9') ||
	(c > 'Z' && c < 'a' && c != '_') ||
	(c > 'z')) {
	to[0] = '%';
	to[1] = hexchars[c >> 4];
	to[2] = hexchars[c & 15];
	to += 3;
	} else {
	*to++ = c;
	}
	}
	*to = 0;
	if (new_length) {
	*new_length = to - start;
	}
	strcpy(dst,start);
	os_free(start);
}

int ICACHE_FLASH_ATTR urldecode(char *str, int len)
{
	char *dest = str;
	char *data = str;
	int value;
	int c;

	while (len--)
	{
		if (*data == '+')
		{
			*dest = ' ';
		}
		else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1))  && isxdigit((int) *(data + 2)))
		{
			c = ((unsigned char *)(data+1))[0];
			if (isupper(c))
			c = tolower(c);
			value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;
			c = ((unsigned char *)(data+1))[1];
			if (isupper(c))
			c = tolower(c);
			value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;
			*dest = (char)value ;
			data += 2;
			len -= 2;
		}
		else
		{
			*dest = *data;
		}
		data++;
		dest++;
	}
	*dest = '\0';
	return dest - str;
}



//返回值
//0=替换失败
//1=替换成功
//replace_str(strSrc,"我被替换了","靓仔",strDst);
//查找并替换字符串
int ICACHE_FLASH_ATTR replace_str(const char *pInput,const char *pSrc,const char *pDst,char *pOutput)
{
	 const char   *pi;
	 char *p;
	 int nSrcLen, nDstLen, nLen;
	 int ret;

	 ret=0;
	 // 指向输入字符串的游动指针.
	 pi = pInput;

	 // 计算被替换串和替换串的长度.
	 nSrcLen = (int)strlen(pSrc);
	 nDstLen = (int)strlen(pDst);
	 // 查找pi指向字符串中第一次出现替换串的位置,并返回指针(找不到则返回null).
	 p = strstr(pi, pSrc);
	 if(p)
	 {
				char *po; // 指向输出字符串的游动指针.
				po=(char*)os_malloc(strlen(pInput)+strlen(pDst)+1);

				// 计算被替换串前边字符串的长度.
				nLen = (int)(p - pi);
				// 复制到输出字符串.
				os_memcpy(po, pi, nLen);
				os_memcpy(po+nLen, pDst, nDstLen);
				strcpy(po+nLen+nDstLen, pi+nLen+nSrcLen);

				// 复制剩余字符串.
				strcpy(pOutput, po);
				os_free(po);
				po=NULL;
				ret=1;
	 }
	 else
	 {
				// 没有找到则原样复制.
				strcpy(pOutput, pi);
	 }
	return ret;
}
