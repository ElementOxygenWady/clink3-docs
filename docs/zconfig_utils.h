/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef __ZCONFIG_UTILS_H__
#define __ZCONFIG_UTILS_H__


#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
extern "C"
{
#endif
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

void dump_mac(uint8_t *src, uint8_t *dst);
void dump_hex(uint8_t *data, int len, int tab_num);
void dump_ascii(uint8_t *data, int len, int tab_num);

uint16_t zconfig_checksum_v3(uint8_t *data, uint8_t len);
char is_utf8(const char *ansi_str, int length);


/* link type */
enum AWS_LINK_TYPE {
    AWS_LINK_TYPE_NONE,
    AWS_LINK_TYPE_PRISM,
    AWS_LINK_TYPE_80211_RADIO,
    AWS_LINK_TYPE_80211_RADIO_AVS
};

typedef enum AWS_LINK_TYPE  aws_link_type_t;

#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
}
#endif

#endif
