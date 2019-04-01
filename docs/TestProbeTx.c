/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */


#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
extern "C"
{
#endif
#ifndef ZERO_CONFIG_SA_OFFSET
#define ZERO_CONFIG_SA_OFFSET           (10)
#endif

#ifndef ZERO_CONFIG_PROBE_PKT_LEN
#define ZERO_CONFIG_PROBE_PKT_LEN       (117)
#endif

#ifndef ZERO_CONFIG_MONITOR_TIMEOUT_MS
#define ZERO_CONFIG_MONITOR_TIMEOUT_MS  (1 * 60 * 1000)
#endif
#ifndef OS_MAC_LEN
#define  OS_MAC_LEN  (17 + 1)
#endif

static const unsigned char zero_config_probe_req_frame[ZERO_CONFIG_PROBE_PKT_LEN] = {
0x40, 0x0,  /* mgnt type, frame control */
0x0, 0x0,  /* duration */
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* DA */
0xb0, 0xf8, 0x93, 0x10, 0x58, 0x1f, /* SA, to be replaced with wifi mac */
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* BSSID */
0xc0, 0x79, /* seq */ 
0x0, 0x0,/* hidden ssid */
0x1, 0x8, 0x82, 0x84, 0x8b, 0x96, 0x8c, 0x92, 0x98, 0xa4,/* supported rates */
0x32, 0x4, 0xb0, 0x48, 0x60, 0x6c,  /* extended supported rates */
0xdd, 0x45, 0xd8, 0x96, 0xe0, 0xaa, 0x1, 0xa, 0x74, 0x74, 0x5f, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x30, 0x31, 0x0, 0xb, 0x61, 0x31, 0x58, 0x32, 0x62, 0x45, 0x6e, 0x50, 0x38, 0x32, 0x7a, 0x10, 0xf4, 0xe5, 0x2c, 0x43, 0x31, 0x62, 0xfc, 0x21, 0x74, 0x2f, 0x32, 0x4, 0xb0, 0xb3, 0xf5, 0xda, 0x4, 0x0, 0x14, 0x9f, 0xc5, 0x25, 0x7f, 0xf6, 0xec, 0xe0, 0xa3, 0xfe, 0xbc, 0xd, 0x3e, 0x26, 0x9e, 0x61, 0x42, 0xb2, 0x5, 0xc5, 0x34, /* vendor specific */
0x3f, 0x84, 0x10, 0x9e /* FCS */
};

static char local_mac[18]= "b0:f8:93:10:58:1f";
char *demo_os_wifi_get_mac_str(char mac_str[OS_MAC_LEN])
{
    char *str = local_mac; 
    int colon_num = 0, i;
    
    /* sanity check */
    while (str) {
        str = strchr(str, ':');
        if (str) {
            colon_num++;
            str++; /* eating char ':' */
        }
    }

    /* convert to capital letter */
    for (i = 0; i < OS_MAC_LEN && mac_str[i]; i ++) {
        if ('a' <= mac_str[i] && mac_str[i] <= 'z') {
            mac_str[i] -= 'a' - 'A';
        }
    }

    return mac_str;
}


extern HAL_Wifi_Send_80211_Raw_Frame(int frameType, unsigned char * probe, int len);

int verify_raw_frame(void)
{
    unsigned char probe[ZERO_CONFIG_PROBE_PKT_LEN];
    memcpy(probe, zero_config_probe_req_frame, sizeof(probe));
    demo_os_wifi_get_mac_str(&probe[ZERO_CONFIG_SA_OFFSET]);
    HAL_Wifi_Send_80211_Raw_Frame(2, probe, sizeof(probe));
    return 0;
}

unsigned char ref_probe[ZERO_CONFIG_PROBE_PKT_LEN] = {0};
unsigned char * get_ref_buffer() {
    memcpy(ref_probe, zero_config_probe_req_frame, sizeof(ref_probe));
    demo_os_wifi_get_mac_str(&ref_probe[ZERO_CONFIG_SA_OFFSET]);
    return ref_probe;
}

#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
}
#endif
