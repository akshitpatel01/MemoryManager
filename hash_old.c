#include "hash.h"
#include "util.h"
#include <stdint.h>
#include <stdlib.h>

bool hash_init(void *lookup_cb, uint32_t key_size)
{
    hash_t *hash = NULL;
    uint i;

    hash = calloc(1, sizeof(hash_t));
    if (hash == NULL) {
        return false;
    }

    hash->key_size = key_size;
    for (i = 0; i < HASH_MAX_BUCKETS; i++) {
        hash->buckets[i] = list_init(lookup_cb);    
    }

    return true;
}

bool hash_insert(const hash_t* hash, void *key, void *val)
{
    uint32_t bucket_ind = calculate_hash(key, hash->key_size);
    return list_insert_head(hash->buckets[bucket_ind], val);
}

bool hash_remove(const hash_t *hash, void *key)
{
    uint32_t bucket_ind = calculate_hash(key, hash->key_size);
    return list_remove_entry(hash->buckets[bucket_ind], key, false);
}

void* hash_lookup(const hash_t *hash, void *key)
{
    uint32_t bucket_ind = calculate_hash(key, hash->key_size);
    return list_lookup(hash->buckets[bucket_ind], key);
}

uint32_t calculate_hash(void *key, uint32_t size)
{
    unsigned long hash = 5381;
    int c;

    while (size--) {
        c = (*(char*)key++);
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return (hash % HASH_MAX_BUCKETS);
}
