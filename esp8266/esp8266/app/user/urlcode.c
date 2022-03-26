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



//����ֵ
//0=�滻ʧ��
//1=�滻�ɹ�
//replace_str(strSrc,"�ұ��滻��","����",strDst);
//���Ҳ��滻�ַ���
int ICACHE_FLASH_ATTR replace_str(const char *pInput,const char *pSrc,const char *pDst,char *pOutput)
{
	 const char   *pi;
	 char *p;
	 int nSrcLen, nDstLen, nLen;
	 int ret;

	 ret=0;
	 // ָ�������ַ������ζ�ָ��.
	 pi = pInput;

	 // ���㱻�滻�����滻���ĳ���.
	 nSrcLen = (int)strlen(pSrc);
	 nDstLen = (int)strlen(pDst);
	 // ����piָ���ַ����е�һ�γ����滻����λ��,������ָ��(�Ҳ����򷵻�null).
	 p = strstr(pi, pSrc);
	 if(p)
	 {
				char *po; // ָ������ַ������ζ�ָ��.
				po=(char*)os_malloc(strlen(pInput)+strlen(pDst)+1);

				// ���㱻�滻��ǰ���ַ����ĳ���.
				nLen = (int)(p - pi);
				// ���Ƶ�����ַ���.
				os_memcpy(po, pi, nLen);
				os_memcpy(po+nLen, pDst, nDstLen);
				strcpy(po+nLen+nDstLen, pi+nLen+nSrcLen);

				// ����ʣ���ַ���.
				strcpy(pOutput, po);
				os_free(po);
				po=NULL;
				ret=1;
	 }
	 else
	 {
				// û���ҵ���ԭ������.
				strcpy(pOutput, pi);
	 }
	return ret;
}
