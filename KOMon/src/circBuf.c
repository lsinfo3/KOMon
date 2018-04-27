#include <linux/kernel.h>
#include <linux/module.h>
#include "circBuf.h"

// Called to push a value into the circular buffer
int circBuf_u32_Push(circBuf_u32_t *c, u32 data)
{
  // next is where head will point to after this write.
  if (!c->lock) {
    int next = c->head + 1;
    if (next >= c->maxLen)
      next = 0;
    
    if (next == c->tail)
      c->tail = c->tail + 1;
    
    if (c->tail >= c->maxLen)
      c->tail = 0;
    
    c->buffer[c->head] = data;
    c->head = next;
    return 0;
  } else printk(KERN_ERR "Error circBuf_u32_Push: Locked\n");
  return 1;
}

int circBuf_u32_Pop(circBuf_u32_t *c, u32 *data)
{
  if (!c->lock) {
    int next = c->tail + 1;
    
    if (c->head == c->tail)
      return 1;
    
    if (next >= c->maxLen)
      next = 0;
    
    *data = c->buffer[c->tail];
    c->buffer[c->tail] = 0;
    
    c->tail = next;
    return 0;
  } else printk(KERN_ERR "Error circBuf_u32_Pop: Locked\n");
  return 1;
}

int circBuf_u32_Clear(circBuf_u32_t *c)
{
  if (!c->lock) {
    c->head = 0;
    c->tail = 0;
    return 0;
  }
  return 1;
}

int circBuf_u32_Lock(circBuf_u32_t *c)
{
  if (c->lock)
    return 1;
  c->lock = true;
  printk(KERN_INFO "circBuf_u32_Lock\n");
  return 0;
}

int circBuf_u32_Unlock(circBuf_u32_t *c)
{
  if (!c->lock)
    return 1;
  c->lock = false;
  printk(KERN_INFO "circBuf_u32_Unlock\n");
  return 0;
}


// Called to push a value into the circular buffer
int circBuf_packet_Push(circBuf_packet_t *c, packet_t data)
{
  if (!c->lock) {
    int next = c->head + 1;
    if (next >= c->maxLen)
      next = 0;
    
    if (next == c->tail)
      c->tail = c->tail + 1;
    
    if (c->tail >= c->maxLen)
      c->tail = 0;
    
    c->buffer[c->head] = data;
    c->head = next;
    return 0;
  } else printk(KERN_ERR "Error circBuf_packet_Push: Locked\n");
  return 1;
}

int circBuf_packet_Pop(circBuf_packet_t *c, packet_t *data)
{
  if (!c->lock) {
    int next = c->tail + 1;
    
    if (c->head == c->tail)
      return 1;
    
    if (next >= c->maxLen)
      next = 0;
    
    *data = c->buffer[c->tail];
    c->tail = next;
    return 0;
  } else printk(KERN_ERR "Error circBuf_packet_Pop: Locked\n");
  return 1;
}

int circBuf_packet_Get(circBuf_packet_t *c, packet_t *data)
{
  if (!c->lock) {
    if (c->head == c->tail)
      return 1;
    
    *data = c->buffer[c->tail];
    return 0;
  } else printk(KERN_ERR "Error circBuf_packet_Pop: Locked\n");
  return 1;
}

int circBuf_packet_Clear(circBuf_packet_t *c)
{
  if (!c->lock) {
    c->head = 0;
    c->tail = 0;
    return 0;
  }
  return 1;
}

int circBuf_packet_Lock(circBuf_packet_t *c)
{
  if (c->lock)
    return 1;
  c->lock = true;
  printk(KERN_INFO "circBuf_packet_Lock\n");
  return 0;
}

int circBuf_packet_Unlock(circBuf_packet_t *c)
{
  if (!c->lock)
    return 1;
  c->lock = false;
  printk(KERN_INFO "circBuf_packet_Unlock\n");
  return 0;
}
