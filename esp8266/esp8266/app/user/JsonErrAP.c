#include "JsonErrAP.h"
#include <mem.h>
#include <driver/spi.h>
#include "http_serv.h"

///////////////////////////////////////////////////
//TCP协议
LOCAL struct espconn jsonErrAP_web_esp_conn;
LOCAL struct espconn* jsonErrAP_pEspConn=NULL;
LOCAL esp_tcp g_JsonErrEspTcp;

LOCAL void ICACHE_FLASH_ATTR jsonErrAP_sent_cb(void *arg);
LOCAL void ICACHE_FLASH_ATTR jsonErrAP_recv_cb(void *arg, char *pusrdata, unsigned short length);
LOCAL void ICACHE_FLASH_ATTR jsonErrAP_discon_cb(void *arg);
LOCAL void ICACHE_FLASH_ATTR jsonErrAP_recon_cb(void *arg, sint8 err);
LOCAL void ICACHE_FLASH_ATTR jsonErrAP_listen(void *arg);


//////////////////////////////////////////////////
//业务处理

TzhHttpServ * g_JsonErrHttpServ=NULL;//http协议处理模块
bool g_jsonErrDisconResetFactory=false;

/*****************************************************************************
 * FunctionName : jsonErrAP_sent_cb
 * Description  : data sent callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
jsonErrAP_sent_cb(void *arg)
{
	struct espconn* pConn=(struct espconn*)arg;
	//-----继续发送到客户端------------------
	TzhHttpServResponeData* fristData = httpServCurrentResumeData(g_JsonErrHttpServ);
	if(fristData)
	{ espconn_send(pConn, fristData->sendbuf, fristData->sendlen); }
	else
	{
		espconn_disconnect(pConn);//断开
	}
}

/******************************************************************************
 * FunctionName : jsonErrAP_discon_cb
 * Description  : disconnect callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
jsonErrAP_discon_cb(void *arg)
{
   //tcp disconnect successfully
    //os_printf("tcp disconnect succeed !!! \n");
	if(g_jsonErrDisconResetFactory)
	{
		g_jsonErrDisconResetFactory=false;
		//重置出厂信息
		freeFactorySetting();
	}
	jsonErrAP_pEspConn=NULL;
}

/******************************************************************************
 * FunctionName : jsonErrAP_recv_cb
 * Description  : receive callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
jsonErrAP_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
   //received some data from tcp connection
   struct espconn* pConn=(struct espconn*)arg;
   //os_printf("tcp recv length=%d\n",length); //打印接收内容
   
   char *url;
   url=(char *)os_malloc(1124);
   //os_printf("-----------------------\n");
   if(0==httpServReslovURL(g_JsonErrHttpServ,pusrdata,length,url))
   {
		char* page;
		char* parameter;

		//////////////
		page=(char*)os_malloc(64);
		parameter=(char*)os_malloc(1024);
		os_memset(page,0,64);
		os_memset(parameter,0,1024);
		//获取页面和参数
		urlSplit(url,page,parameter);
		//os_printf("url=%s\n",url);

		//////////////////////////////////////////////
		//WEB页面
		if(0==os_strcmp(page,"/") || 0==os_strcmp(page,"/index.htm") || 0==os_strcmp(page,"/config.htm"))//主页
		{
				char str_exc[10]={0};
				urlGetParameter(parameter,"exc",str_exc);
				if(0==os_strcmp("cfg",str_exc))
				{					
					char* json_cfg=(char*)os_malloc(2000);
					memset(json_cfg,0,2000);
					int json_len=0;
					urlGetParameter(parameter,"cfg",json_cfg);

					//转换成UTF8
					json_len=strlen(json_cfg);
					urldecode(json_cfg,json_len);
					json_len=strlen(json_cfg);

					//写入
					spi_flash_erase_sector(EEPROM_DATA_ADDR_JSON);//WIFI固定信息
					if(SPI_FLASH_RESULT_OK==spi_flash_write(
											EEPROM_DATA_ADDR_JSON * SPI_FLASH_SEC_SIZE,
											(uint32*)json_cfg,
											json_len+1))
					{
							//断开后重置出厂信息
							g_jsonErrDisconResetFactory=true;
							httpServResponeJumpUrl(g_JsonErrHttpServ,"config.htm");
							//-----发送到客户端------------------
							TzhHttpServResponeData* fristData = httpServFristData(g_JsonErrHttpServ);
							espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
							goto _nnc;
					}
					////os_printf("EEPROM_DATA_ADDR_JSON SPI_FLASH_RESULT_OK=%d \n",SPI_FLASH_RESULT_OK);
					if(json_cfg)
					{
						os_free(json_cfg);
						json_cfg=NULL;
					}
				}
				
				//--------发送页面 BEGIN----------------------------------
				//读取配置结构
				char devUUID[66]={0};
				TzhEEPRomDevConfig* devInfo=(TzhEEPRomDevConfig*)os_malloc(sizeof(TzhEEPRomDevConfig));
				spi_flash_read(EEPROM_DATA_ADDR_DevConfig * SPI_FLASH_SEC_SIZE,
											(uint32*)devInfo,
											sizeof(TzhEEPRomDevConfig));
				strcpy(devUUID,devInfo->devUUID);
				os_free(devInfo);
				devInfo=NULL;

				///////////////////////
				char *cfg_json_buffer;
				int jsonlen=0;
				cfg_json_buffer=(char*)os_malloc(1024);
				memset(cfg_json_buffer,0,1024);
				spi_flash_read(EEPROM_DATA_ADDR_JSON * SPI_FLASH_SEC_SIZE,(uint32*)cfg_json_buffer,1024);
				for(jsonlen=0;jsonlen<1023;jsonlen++)
				{
					if(cfg_json_buffer[jsonlen]==0x00 || cfg_json_buffer[jsonlen]==0xFF)
					{ 
						cfg_json_buffer[jsonlen]=0;
						cfg_json_buffer[jsonlen+1]=0;
						break; 
					}
				}

				//------------
				//读取页面信息
				int i;
				char *htmldata;
				int htmllen=0;
				htmldata=(char *)os_malloc(6000);
				spi_flash_read(EEPROM_DATA_ADDR_Web3 * SPI_FLASH_SEC_SIZE,(uint32*)htmldata,6000);
				for(i=0;i<5999;i++)
				{
					if(htmldata[i]==0x00 || htmldata[i]==0xFF)
					{ break; }
					htmllen++;
				}
				htmldata[htmllen]=0;

				//------------------------------------------------------
				//转换内容
				replace_str(htmldata,"{devUUID}",devUUID,htmldata);
				replace_str(htmldata,"{cfg}",cfg_json_buffer,htmldata);
				//-------------------------------------------------------
				//发送
				httpServResponeOK(g_JsonErrHttpServ,htmldata,strlen(htmldata));
					
				//-----发送页面到客户端------------------
				TzhHttpServResponeData* fristData = httpServFristData(g_JsonErrHttpServ);
				espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
				//--------发送页面 END----------------------------------
				os_free(cfg_json_buffer);
				cfg_json_buffer=NULL;
				os_free(htmldata);
				htmldata=NULL;
		}
		else if(0==os_strcmp(page,"/favicon.ico"))
		{
				//不处理这个图标
				//直接断开连接
				 espconn_disconnect(pConn);
				 //os_printf("favicon.ico disconnect \n");
		}
		else
		{
				//找不到页面
				httpServResponeNoFound(g_JsonErrHttpServ);
		}

	_nnc:
		os_free(page);
		page=NULL;
		os_free(parameter);
		parameter=NULL;
   }

	if(url)
	{
		os_free(url);
		url=NULL;
	}
}

LOCAL void ICACHE_FLASH_ATTR jsonErrAP_recon_cb(void *arg, sint8 err)
{
   //error occured , tcp connection broke.  
   // //os_printf("reconnect callback, error code %d !!! \n",err);
}

LOCAL void ICACHE_FLASH_ATTR jsonErrAP_listen(void *arg)
{
	struct espconn *pjsonErrAP_web_esp_conn = arg;

	if(jsonErrAP_pEspConn)
	{
		espconn_disconnect(pjsonErrAP_web_esp_conn);
	}
	//os_printf("jsonErrAP_listen !!! \n");
	espconn_regist_recvcb(pjsonErrAP_web_esp_conn, jsonErrAP_recv_cb);
	espconn_regist_reconcb(pjsonErrAP_web_esp_conn, jsonErrAP_recon_cb);
	espconn_regist_disconcb(pjsonErrAP_web_esp_conn, jsonErrAP_discon_cb);  
	espconn_regist_sentcb(pjsonErrAP_web_esp_conn, jsonErrAP_sent_cb);
	jsonErrAP_pEspConn=pjsonErrAP_web_esp_conn;

}

void ICACHE_FLASH_ATTR JsonErrAP_initService()
{
    jsonErrAP_web_esp_conn.type = ESPCONN_TCP;
    jsonErrAP_web_esp_conn.state = ESPCONN_NONE;
    jsonErrAP_web_esp_conn.proto.tcp = &g_JsonErrEspTcp;
    jsonErrAP_web_esp_conn.proto.tcp->local_port = 80;
    espconn_regist_connectcb(&jsonErrAP_web_esp_conn, jsonErrAP_listen);
    sint8 ret = espconn_accept(&jsonErrAP_web_esp_conn);
}

/******************************************************************************
* FunctionName : user_init
* Description  : entry of user application, init user function here
* Parameters   : none
* Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR JsonErrAP_init()
{
	//
	wifi_station_disconnect();

	g_JsonErrHttpServ=(TzhHttpServ*)os_malloc(sizeof(TzhHttpServ));
	memset(g_JsonErrHttpServ,0,sizeof(TzhHttpServ));
	//打开AP服务
	factory_set_ap_config("Hxkong_Json_Err",AUTH_OPEN,"");

	//打开WEB配置服务
	system_init_done_cb(JsonErrAP_initService);
}
