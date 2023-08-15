#ifndef __HASH_H_
#define __HASH_H_

#include <stdint.h>
#include "util.h"
#define HASH_MAX_BUCKETS 4096

typedef struct hash_t_ {
    list_t *buckets[HASH_MAX_BUCKETS];
    uint32_t key_size;
} hash_t;

uint32_t calculate_hash(void *key, uint32_t size);
bool hash_init(void *lookup_cb, uint32_t key_size);
bool hash_insertconst (hash_t *hash, void *key, void *val);
bool hash_removeconst (hash_t *hash, void *key);
void *hash_lookup(const hash_t *hash, void *key);
#endif
