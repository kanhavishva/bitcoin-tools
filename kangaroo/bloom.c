#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bloom.h"
#include <stdio.h>

struct bloom_filter {
    void *bits;
    size_t size;
};


bloom_t bloom_create(size_t sizebits) {
    if (sizebits < 1 || sizebits > 64) {
        fprintf(stderr,"Incorrect bloom filter size %ld bits\n", sizebits);
        exit(-1);
    }
    size_t size;
    bloom_t res = calloc(1, sizeof(struct bloom_filter));
    size = ((uint64_t)1<<sizebits);
    res->size = size;
    res->bits = malloc(size);
    if (res->bits==NULL) {
        fprintf(stderr,"Failed to create bloom filter\n");
        exit(1);
	}
    return res;
}


void bloom_free(bloom_t filter) {
    if (filter) {
        free(filter->bits);
        free(filter);
    }
}


void bloom_add(bloom_t filter, const void *item) {
    uint8_t *bits = filter->bits;
    uint64_t hash = *((uint64_t*)item);
    //printf("ADD HASH %08lx\n", hash);
    hash %= filter->size * 8;
    //printf("ADD HASH %08lx\n", hash);
    bits[hash / 8] |= 1 << hash % 8;
}


bool bloom_test(bloom_t filter, const void *item) {
    uint8_t *bits = filter->bits;
    uint64_t hash = *((uint64_t*)item);
    hash %= filter->size * 8;
    //printf("TESTING HASH %08lx\n", hash);
    if (!(bits[hash / 8] & 1 << hash % 8)) {
        return false;
    }
    return true;
}
