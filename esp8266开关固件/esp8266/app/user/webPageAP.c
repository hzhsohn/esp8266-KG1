#include "webPageAP.h"
#include <mem.h>
#include <driver/spi.h>
#include "http_serv.h"
#include "json_do.h"
#include "hxk_ota.h"
#include "hxnet-protocol.h"

///////////////////////////////////////////////////
//TCP协议
LOCAL struct espconn web_esp_conn;
LOCAL struct espconn* web_pEspConn=NULL;
LOCAL esp_tcp web_esptcp;

//路由连接状态
extern EzhRouterSTA g_isRouterSTA;
//JSON文件
extern TzhJsonDo g_jsdo;
////////////////////////////////////////////////////
int g_is_scaning=0;
bool g_webAPDisconResetFactory=false;

//搜索SSID
typedef struct _TzhListSSID
{
	char ssid[33];
	char is_encrypt;
	sint8 rssi;
	struct _TzhListSSID *_next;
}TzhListSSID;
TzhListSSID * g_lstScanSSID=NULL; //搜索到的SSID列表
TzhListSSID * g_LastScanSSID=NULL; //最后一个元素

//////////////////////////////////////////////////
//业务处理

TzhHttpServ * g_httpServ=NULL;//http协议处理模块
bool is_disconnect_cb_restart=false;//发送TCP数据完毕后重启模块
int  index_page_ckd=0;//是否模块重启后第一次访问index
bool is_frist_config_page=true;//是否模块重启后第一次访问config

void ICACHE_FLASH_ATTR factory_main_ok();
void ICACHE_FLASH_ATTR user_scan(void);

/*****************************************************************************
 * FunctionName : web_send_cb
 * Description  : data sent callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
web_send_cb(void *arg)
{
	struct espconn* pConn=(struct espconn*)arg;
	//-----继续发送到客户端------------------
	TzhHttpServResponeData* fristData = httpServCurrentResumeData(g_httpServ);
	if(fristData)
	{ espconn_send(pConn, fristData->sendbuf, fristData->sendlen); }
	else
	{
		espconn_disconnect(pConn);//断开
	}
}

/******************************************************************************
 * FunctionName : web_discon_cb
 * Description  : disconnect callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
web_discon_cb(void *arg)
{
   //tcp disconnect successfully
   // os_printf("tcp disconnect succeed !!! \n");
	
	if(is_disconnect_cb_restart)
	{	
		//重启
		trRestart();
	}
	if(g_webAPDisconResetFactory)
	{
		g_webAPDisconResetFactory=false;
		//重置出厂信息
		freeFactorySetting();
	}
	web_pEspConn=NULL;
}

void ICACHE_FLASH_ATTR scan_done(void *arg, STATUS status)
{
  char ssid[33];
  char isExist; //是否存在在链表里面啦
  if (status == OK)
  {
    struct bss_info *bss_link = (struct bss_info *)arg;
    bss_link = bss_link->next.stqe_next;//ignore the first one , it's invalid.

	//
    while (bss_link != NULL)
    {
		isExist=0;

		os_memset(ssid, 0, 33);
		if (os_strlen(bss_link->ssid) <= 32)
		{ os_memcpy(ssid, bss_link->ssid, os_strlen(bss_link->ssid)); }
		else
		{ os_memcpy(ssid, bss_link->ssid, 32); }

		//os_printf("scan =%s\r\n",ssid);

		//////
		//查找是否已经存在在链表里
		TzhListSSID*pssid;
		pssid=g_lstScanSSID;
		while(pssid)
		{
			if(0==strcmp(ssid,pssid->ssid))
			{
				isExist=1;
				break;
			}
			pssid=pssid->_next;
		}
		//////
		//链表内不存在,添加至链表内
		if(0==isExist)
		{
			if(NULL==g_lstScanSSID)
			{
				g_lstScanSSID=(TzhListSSID*)os_malloc(sizeof(TzhListSSID));
				g_LastScanSSID=g_lstScanSSID;
			}
			else
			{
				g_LastScanSSID->_next=(TzhListSSID*)os_malloc(sizeof(TzhListSSID));
				g_LastScanSSID=g_LastScanSSID->_next;
			}
			os_memset(g_LastScanSSID->ssid, 0, 33);
			g_LastScanSSID->_next=NULL;
			os_memcpy(g_LastScanSSID->ssid, ssid, 32);
			g_LastScanSSID->is_encrypt=( bss_link->authmode!=AUTH_OPEN)?1:0;
			g_LastScanSSID->rssi=bss_link->rssi;
		}
        bss_link = bss_link->next.stqe_next;
    }
  }
  else
  {
     //os_printf("scan fail !!!\r\n");
  }
  g_is_scaning++;
  user_scan();
}

//延时启用
LOCAL os_timer_t g_trAPStartTimer;
void ICACHE_FLASH_ATTR trAPStartHandler()
{
	//启动服务
	factory_main_ok();
}

void ICACHE_FLASH_ATTR user_scan(void)
{
	if(g_is_scaning==0)
	{
		//清空原有列表
		TzhListSSID*pssid,*pssid2;
		pssid=g_lstScanSSID;
		while(pssid)
		{
			pssid2=pssid;
			os_free(pssid);
			pssid=pssid2->_next;
		}
	}
	if(g_is_scaning>=2) //上电搜索几次就不再搜索 
	{
		TzhListSSID*pssid,*pssid2;
		//获取当前SSID列表
		char *ssidlist;	
		ssidlist=(char *)os_malloc(1024);
		os_memset(ssidlist,0,1024);
		//
		TzhListSSID*pp;
		pp=g_lstScanSSID;
		while(pp)
		{
			/*
			是否有密码*/
			char *pssLock="";
			if(pp->is_encrypt)
			{ pssLock="*]"; };
			if(strlen(ssidlist)+strlen(pp->ssid)<1000)
			{
				os_strcat(ssidlist,"\"");
				os_strcat(ssidlist,pssLock);
				os_strcat(ssidlist,pp->ssid);
				os_strcat(ssidlist,"\",");
				//os_printf("ssida=%s\n",pp->ssid);
			}
			else
			{
				//os_printf("ssid list size > 500 break.  ssida=%s\n",pp->ssid);
				break;
			}					
			pp=pp->_next;
		}
		ssidlist[strlen(ssidlist)-1]=0x00;
		//生成内容到FLASH
		os_printf("ssidlist=%s\n",ssidlist);
		spi_flash_erase_sector(EEPROM_DATA_ADDR_TMP_SSID);//WIFI固定信息
		spi_flash_write(EEPROM_DATA_ADDR_TMP_SSID * SPI_FLASH_SEC_SIZE,(uint32*)ssidlist,1024);
		os_free(ssidlist);

		//清空原有列表
		pssid=g_lstScanSSID;
		while(pssid)
		{
			pssid2=pssid;
			os_free(pssid);
			pssid=pssid2->_next;
		}
		//------------
		//运行服务
		os_timer_disarm(&g_trAPStartTimer);
		os_timer_setfn(&g_trAPStartTimer, (os_timer_func_t *)trAPStartHandler, NULL);
		os_timer_arm(&g_trAPStartTimer, 100, 0);
		return;
	}
    wifi_station_scan(NULL,scan_done);
}

/******************************************************************************
 * FunctionName : web_recv_cb
 * Description  : receive callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
web_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
   //received some data from tcp connection
   struct espconn* pConn=(struct espconn*)arg;
   //os_printf("tcp recv length=%d\n",length); //打印接收内容
   
   char *url;
   url=(char *)os_malloc(1124);
   //os_printf("-----------------------\n");
   if(0==httpServReslovURL(g_httpServ,pusrdata,length,url))
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
		if(url)
		{
			os_free(url);
			url=NULL;
		}
		//////////////////////////////////////////////
		//WEB页面
		if(0==os_strcmp(page,"/") || 0==os_strcmp(page,"/index.htm"))//主页
		{
					//读取配置结构
					/*TzhEEPRomDevConfig config={0};
					spi_flash_read(EEPROM_DATA_ADDR_DevConfig * SPI_FLASH_SEC_SIZE,
												(uint32*)&config,
												sizeof(TzhEEPRomDevConfig));*/
					//
					TzhEEPRomUserFixedInfo fixedInfo={0};
					spi_flash_read(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,
													(uint32*)&fixedInfo,
													sizeof(TzhEEPRomUserFixedInfo));
					////////////////////////////////////
					//获取当前SSID列表
					char *ssidlist;	
					ssidlist=(char *)os_malloc(1024);
					os_memset(ssidlist,0,1024);
					//
					spi_flash_read(EEPROM_DATA_ADDR_TMP_SSID * SPI_FLASH_SEC_SIZE,(uint32*)ssidlist,1000);
					//------------------------------------------------------------------------
					int i;
					char *htmldata;
					int htmllen=0;
					htmldata=(char *)os_malloc(10000);
					if(NULL==htmldata)
					{
						goto _nnc;
					}
					spi_flash_read(EEPROM_DATA_ADDR_Web0 * SPI_FLASH_SEC_SIZE,(uint32*)htmldata,9999);
					for(i=0;i<9999;i++)
					{
						if(htmldata[i]==0x00 || htmldata[i]==0xFF)
						{ break; }
						htmllen++;
					}
					htmldata[htmllen]=0;
					//-------------------------------------------
					index_page_ckd=rand()%1000+1;
					char ckd[10]={0};
					os_sprintf(ckd,"%d",index_page_ckd);
					replace_str(htmldata,"{ckd}",ckd,htmldata);
					replace_str(htmldata,"{li}",ssidlist,htmldata);
					//
					httpServResponeOK(g_httpServ,htmldata,strlen(htmldata));
					
					//-----发送到客户端------------------
					TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
					espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
					
					os_free(htmldata);
					if(ssidlist)
					{
						os_free(ssidlist);
						ssidlist=NULL;
					}
					htmldata=NULL;
		}
		///////////////////////////////////////////////////////////////////////////
		//重启接口
		else if(0==os_strcmp(page,"/save.htm"))
		{
				char ssport[20]={0};
				urlGetParameter(parameter,"ckd",ssport);
				int ckd=atoi(ssport);
				if(ckd!=index_page_ckd)
				{
					httpServResponeJumpUrl(g_httpServ,"index.htm");
					//-----发送到客户端------------------
					TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
					espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
					goto _nnc;
				}
				//
				TzhEEPRomUserFixedInfo fixedInfo={0};

				spi_flash_read(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,
												(uint32*)&fixedInfo,
												sizeof(TzhEEPRomUserFixedInfo));				
		

				//写入,路由信息
				fixedInfo.work_mode=ezhWorkModeUWork;
				//
				urlGetParameter(parameter,"ssid",fixedInfo.sta_ssid);
				urlGetParameter(parameter,"wifipwd",fixedInfo.sta_passwd);
				//
				urlGetParameter(parameter,"dev_id",fixedInfo.dev_id);
				urlGetParameter(parameter,"mqtt_host",fixedInfo.mqtt_host);
				urlGetParameter(parameter,"mqtt_port",ssport);
				fixedInfo.mqtt_port=atoi(ssport);
				urlGetParameter(parameter,"mqtt_subscr",fixedInfo.mqtt_subscr);
				urlGetParameter(parameter,"mqtt_publish",fixedInfo.mqtt_publish);
				urlGetParameter(parameter,"mqtt_client_id",fixedInfo.mqtt_client_id);
				urlGetParameter(parameter,"mqtt_user",fixedInfo.mqtt_user);
				urlGetParameter(parameter,"mqtt_passwd",fixedInfo.mqtt_passwd);
				//				
				urlGetParameter(parameter,"devflag",fixedInfo.dev_flag);
				//转换成UTF8
				urldecode(fixedInfo.sta_ssid,strlen(fixedInfo.sta_ssid));
				urldecode(fixedInfo.sta_passwd,strlen(fixedInfo.sta_passwd));
				//
				urldecode(fixedInfo.mqtt_host,strlen(fixedInfo.mqtt_host));		
				urldecode(fixedInfo.mqtt_subscr,strlen(fixedInfo.mqtt_subscr));	
				urldecode(fixedInfo.mqtt_publish,strlen(fixedInfo.mqtt_publish));	
				urldecode(fixedInfo.mqtt_client_id,strlen(fixedInfo.mqtt_client_id));
				urldecode(fixedInfo.mqtt_user,strlen(fixedInfo.mqtt_user));
				urldecode(fixedInfo.mqtt_passwd,strlen(fixedInfo.mqtt_passwd));

				//os_printf("---%s\n",fixedInfo.sta_ssid);
				//os_printf("---%s\n",fixedInfo.sta_passwd);
				//os_printf("---%s\n",fixedInfo.mqtt_host);
				//os_printf("---%d\n",fixedInfo.mqtt_port);
				//os_printf("---%s\n",fixedInfo.mqtt_subscr);
				//os_printf("---%s\n",fixedInfo.mqtt_publish);
				//os_printf("---%s\n",fixedInfo.mqtt_client_id);
				//os_printf("---%s\n",fixedInfo.mqtt_user);
				//os_printf("---%s\n",fixedInfo.mqtt_passwd);

				if(strcmp(fixedInfo.sta_ssid,"") )//fixedInfo.sta_ssid一定不为空
				{					
						spi_flash_erase_sector(EEPROM_DATA_ADDR_UserFix);//WIFI固定信息
						if(SPI_FLASH_RESULT_OK==spi_flash_write(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,(uint32*)&fixedInfo,sizeof(TzhEEPRomUserFixedInfo)))
						{
		
								int i;
								char *htmldata;
								int htmllen=0;
								htmldata=(char *)os_malloc(7000);
								if(NULL==htmldata)
								{
									goto _nnc;
								}
								spi_flash_read(EEPROM_DATA_ADDR_Web2 * SPI_FLASH_SEC_SIZE,(uint32*)htmldata,7000);
								for(i=0;i<6999;i++)
								{
									if(htmldata[i]==0x00 || htmldata[i]==0xFF)
									{ break; }
									htmllen++;
								}
								htmldata[htmllen]=0;
								//-------------------------------------------	
								//
								httpServResponeOK(g_httpServ,htmldata,strlen(htmldata));
								//-----发送到客户端------------------
								TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
								espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
								os_free(htmldata);
								htmldata=NULL;
								
								//发送完后重启
								is_disconnect_cb_restart=true;
						}
						else
						{
							//-----回复客户端------------------
							char str[]="eeprom have some problem...";
							httpServResponeOK(g_httpServ,str,strlen(str));								
							TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
							espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
						}
				}
				else
				{
						//-----回复客户端------------------
						char str[]="devname,sta_ssid not allow null";
						httpServResponeOK(g_httpServ,str,strlen(str));								
						TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
						espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
				}
		}
		/////////////////////////////////////////////////////
		//配置接口
		else if(0==os_strcmp(page,"/config.htm"))
		{
				char str_exc[10]={0};
				urlGetParameter(parameter,"exc",str_exc);
				if(0==os_strcmp("cfg",str_exc))
				{
					if(is_frist_config_page)
					{
						httpServResponeJumpUrl(g_httpServ,"config.htm");
						//-----发送到客户端------------------
						TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
						espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
						goto _nnc;
					}
					
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
							//断开连接后重置出厂信息
							g_webAPDisconResetFactory=true;

							httpServResponeJumpUrl(g_httpServ,"config.htm");
							//-----发送到客户端------------------
							TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
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
				TzhEEPRomDevConfig devInfo={0};
				spi_flash_read(EEPROM_DATA_ADDR_DevConfig * SPI_FLASH_SEC_SIZE,
											(uint32*)&devInfo,
											sizeof(TzhEEPRomDevConfig));

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
				if(NULL==htmldata)
				{
					goto _nnc;
				}
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
				replace_str(htmldata,"{devUUID}",devInfo.devUUID,htmldata);
				replace_str(htmldata,"{cfg}",cfg_json_buffer,htmldata);
				//-------------------------------------------------------
				//发送
				httpServResponeOK(g_httpServ,htmldata,strlen(htmldata));
					
				//-----发送页面到客户端------------------
				TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
				espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
				//--------发送页面 END----------------------------------
				os_free(cfg_json_buffer);
				cfg_json_buffer=NULL;
				os_free(htmldata);
				htmldata=NULL;
				is_frist_config_page=false;
		}
		else if(0==os_strcmp(page,"/favicon.ico"))
		{
				//不处理这个图标
				//直接断开连接
				 espconn_disconnect(pConn);
				 //os_printf("favicon.ico disconnect \n");
		}
		/////////////////////////////////////////////////////////////////////////
		//用于验证是否韩讯联控设备
		else if(0==os_strcmp(page,"/isdev-mqtt.json"))
		{
				char* rebuf="{\"ret\":0,\"msg\":\"yes\"}";
				httpServResponeOK(g_httpServ,rebuf,strlen(rebuf));
				TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
				espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
		}
		//测试SSID和密码是否成功连接到路由
		else if(0==os_strcmp(page,"/set-sta.json"))
		{
				char ssid[64]={0};
				char pwd[32]={0};
				urlGetParameter(parameter,"ssid",ssid);
				urlGetParameter(parameter,"pwd",pwd);
				//转换成UTF8
				urldecode(ssid,strlen(ssid));
				urldecode(pwd,strlen(pwd));
				//
				g_isRouterSTA=ezhRouterSTAUnknow;
				//
				wifi_station_disconnect();
				struct station_config stationConf={0}; 
				os_strcpy(&stationConf.ssid,ssid);
				os_strcpy(&stationConf.password,pwd);
				wifi_station_set_config(&stationConf);
				wifi_station_connect ();

				//回复内容
				char* rebuf="{\"ret\":0,\"msg\":\"ok\"}";
				httpServResponeOK(g_httpServ,rebuf,strlen(rebuf));
				TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
				espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
		}
		//检测验证当前SSID连接状态
		else if(0==os_strcmp(page,"/sta-status.json"))
		{
				char* rebuf;
				switch (g_isRouterSTA)
				{
				case ezhRouterSTAUnknow:
					rebuf="{\"ret\":0,\"msg\":\"ezhRouterSTAUnknow\"}";
					break;
				case ezhRouterSTAFail:
					rebuf="{\"ret\":1,\"msg\":\"ezhRouterSTAFail\"}";
					break;
				case ezhRouterSTASuccess:
					rebuf="{\"ret\":2,\"msg\":\"ezhRouterSTASuccess\"}";
					break;
				}
				
				httpServResponeOK(g_httpServ,rebuf,strlen(rebuf));
				TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
				espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
		}
		//保存账号,保存账号后会自动重启
		else if(0==os_strcmp(page,"/save-sta.json"))
		{
				//
				TzhEEPRomUserFixedInfo fixedInfo={0};
				spi_flash_read(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,
												(uint32*)&fixedInfo,
												sizeof(TzhEEPRomUserFixedInfo));

				//写入,路由信息
				fixedInfo.work_mode=ezhWorkModeUWork;
				//
				urlGetParameter(parameter,"ssid",fixedInfo.sta_ssid);
				urlGetParameter(parameter,"wifipwd",fixedInfo.sta_passwd);
				//
				urlGetParameter(parameter,"mqtt_host",fixedInfo.mqtt_host);
				char ssport[20]={0};
				urlGetParameter(parameter,"mqtt_port",ssport);
				fixedInfo.mqtt_port=atoi(ssport);
				urlGetParameter(parameter,"mqtt_subscr",fixedInfo.mqtt_subscr);
				urlGetParameter(parameter,"mqtt_publish",fixedInfo.mqtt_publish);
				urlGetParameter(parameter,"mqtt_client_id",fixedInfo.mqtt_client_id);
				urlGetParameter(parameter,"mqtt_user",fixedInfo.mqtt_user);
				urlGetParameter(parameter,"mqtt_passwd",fixedInfo.mqtt_passwd);

				//转换成UTF8
				urldecode(fixedInfo.sta_ssid,strlen(fixedInfo.sta_ssid));
				urldecode(fixedInfo.sta_passwd,strlen(fixedInfo.sta_passwd));
				//
				urldecode(fixedInfo.mqtt_host,strlen(fixedInfo.mqtt_host));		
				urldecode(fixedInfo.mqtt_subscr,strlen(fixedInfo.mqtt_subscr));	
				urldecode(fixedInfo.mqtt_publish,strlen(fixedInfo.mqtt_publish));	
				urldecode(fixedInfo.mqtt_client_id,strlen(fixedInfo.mqtt_client_id));
				urldecode(fixedInfo.mqtt_user,strlen(fixedInfo.mqtt_user));
				urldecode(fixedInfo.mqtt_passwd,strlen(fixedInfo.mqtt_passwd));
				
				//os_printf("---%s\n",fixedInfo.sta_ssid);
				//os_printf("---%s\n",fixedInfo.sta_passwd);
				//os_printf("---%s\n",fixedInfo.mqtt_host);
				//os_printf("---%d\n",fixedInfo.mqtt_port);
				//os_printf("---%s\n",fixedInfo.mqtt_subscr);
				//os_printf("---%s\n",fixedInfo.mqtt_publish);
				//os_printf("---%s\n",fixedInfo.mqtt_client_id);
				//os_printf("---%s\n",fixedInfo.mqtt_user);
				//os_printf("---%s\n",fixedInfo.mqtt_passwd);
			
				char* rebuf=NULL;
				spi_flash_erase_sector(EEPROM_DATA_ADDR_UserFix);//WIFI固定信息
				if(SPI_FLASH_RESULT_OK==spi_flash_write(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,(uint32*)&fixedInfo,sizeof(TzhEEPRomUserFixedInfo)))
				{
					rebuf="{\"ret\":0,\"msg\":\"ok.to restart\"}";
				}
				else
				{
					rebuf="{\"ret\":1,\"msg\":\"fail\"}";
				}				
				httpServResponeOK(g_httpServ,rebuf,strlen(rebuf));
				TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
				espconn_send(pConn, fristData->sendbuf, fristData->sendlen);

				//重启
				is_disconnect_cb_restart=true;
		}
		//mqtt信息
		else if(0==os_strcmp(page,"/get-mqtt.json"))
		{
				TzhEEPRomUserFixedInfo fixedInfo={0};
				spi_flash_read(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,
												(uint32*)&fixedInfo,
												sizeof(TzhEEPRomUserFixedInfo));

				char rebuf[256]={0};				
				os_sprintf(rebuf,"{\"ret\":0,\"mqtt_host\":\"%s\",\"mqtt_port\":%d,\"mqtt_host\":\"%s\",\"mqtt_host\":\"%s\"}",
									fixedInfo.mqtt_host,
									fixedInfo.mqtt_port,
									fixedInfo.mqtt_subscr,
									fixedInfo.mqtt_publish);

				httpServResponeOK(g_httpServ,rebuf,strlen(rebuf));
				TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
				espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
		}
		else if(0==os_strcmp(page,"/save-mqtt.json"))
		{
				//
				TzhEEPRomUserFixedInfo fixedInfo={0};
				spi_flash_read(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,
												(uint32*)&fixedInfo,
												sizeof(TzhEEPRomUserFixedInfo));
				//
				char mqtt_host[128]={0};
				char mqtt_port[20]={0};
				char mqtt_subscr[96]={0};
				char mqtt_publish[64]={0};
				char mqtt_client_id[64]={0};
				char mqtt_user[64]={0};
				char mqtt_passwd[48]={0};
				urlGetParameter(parameter,"mqtt_host",mqtt_host);	
				urlGetParameter(parameter,"mqtt_port",mqtt_port);
				urlGetParameter(parameter,"mqtt_subscr",mqtt_subscr);
				urlGetParameter(parameter,"mqtt_publish",mqtt_publish);
				urlGetParameter(parameter,"mqtt_client_id",mqtt_client_id);
				urlGetParameter(parameter,"mqtt_user",mqtt_user);
				urlGetParameter(parameter,"mqtt_passwd",mqtt_passwd);

				//转换成UTF8
				urldecode(mqtt_host,strlen(mqtt_host));		
				urldecode(mqtt_subscr,strlen(mqtt_subscr));	
				urldecode(mqtt_publish,strlen(mqtt_publish));
				urldecode(mqtt_client_id,strlen(mqtt_client_id));
				urldecode(mqtt_user,strlen(mqtt_user));
				urldecode(mqtt_passwd,strlen(mqtt_passwd));

				//
				strcpy(fixedInfo.mqtt_host,mqtt_host);
				fixedInfo.mqtt_port=atoi(mqtt_port);
				strcpy(fixedInfo.mqtt_subscr,mqtt_subscr);
				strcpy(fixedInfo.mqtt_publish,mqtt_publish);
				strcpy(fixedInfo.mqtt_client_id,mqtt_client_id);
				strcpy(fixedInfo.mqtt_user,mqtt_user);
				strcpy(fixedInfo.mqtt_passwd,mqtt_passwd);

				char* rebuf=NULL;
				spi_flash_erase_sector(EEPROM_DATA_ADDR_UserFix);//WIFI固定信息
				if(SPI_FLASH_RESULT_OK==spi_flash_write(EEPROM_DATA_ADDR_UserFix * SPI_FLASH_SEC_SIZE,(uint32*)&fixedInfo,sizeof(TzhEEPRomUserFixedInfo)))
				{ rebuf="{\"ret\":0,\"msg\":\"set mqtt information ok\"}"; }
				else
				{ rebuf="{\"ret\":1,\"msg\":\"fail\"}"; }				
				httpServResponeOK(g_httpServ,rebuf,strlen(rebuf));
				TzhHttpServResponeData* fristData = httpServFristData(g_httpServ);
				espconn_send(pConn, fristData->sendbuf, fristData->sendlen);
		}
		else
		{
				//找不到页面
				httpServResponeNoFound(g_httpServ);
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
/******************************************************************************
 * FunctionName : web_recon_cb
 * Description  : reconnect callback, error occured in TCP connection.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR web_recon_cb(void *arg, sint8 err)
{
   //error occured , tcp connection broke.  
   // //os_printf("reconnect callback, error code %d !!! \n",err);
}

/******************************************************************************
 * FunctionName : web_listen
 * Description  : TCP server listened a connection successfully
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR web_listen(void *arg)
{
	struct espconn *pweb_esp_conn = arg;

	if(web_pEspConn)
	{
		espconn_disconnect(pweb_esp_conn);
	}
	//os_printf("web_listen !!! \n");
	espconn_regist_recvcb(pweb_esp_conn, web_recv_cb);
	espconn_regist_reconcb(pweb_esp_conn, web_recon_cb);
	espconn_regist_disconcb(pweb_esp_conn, web_discon_cb);  
	espconn_regist_sentcb(pweb_esp_conn, web_send_cb);
	web_pEspConn=pweb_esp_conn;

}

/******************************************************************************
* FunctionName : user_tcpserver_init
* Description  : parameter initialize as a TCP server
* Parameters   : port -- server port
* Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
factory_tcpserver_init(uint32 port)
{
    web_esp_conn.type = ESPCONN_TCP;
    web_esp_conn.state = ESPCONN_NONE;
    web_esp_conn.proto.tcp = &web_esptcp;
    web_esp_conn.proto.tcp->local_port = port;
    espconn_regist_connectcb(&web_esp_conn, web_listen);
    sint8 ret = espconn_accept(&web_esp_conn);
}
int ICACHE_FLASH_ATTR factory_set_ap_config(char*ap_ssid,AUTH_MODE auth,char*wifipwd)
{
	// Wifi configuration 
	char ssid[32]={0};
	char wpwd[64]={0}; 
	char tpwd[64]={0}; 
	struct ip_info ipinfo;
	struct softap_config config={0};

	//
	wifi_station_disconnect();

	//Set  station mode 
    wifi_set_opmode(SOFTAP_MODE); 
	strcpy(ssid,ap_ssid);
	strcpy(wpwd,wifipwd);

	//////////////////////////////
	wifi_softap_get_config(&config); // Get config first.   
	os_memcpy(&config.ssid, ssid, 32); 
	os_memcpy(&config.password, wpwd, 32); 
	config.authmode = auth;
	config.ssid_len = 0;// or its actual length
	config.max_connection = 2; //max 4, how many stations can connect to ESP8266 softAP at most.
	
	//set ip address
	wifi_softap_dhcps_stop();
	IP4_ADDR(&ipinfo.ip,192,168,1,10);
	IP4_ADDR(&ipinfo.netmask,255,255,255,0);
	IP4_ADDR(&ipinfo.gw,192,168,1,10);
	wifi_set_ip_info(SOFTAP_IF,&ipinfo);
	wifi_softap_dhcps_start();
	wifi_softap_set_config(&config);// Set ESP8266 softap config .
	return 0;
}
 
/******************************************************************************
* FunctionName : user_init
* Description  : entry of user application, init user function here
* Parameters   : none
* Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR factory_main_ok()
{	
	//
	wifi_station_disconnect();
	wifi_set_opmode_current(SOFTAP_MODE); 

	//转换WIFI的名称
	char tmp_ssid[36]={0};
	char chipid[16]={0};//芯片ID
	os_sprintf(chipid,"%02X",system_get_chip_id());
	if(0==g_jsdo.ap_ssid[0])
	{
		os_sprintf(tmp_ssid,"%s_ap_isNull",chipid);
	}
	else
	{
		os_sprintf(tmp_ssid,"%s%s",g_jsdo.ap_ssid,chipid);
	}
	//打开AP服务
	if(0==g_jsdo.ap_passwd[0])
	{		
		//os_printf("---ssid %s no password\n",g_jsdo.ap_ssid);
		factory_set_ap_config(tmp_ssid,AUTH_OPEN,"");
	}
	else
	{
		//os_printf("---ssid %s have password=%s\n",g_jsdo.ap_ssid,g_jsdo.ap_passwd);
		factory_set_ap_config(tmp_ssid,AUTH_WPA2_PSK,g_jsdo.ap_passwd);
	}
	//打开WEB配置服务
	g_httpServ=(TzhHttpServ*)os_malloc(sizeof(TzhHttpServ));
	memset(g_httpServ,0,sizeof(TzhHttpServ));
	factory_tcpserver_init(80);
}
void ICACHE_FLASH_ATTR factory_main(TzhEEPRomUserFixedInfo* user)
{
	//上电搜索五遍周边WIFI-SSID
	g_is_scaning=0;
	//Set softAP + station mode 
    wifi_set_opmode_current(STATIONAP_MODE); 
	//获取可用WIFI
	system_init_done_cb(user_scan);
	//system_init_done_cb(factory_main_ok);
}
