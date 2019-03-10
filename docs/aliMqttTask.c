#if 1//ndef WIN32//zhmch 20180529
//#include "sys_if.h"
//#include "hb_typedef.h"
#include "app_ltlcom.h"       	/* Task message communiction */
#include "cbm_api.h"
#include "cbm_consts.h"
#include "soc_api.h"
#include "syscomp_config.h"
#include "task_config.h"      	/* Task creation */
#include "MMIDataType.h"
#include "DtcntSrvIntStruct.h"
#include "SimCtrlSrvGprot.h"

#include "kal_public_defs.h"
#include "kal_general_types.h"
#include "kal_public_api.h"
#include "kal_trace.h"
#include "app2soc_struct.h"
#include "stack_ltlcom.h"
#include "stack_config.h"
#include "aliMqttTask.h"
#include "MMI_features.h"
#include "TimerEvents.h"
#include "dev_sign_api.h"
#include "wrapper.h"
#include "mqtt_api.h"

typedef enum {
   ALI_NET_ID_MOBILE,     //移动
   ALI_NET_ID_CN,         // 联通gsm
   ALI_NET_ID_CDMA,       //联通CDMA
   ALI_NET_ID_NONE,       //未插卡
   ALI_NET_ID_OTHER       //其他网络
}NET_ID_TPYE;

typedef enum
{
	ALI_UNET_SIM_MASTER,
	ALI_UNET_SIM_SLAVE,
	ALI_UNET_SIM_THIRD,
	ALI_UNET_SIM_FOURTH,
	ALI_UNET_SIM_MAX
}e_unet_sim_type;


typedef void (*FuncPtr) (void);

static cbm_sim_id_enum S_CBM_SIM_ID = CBM_SIM_ID_SIM1;
static kal_int8 g_gprs_status =-1;
static kal_bool  g_sim_init_complete =KAL_FALSE;
iotx_sign_mqtt_t g_mqtt_signout;
kal_uint8  g_ali_app_id =0;
kal_uint32 g_ali_nwk_account_id = 0;
kal_int16 g_ali_request_id =60;
int g_mqtt_connect_status = 0;
void *g_mqtt_handle = NULL;

static void mqtt_task_main(task_entry_struct *task_entry_ptr);
static kal_bool mqtt_task_init(task_indx_type task_indx);

extern void StartTimer(kal_uint16 timerid, kal_uint32 delay, FuncPtr funcPtr);
extern void StopTimer(kal_uint16 timerid);


kal_bool mqtt_create(comptask_handler_struct **handle)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    static const comptask_handler_struct mqtt_handler_info =
    {
        mqtt_task_main,  /* task entry function */
        mqtt_task_init,  /* task initialization function */
        NULL,           /* task configuration function */
        NULL, /* task reset handler */
        NULL,           /* task termination handler */
    };

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    *handle = (comptask_handler_struct*)&mqtt_handler_info;

    return KAL_TRUE;
}
kal_uint16 mqtt_start_flag =0;


static kal_bool mqtt_task_init(task_indx_type task_indx)
{
	return KAL_TRUE;
}

void example_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    DEBUG_TRACE("msg->event_type : %d\n", msg->event_type);
}

kal_bool ali_socket_host_by_name_notify(void* inMsg)
{
	kal_int8 i;
	int res = 0;
    app_soc_get_host_by_name_ind_struct* dns_ind = (app_soc_get_host_by_name_ind_struct*)inMsg;
	kal_uint8 addr_buff[MAX_SOCK_ADDR_LEN] = {0};
	kal_uint32 addr_buf;
	kal_uint8 addr_len;
	kal_uint16 port;
    sockaddr_struct addr;
    int connect_result = -1;
    char ipaddr[MAX_SOCK_ADDR_LEN] = {0};	
	iotx_mqtt_param_t mqtt_params;

	DEBUG_TRACE("[ALI_TASK].socket_host_by_name_notify.g_ali_request_id=%d,request_id=%d", g_ali_request_id,dns_ind->request_id);

    if(g_ali_request_id != dns_ind->request_id){
        return KAL_FALSE;
    }

	DEBUG_TRACE("[ALI_TASK].socket_host_by_name_notify.dns_ind->result=%d", dns_ind->result);
	DEBUG_TRACE("[ALI_TASK].socket_host_by_name_notify.dns_ind->addr_len=%d", dns_ind->addr_len);
	
	for(i=0;i<dns_ind->addr_len;i++)
    {
		DEBUG_TRACE("[ALI_TASK].socket_host_by_name_notify.dns_ind->addr[%d]=%d",i,dns_ind->addr[i]);
	}

    if(dns_ind)
    {
     DEBUG_TRACE("[ALI_TASK].socket_host_by_name_notify dns_ind->request_id=%d, g_ali_request_id=%d", dns_ind->request_id, g_ali_request_id);
     
    }
	
    if (dns_ind && dns_ind->result != KAL_FALSE)
    {
        sprintf(ipaddr,"%d.%d.%d.%d",dns_ind->addr[0],dns_ind->addr[1],dns_ind->addr[2],dns_ind->addr[3]);
		HAL_Free(g_mqtt_signout.hostname);
		g_mqtt_signout.hostname = HAL_Malloc(strlen(ipaddr) + 1);
		if (g_mqtt_signout.hostname == NULL) {
			DEBUG_TRACE("HAL_Malloc failed");
			return;
		}
		memset(g_mqtt_signout.hostname,0,strlen(ipaddr) + 1);
		memcpy(g_mqtt_signout.hostname,ipaddr,strlen(ipaddr));
		DEBUG_TRACE("g_mqtt_signout.hostname: %s",g_mqtt_signout.hostname);

		memset(&mqtt_params,0,sizeof(iotx_mqtt_param_t));

		
		mqtt_params.port = g_mqtt_signout.port;
		mqtt_params.host = g_mqtt_signout.hostname;
		mqtt_params.client_id = g_mqtt_signout.clientid;
		mqtt_params.username = g_mqtt_signout.username;
		mqtt_params.password = g_mqtt_signout.password;
	
		mqtt_params.request_timeout_ms = 2000;
		mqtt_params.clean_session = 0;
		mqtt_params.keepalive_interval_ms = 60000;
		mqtt_params.read_buf_size = 1024;
		mqtt_params.write_buf_size = 1024;
	
		mqtt_params.handle_event.h_fp = example_event_handle;
		mqtt_params.handle_event.pcontext = NULL;

		g_mqtt_handle = IOT_MQTT_Construct(&mqtt_params);
		if (g_mqtt_handle != NULL){
			g_mqtt_connect_status = 1;
			DEBUG_TRACE("IOT_MQTT_Construct Success");
		}
    }
    else
    {
        DEBUG_TRACE("[ALI_TASK].socket_host_by_name_notify.dns_ind->result.2 =%d, FAIL!",dns_ind->result);
        soc_gethostbyname(KAL_FALSE,MOD_MQTT,g_ali_request_id,g_mqtt_signout.hostname, 
				 (kal_uint8 *)addr_buff, &addr_len,0,g_ali_nwk_account_id);
    }

    return KAL_TRUE;
}

void example_message_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    iotx_mqtt_topic_info_t	   *topic_info = (iotx_mqtt_topic_info_pt) msg->msg;
	char topic[128] = {0};
	char payload[256] = {0};

    DEBUG_TRACE("msg->event_type : %d\n", msg->event_type);
	switch (msg->event_type) {
		case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
		{
			memcpy(topic,topic_info->ptopic,topic_info->topic_len);
			memcpy(payload,topic_info->payload,topic_info->payload_len);
			
			/* print topic name and topic message */
			DEBUG_TRACE("Message Arrived:");
			DEBUG_TRACE("Topic  : %s", topic);
			DEBUG_TRACE("Payload: %s", payload);
		}
		break;
		default:
			break;
	}
}

int example_subscribe(void *handle)
{
    int res = 0;
    char product_key[IOTX_PRODUCT_KEY_LEN] = {0};
    char device_name[IOTX_DEVICE_NAME_LEN] = {0};
    const char *fmt = "/%s/%s/get";
    char topic[128] = {0};

    HAL_GetProductKey(product_key);
    HAL_GetDeviceName(device_name);

    HAL_Snprintf(topic, 128, fmt, product_key, device_name);

    res = IOT_MQTT_Subscribe(handle, topic, IOTX_MQTT_QOS0, example_message_arrive, NULL);
    if (res < 0) {
        DEBUG_TRACE("subscribe failed\n");
        return -1;
    }
    return 0;
}

int example_publish(void *handle)
{
    int res = 0;
    iotx_mqtt_topic_info_t topic_msg;
    char product_key[IOTX_PRODUCT_KEY_LEN] = {0};
    char device_name[IOTX_DEVICE_NAME_LEN] = {0};
    const char *fmt = "/%s/%s/update";
    char topic[128] = {0};
    char *payload = "hello,world";

    HAL_GetProductKey(product_key);
    HAL_GetDeviceName(device_name);

    HAL_Snprintf(topic, 128, fmt, product_key, device_name);

    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
    topic_msg.qos = IOTX_MQTT_QOS1;
    topic_msg.retain = 0;
    topic_msg.dup = 0;
    topic_msg.payload = (void *)payload;
    topic_msg.payload_len = strlen(payload);

    res = IOT_MQTT_Publish(handle, topic, &topic_msg);
    if (res < 0) {
        DEBUG_TRACE("publish failed\n");
        return -1;
    }
    return 0;
}

void ali_mqtt_yield(void)
{
	static int send_interval = 0;
	DEBUG_TRACE("ali_mqtt_yield...");
	IOT_MQTT_Yield(g_mqtt_handle,200);

	send_interval+=5;
	if (send_interval == 20) {
		send_interval = 0;
		example_publish(g_mqtt_handle);
	}
	StartTimer(ALIYUN_YIELD_TIMER,1000*5,ali_mqtt_yield);
}

void ali_socket_notify(void *msg_ptr)
{
    kal_bool is_ready = KAL_FALSE;
    int ret,retVal =1;
    app_soc_notify_ind_struct *soc_notify;
	iotx_mqtt_nwk_param_t nwk_param;
    DEBUG_TRACE("ali_socket_notify\n");

    soc_notify = (app_soc_notify_ind_struct*) msg_ptr;

	DEBUG_TRACE("[ali_socket_notify socket_id=%d, event_type=%d, result=%d"
    	,soc_notify->socket_id,soc_notify->event_type,soc_notify->result);

	memset(&nwk_param,0,sizeof(iotx_mqtt_nwk_param_t));
	nwk_param.fd = soc_notify->socket_id;

    switch (soc_notify->event_type) 
    {
        case SOC_READ:
        {
        	DEBUG_TRACE("Ali SOC_READ Event");
			IOT_MQTT_Nwk_Event_Handler(g_mqtt_handle,IOTX_MQTT_SOC_READ,&nwk_param);
			DEBUG_TRACE("Ali SOC_READ End");
        }
		break; 
        case SOC_CONNECT:
        {
			DEBUG_TRACE("Ali SOC_CONNECT Event");
			ret = IOT_MQTT_Nwk_Event_Handler(g_mqtt_handle,IOTX_MQTT_SOC_CONNECTED,&nwk_param);
			if (ret == SUCCESS_RETURN) {
				g_mqtt_connect_status = 2;
				DEBUG_TRACE("Aliyun connect success,start yield");
				example_subscribe(g_mqtt_handle);
				StopTimer(ALIYUN_YIELD_TIMER);
    			StartTimer(ALIYUN_YIELD_TIMER,1000*2,ali_mqtt_yield);
			}
        }
		break; 
        case SOC_CLOSE:
        {
		   DEBUG_TRACE("Ali SOC_CLOSE Event"); 
        }
		break;
		default:
			break;
    }
}


void mqtt_task_main(task_entry_struct *task_entry_ptr)
{
    ilm_struct current_ilm;
    kal_uint32 my_index = 0;

    //mqtt_timer_init();

    kal_get_my_task_index(&my_index);

    while(1)
    {
        receive_msg_ext_q( task_info_g[task_entry_ptr->task_indx].task_ext_qid, &current_ilm);
        stack_set_active_module_id(my_index, current_ilm.dest_mod_id);
        switch (current_ilm.msg_id)
        {
            /*se MSG_ID_TIMER_EXPIRY:
            {
                mqtt_timer_expiry_handler(current_ilm);
                break;
            }*/
            case MSG_ID_APP_SOC_NOTIFY_IND:
            {
				//ERROR_TRACE("%s,%d", __FUNCTION__,__LINE__);
				ERROR_TRACE("%s,%d", __FUNCTION__,__LINE__);
				ali_socket_notify(current_ilm.local_para_ptr);
                break;
            }
            case MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND:
            {
                ali_socket_host_by_name_notify(current_ilm.local_para_ptr);
                break;
            }
            case MSG_ID_ALIMQTT_SEND:
            {
                //mqttsocket_send_start();
                break;
            }
            default:
                break;
        }
		//shining modify for shouqianba :segment fault 20181123
		free_ilm( &current_ilm);
    }
    
}

static int ali_unet_get_networkid(cbm_sim_id_enum simId)
{
	char plmn[SRV_MAX_PLMN_LEN + 1];

	srv_nw_info_get_nw_plmn(MMI_SIM1 << simId, plmn, sizeof(plmn));

	if (strncmp(plmn, "46002", 5) == 0
		|| strncmp((char *)plmn, "46000", 5) == 0
		|| strncmp((char *)plmn, "46007", 5) == 0)
	{
		return ALI_NET_ID_MOBILE;
	} 
	else if(strncmp(plmn, "46001", 5) ==0)
	{
		return ALI_NET_ID_CN;
	} 
	else if(strncmp(plmn, "46003", 5) == 0)
	{
		return ALI_NET_ID_CDMA;
	}

	return ALI_NET_ID_OTHER;
}

static kal_uint32 ali_unet_mgr_find_acc_prof_id_by_apn(const char* apn, cbm_sim_id_enum sim)
{
	extern srv_dtcnt_store_info_context g_srv_dtcnt_store_ctx;
	int i;

	for (i = 0; i < SRV_DTCNT_PROF_MAX_ACCOUNT_NUM; ++i)
	{
		if (g_srv_dtcnt_store_ctx.acc_list[i].in_use &&
			g_srv_dtcnt_store_ctx.acc_list[i].bearer_type == SRV_DTCNT_BEARER_GPRS &&
			g_srv_dtcnt_store_ctx.acc_list[i].sim_info == (sim + 1) &&
			app_stricmp((char*)g_srv_dtcnt_store_ctx.acc_list[i].dest_name, (char*)apn) == 0)            
		{       
			return g_srv_dtcnt_store_ctx.acc_list[i].acc_id;
		}
	}

	return -1;		
}

static kal_uint32 ali_unet_mgr_find_acc_prof_id(const WCHAR* account_name)
{
	int i, j;
	srv_dtcnt_result_enum ret;
	srv_dtcnt_store_prof_qry_struct store_prof_qry;
	srv_dtcnt_prof_str_info_qry_struct prof_str_info_qry;
	U16 name[SRV_DTCNT_PROF_MAX_ACC_NAME_LEN + 1] = {0};

	for (j = 0; j < SRV_SIM_CTRL_MAX_SIM_NUM; ++j)
	{
		memset(&store_prof_qry, 0, sizeof(store_prof_qry));
		store_prof_qry.qry_info.filters = SRV_DTCNT_STORE_QRY_TYPE_SIM | SRV_DTCNT_STORE_QRY_TYPE_BEARER_TYPE;
		store_prof_qry.qry_info.sim_qry_info = j + 1;
		store_prof_qry.qry_info.bearer_qry_info = SRV_DTCNT_BEARER_GPRS;

		ret = srv_dtcnt_store_qry_ids(&store_prof_qry);
		if (ret == SRV_DTCNT_RESULT_SUCCESS)
		{
			for (i = 0; i < store_prof_qry.num_ids; ++i)
			{
				prof_str_info_qry.dest = (S8*)name;
				prof_str_info_qry.dest_len = sizeof(name);
				srv_dtcnt_get_account_name(store_prof_qry.ids[i], &prof_str_info_qry, SRV_DTCNT_ACCOUNT_PRIMARY);
				
				if (kal_wstrcmp(account_name, name) == 0)
				{
					return store_prof_qry.ids[i];
				}
			}
		}	
	}

	return -1;	
}

static kal_uint32 ali_unet_mgr_add_acc_prof_id(const WCHAR* account_name)
{
	srv_dtcnt_store_prof_data_struct store_prof_data;
	srv_dtcnt_prof_gprs_struct prof_gprs;
	kal_uint32 acc_prof_id;
	srv_dtcnt_result_enum ret;

	memset(&prof_gprs, 0, sizeof(prof_gprs));
	prof_gprs.APN = "cmwap";
	prof_gprs.prof_common_header.sim_info = SRV_DTCNT_SIM_TYPE_1;
	prof_gprs.prof_common_header.AccountName = (const U8*)account_name;

	prof_gprs.prof_common_header.acct_type = SRV_DTCNT_PROF_TYPE_USER_CONF;
	
	prof_gprs.prof_common_header.px_service = SRV_DTCNT_PROF_PX_SRV_HTTP;
	prof_gprs.prof_common_header.use_proxy = KAL_TRUE;
	prof_gprs.prof_common_header.px_addr[0] = 10;
	prof_gprs.prof_common_header.px_addr[1] = 0;
	prof_gprs.prof_common_header.px_addr[2] = 0;
	prof_gprs.prof_common_header.px_addr[3] = 172;
	prof_gprs.prof_common_header.px_port = 80;

    //ll_zxh
//    prof_gprs.prof_common_header.Auth_info.UserName ="";
//    prof_gprs.prof_common_header.Auth_info.Passwd ="";
    //ll_zxh	
	
	store_prof_data.prof_data = &prof_gprs;
	store_prof_data.prof_fields = SRV_DTCNT_PROF_FIELD_ALL;
	store_prof_data.prof_type = SRV_DTCNT_BEARER_GPRS;

	ret = srv_dtcnt_store_add_prof(&store_prof_data, &acc_prof_id);

	if (ret != SRV_DTCNT_RESULT_SUCCESS)
	{
		acc_prof_id = -1;
	}	

	return acc_prof_id;
}

static kal_bool ali_unet_mgr_update_acc_prof_id(kal_uint32 acc_prof_id, const WCHAR* account_name,
				const char* apn, e_unet_sim_type sim )
{
	srv_dtcnt_result_enum ret;
	srv_dtcnt_store_prof_data_struct prof_info;
	srv_dtcnt_prof_gprs_struct prof_gprs;
	int i;

//	MMI_ASSERT(apn != NULL);
	
	memset(&prof_gprs, 0, sizeof(prof_gprs));
	prof_gprs.APN = apn;
	prof_gprs.prof_common_header.sim_info = sim + 1;
	prof_gprs.prof_common_header.AccountName = (const U8*)account_name;

	prof_gprs.prof_common_header.acct_type = SRV_DTCNT_PROF_TYPE_USER_CONF;
	
	prof_gprs.prof_common_header.px_service = SRV_DTCNT_PROF_PX_SRV_HTTP;
	if (app_stricmp((char*)apn, (char*)"cmwap") == 0)
	{
		prof_gprs.prof_common_header.use_proxy = KAL_TRUE;
		prof_gprs.prof_common_header.px_addr[0] = 10;
		prof_gprs.prof_common_header.px_addr[1] = 0;
		prof_gprs.prof_common_header.px_addr[2] = 0;
		prof_gprs.prof_common_header.px_addr[3] = 172;
		prof_gprs.prof_common_header.px_port = 80;
	}

    //ll_zxh
//    prof_gprs.prof_common_header.Auth_info.UserName ="";
//    prof_gprs.prof_common_header.Auth_info.Passwd ="";
    //ll_zxh
    
	prof_info.prof_data = &prof_gprs;
	prof_info.prof_fields = SRV_DTCNT_PROF_FIELD_ALL;
	prof_info.prof_type = SRV_DTCNT_BEARER_GPRS;

	ret = srv_dtcnt_store_update_prof(acc_prof_id, &prof_info);

	if (ret == SRV_DTCNT_RESULT_SUCCESS)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


static kal_uint32 ali_unet_mgr_get_acc_prof_id(char * apn)
{
	const WCHAR* account_names[] = { L"KNU1 GPRS", L"KNU2 GPRS" };
	kal_uint32 modify_acc_prof_ids[] = { SRV_DTCNT_PROF_MAX_ACCOUNT_NUM - 1, SRV_DTCNT_PROF_MAX_ACCOUNT_NUM - 2 };
	kal_uint32 acc_prof_id;
	kal_int32 id;

	id = 0;

    
	acc_prof_id = ali_unet_mgr_find_acc_prof_id_by_apn(apn, S_CBM_SIM_ID);


	if (acc_prof_id != -1)
	{
		return acc_prof_id;
	}


	acc_prof_id = ali_unet_mgr_find_acc_prof_id(account_names[id]);


	if (acc_prof_id == -1)
	{
		acc_prof_id = ali_unet_mgr_add_acc_prof_id(account_names[id]);
	}


	if (acc_prof_id == -1)
	{
		if (id == 0)
		{
			acc_prof_id = ali_unet_mgr_find_acc_prof_id(account_names[1]);
		}
		else
		{
			acc_prof_id = ali_unet_mgr_find_acc_prof_id(account_names[0]);
		}

		if (acc_prof_id == -1)
		{
			acc_prof_id = modify_acc_prof_ids[id];
		}
		else
		{
			if (acc_prof_id == SRV_DTCNT_PROF_MAX_ACCOUNT_NUM - 1)
			{
				acc_prof_id = acc_prof_id - 1;
			}
			else
			{
				acc_prof_id = acc_prof_id + 1;
			}
		}
	}
	if (!ali_unet_mgr_update_acc_prof_id(acc_prof_id, account_names[id], apn, S_CBM_SIM_ID))
	{
		acc_prof_id = -1;
	}
	return acc_prof_id;
}


kal_uint32 ali_get_acc_prof_id(void)
{
	int network;
	kal_uint32 accid = -1;
	
	network = ali_unet_get_networkid(S_CBM_SIM_ID);

	if(network == ALI_NET_ID_MOBILE)
	{
		if((accid = ali_unet_mgr_get_acc_prof_id("cmnet")) == -1)
        {
			accid = ali_unet_mgr_get_acc_prof_id("cmwap");
		}
	}
	else if((network == ALI_NET_ID_CN) || (network == ALI_NET_ID_CDMA))
	{
		if((accid = ali_unet_mgr_get_acc_prof_id("uninet")) == -1)
        {
			accid = ali_unet_mgr_get_acc_prof_id("uniwap");
		}
	}
	else
	{
		accid = CBM_DEFAULT_ACCT_ID;
	}

	return accid;
}

static void ali_app_start_fun(void)
{
	uint32_t res = 0;
	iotx_dev_meta_info_t meta;
	kal_uint8 addr_buff[MAX_SOCK_ADDR_LEN] = {0};
	kal_uint8  addr_len =0;
	kal_uint32 accproid;
	char ipaddr[MAX_SOCK_ADDR_LEN] = {0};
	iotx_mqtt_param_t mqtt_params;
	
	memset(&meta,0,sizeof(iotx_dev_meta_info_t));
	memset(&g_mqtt_signout,0,sizeof(iotx_sign_mqtt_t));

	HAL_GetProductKey(meta.product_key);
	HAL_GetDeviceName(meta.device_name);
	HAL_GetDeviceSecret(meta.device_secret);

	res = IOT_Sign_MQTT(IOTX_CLOUD_REGION_SHANGHAI,&meta,&g_mqtt_signout);
	if (res == 0) {
		DEBUG_TRACE("signout.hostname: %s",g_mqtt_signout.hostname);
		DEBUG_TRACE("signout.port    : %d",g_mqtt_signout.port);
		DEBUG_TRACE("signout.clientid: %s",g_mqtt_signout.clientid);
		DEBUG_TRACE("signout.username: %s",g_mqtt_signout.username);
		DEBUG_TRACE("signout.password: %s",g_mqtt_signout.password);

    	cbm_register_app_id(&g_ali_app_id);

		accproid = ali_get_acc_prof_id();
		g_ali_nwk_account_id = cbm_encode_data_account_id(/*CBM_DEFAULT_ACCT_ID*/accproid, S_CBM_SIM_ID, g_ali_app_id, KAL_FALSE);
	
		res = soc_gethostbyname(KAL_FALSE,MOD_MQTT,g_ali_request_id,g_mqtt_signout.hostname, 
				 (kal_uint8 *)addr_buff, &addr_len,0,g_ali_nwk_account_id);

		if (res >= SOC_SUCCESS)             // success
	    {
	        DEBUG_TRACE("ali task socket_host_by_name SOC_SUCCESS");
	        if(addr_len !=0 && addr_len<MAX_SOCK_ADDR_LEN)
	        {
	            //connect....
	            sprintf(ipaddr,"%d.%d.%d.%d",addr_buff[0],addr_buff[1],addr_buff[2],addr_buff[3]);
				DEBUG_TRACE("ipaddr: %s[%d.%d.%d.%d]",ipaddr,addr_buff[0],addr_buff[1],addr_buff[2],addr_buff[3]);
				HAL_Free(g_mqtt_signout.hostname);
				g_mqtt_signout.hostname = HAL_Malloc(strlen(ipaddr) + 1);
				if (g_mqtt_signout.hostname == NULL) {
					DEBUG_TRACE("HAL_Malloc failed");
					return;
				}
				memset(g_mqtt_signout.hostname,0,strlen(ipaddr) + 1);
				memcpy(g_mqtt_signout.hostname,ipaddr,strlen(ipaddr));

				DEBUG_TRACE("g_mqtt_signout.hostname: %s",g_mqtt_signout.hostname);

				memset(&mqtt_params,0,sizeof(iotx_mqtt_param_t));
		
				mqtt_params.port = g_mqtt_signout.port;
				mqtt_params.host = g_mqtt_signout.hostname;
				mqtt_params.client_id = g_mqtt_signout.clientid;
				mqtt_params.username = g_mqtt_signout.username;
				mqtt_params.password = g_mqtt_signout.password;
			
				mqtt_params.request_timeout_ms = 2000;
				mqtt_params.clean_session = 0;
				mqtt_params.keepalive_interval_ms = 60000;
				mqtt_params.read_buf_size = 1024;
				mqtt_params.write_buf_size = 1024;
			
				mqtt_params.handle_event.h_fp = example_event_handle;
				mqtt_params.handle_event.pcontext = NULL;

				g_mqtt_handle = IOT_MQTT_Construct(&mqtt_params);
				if (g_mqtt_handle != NULL){
					g_mqtt_connect_status = 1;
					DEBUG_TRACE("IOT_MQTT_Construct Success");
				}
	        }

	    } 
		else if (res == SOC_WOULDBLOCK)         // block
	    {
	        DEBUG_TRACE("socket_host_by_name SOC_WOULDBLOCK");
	    }
	    else                                    // error
	    {
	        soc_gethostbyname(KAL_FALSE,MOD_MQTT,g_ali_request_id,g_mqtt_signout.hostname, 
				 (kal_uint8 *)addr_buff, &addr_len,0,g_ali_nwk_account_id);
	    }
	}

	kal_prompt_trace(MOD_NIL, "hello world");
	
	//StartTimer(MYAPP_START_TIMER,1000*5,ali_app_start_fun);
}

#define     ALIYUN_NETWORK_GPRS_OK          3
//raymond add for mycommon network status start 20181222
#define     ALIYUN_NETWORK_SEARCHING          0
#define     ALIYUN_NETWORK_NO_SERVICE          1
#define     ALIYUN_NETWORK_LIMITED_SERVICE          2


void aliyun_network_changed(kal_int8 status)
{
    /* srv_nw_info_location_info_struct info; */
    DEBUG_TRACE("service_changedstatus.status=%d",status);

    g_gprs_status = status;
    if (g_gprs_status == ALIYUN_NETWORK_GPRS_OK) 
    {
        if(g_sim_init_complete == KAL_FALSE)
        {
            g_sim_init_complete = KAL_TRUE;
			StopTimer(ALIYUN_START_TIMER);
    		StartTimer(ALIYUN_START_TIMER,1000*20,ali_app_start_fun);
			
        }
    }
	//raymond add for mycommon network status start 20181222
	else if(status == ALIYUN_NETWORK_SEARCHING)
	{

	}
	else if(status == ALIYUN_NETWORK_NO_SERVICE)
	{

	}
	else if(status == ALIYUN_NETWORK_LIMITED_SERVICE)
	{

	}
	//raymond add for mycommon network status end 20181222
}

#endif
