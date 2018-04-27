#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/crypto.h>
#include "hash.h"

// Creates an MD5 hash based on the packet payload
void hash_payload(char * input, char *output, size_t len)
{
  struct scatterlist sg;
  struct hash_desc desc;
  
  sg_init_one(&sg, input, len);
  desc.tfm = crypto_alloc_hash("md5", 0, CRYPTO_ALG_ASYNC);
  
  crypto_hash_init(&desc);
  crypto_hash_update(&desc, &sg, len);
  crypto_hash_final(&desc, output);
  
  crypto_free_hash(desc.tfm);
}
