#ifndef _CIRCBUF_H
#define _CIRCBUF_H

#include "packet.h"

// Definition of the circular buffer structure that will store processing times
typedef struct {
  u32 * buffer;
  bool lock;
  int head;
  int tail;
  int maxLen;
} circBuf_u32_t;

typedef struct {
  packet_t * buffer;
  bool lock;
  int head;
  int tail;
  int maxLen;
} circBuf_packet_t;

// Pushes a data value into the circular buffer
int circBuf_u32_Push(circBuf_u32_t *c, u32 data);
// Pops the oldest element from the buffer
int circBuf_u32_Pop(circBuf_u32_t *c, u32 *data);
// Resets head and tail to 0 effectively clearing the buffer
int circBuf_u32_Clear(circBuf_u32_t *c);
// Lock and unlock functions
int circBuf_u32_Lock(circBuf_u32_t *c);
int circBuf_u32_Unlock(circBuf_u32_t *c);

// Pushes a data value into the circular buffer
int circBuf_packet_Push(circBuf_packet_t *c, packet_t data);
// Pops the oldest element from the buffer
int circBuf_packet_Pop(circBuf_packet_t *c, packet_t *data);
// Get without Pop
int circBuf_packet_Get(circBuf_packet_t *c, packet_t *data);
// Resets head and tail to 0 effectively clearing the buffer
int circBuf_packet_Clear(circBuf_packet_t *c);
//Lock and unlock functons
int circBuf_packet_Lock(circBuf_packet_t *c);
int circBuf_packet_Unlock(circBuf_packet_t *c);

#define INIT_CIRCBUF_U32(SIZE) (circBuf_u32_t){.head = 0, .tail = 0, .maxLen = SIZE, .buffer = kcalloc(SIZE, sizeof(u32), GFP_KERNEL), .lock = false}
#define INIT_CIRCBUF_PACKET(SIZE) (circBuf_packet_t){.head = 0, .tail = 0, .maxLen = SIZE, .buffer = kcalloc(SIZE, sizeof(packet_t), GFP_KERNEL), .lock = false} 

#endif
