#ifndef _PACKET_T_H
#define _PACKET_T_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/ktime.h>

// Struct that represents a single packet
typedef struct {
  int id;
  u32 src_ip;
  u32 dst_ip;
  u16 src_port;
  u16 dst_port;
  u8 protocol;
  unsigned int digest;
  ktime_t timestamp;
} packet_t;

#endif
