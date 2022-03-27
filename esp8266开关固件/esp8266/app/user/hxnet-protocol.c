#include "hxnet-protocol.h"
#include "hx-crc.h"

//---------------------------------------------------------
#if _IS_MCU_
int memcmp(char* src,char* dst,int len)
{
    int i;
    char b;
    
    b=0;
    for(i=0;i<len;i++)
    if(src[i]!=dst[i])
    {b=1;break;}
    return b;
}

void memcpy(void* dest, void* src, int count)
{
    char* pbTo = (char*)dest;
    char* pbFrom = (char*)src;
    while(count-->0) { *pbTo++ = *pbFrom++; }
    return;
}

int strlen(const char *str)
{
    int len = 0;
    while(*str++)
    {len++;}
    return len;
}
char *strcat(char *str1, char *str2)  
{  
    if(str1==NULL ||str2==NULL)return 0;  
    char *pt = str1;  
    while(*str1!='\0') str1++;  
    while(*str2!='\0') *str1++ = *str2++;  
    *str1 = '\0';  
    return pt;  
}
char* strcpy(char* dest, const char* src)
{
    //assert(dest != NULL && src != NULL);
    char* address=dest;
    while((*dest++=*src++) != '\0');
    return address;
}

int strcmp(const char *str1,const char *str2)
{
    while(*str1 && *str2 && (*str1 == *str2))
    {str1++;str2++;}
    return *str1-*str2;
}
#endif

char *itoa(int num, char *str, int radix)   
{
    char  string[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    char* ptr = str;
    int denom = 0;  //余数
    int count = -1;
    int i;
    int j;
    while (num >= radix)
    {
        denom   = num % radix;
        num    /= radix;

        *ptr++  = string[denom];
        count++;
    }	
    *ptr++ = string[num];
    count++;
    *ptr = '\0';
    j    = count;

    for (i = 0; i < (count + 1) / 2; i++)
    {
        int temp = str[i];
        str[i]   = str[j];
        str[j--] = temp;
    }

    return str;
}
//---------------------------------------------------------

/*
  创建一条新的指令
    返回指令的长度buf
*/
int hxNetCreateFrame(const char* flag,
                  uint parm_len,
                  const uchar* parm,
				  uchar is_need_crc16,
                  uchar* dst_buf)
{
    int flag_len=0;
    U16 crc16=0;
    int n=0;
    int i=0;
 
    dst_buf[n]=0xAA;
    n++;
    
    while(0x00!=flag[flag_len])
    {
        dst_buf[n]=flag[flag_len];
        n++;
        flag_len++;
        
        if(flag_len>255)
        {return 0;}
    }
    dst_buf[n]=0;
    n++;
    
    if(parm_len<0xFF)
    {
      dst_buf[n]=parm_len;
      n++;
    }
    else
    {
	  dst_buf[n]=0xFF;
      n++;
      dst_buf[n]=parm_len&0xFF;
      n++;
      dst_buf[n]=(parm_len&0xFF00)>>8;
      n++;
    }
    
    for(i=0;i<parm_len;i++)
    {
		if(parm)
		{
			dst_buf[n]=parm[i];
			n++;
		}
    }

	if(is_need_crc16)
	{
		dst_buf[n]=0xBB;
		n++;
    
		//CRC16检验
		crc16=hxCRC16(&dst_buf[0],n);
		dst_buf[n]=crc16&0xff;
		dst_buf[n+1]=crc16>>8;
		n+=2;
	}
	else
	{
		dst_buf[n]=0xBA;
		n++;
	}
    return n;
}



int hxNetGetFrame(uchar* cache,int cache_len,TzhNetFrame_Cmd* pcmd,uchar* is_get_cmd_success)
{
    int n=0;
    int flag_len=0;
    U16 checksum=0;
	int tmpInvaidN=0;
    
    *is_get_cmd_success=0;
    
    //最少长度为6
    //6=0xAA+协议标识字符串结尾\0+指令+参数长度+(校验码+0xBB)或(0xBA)
    if(cache_len<4)
        return 0;
    
    for(n=0;n<cache_len;n++)
    {
        //判断是否cloudkon协议头
        if(0xAA==cache[n])
        {
			pcmd->frame_head=&cache[n];
			tmpInvaidN=n;
            break;
        }
    }
    //去掉头
    if(n>0)
        return n;
    
    pcmd->head=0xAA;
    
    //获取协议标识
    for(flag_len=1;flag_len<cache_len;flag_len++)
    {
        if(0x00==cache[flag_len])
        {break;}
        else if(flag_len==cache_len-1)
        {
            //数据异常
            return 1;
        }
    }
    
    pcmd->flag=(char*)&cache[1];
    //跳到去指令
    n=flag_len+1;
    if(0xFF==cache[n])
    {
        pcmd->parameter_len=cache[n+1]|cache[n+2]<<8;
        n+=3;
    }
    else
    {
        pcmd->parameter_len=cache[n];
        n++;
    }
    
    //参数
    pcmd->parameter=&cache[n];
    n+=pcmd->parameter_len;
    
    //判断数据长度,3=帧尾0xbb+crc16
    if(cache_len<n+1)
        return 0;
       
    //校验尾标识
    if(0xBB!=cache[n] && 0xBA!=cache[n])
	  {return 1;}
    pcmd->end=cache[n];
    n++;
    
	if(0xBB==pcmd->end)
	{
		U16 t_checksum=0;
		//二次校检码
		checksum=cache[n]|cache[n+1]<<8;
		t_checksum=hxCRC16(&cache[0] ,n);
		if(checksum!=t_checksum)
		{ return 1; }
		pcmd->checksum=checksum;
		//总长度;
		n+=2;
	}
    
    //数据校验成功
    *is_get_cmd_success=1;
	pcmd->frame_len=n-tmpInvaidN;
    return n;
}
