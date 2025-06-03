#include "hash.h"

uint32_t jenkins_hash(const char *str)
{
    uint32_t hash = 0;

    while (*str) {
        hash += (unsigned char)(*str++);
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}