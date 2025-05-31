#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct heap_list {
    size_t len;
    size_t _cap;
    void** buf;
};

struct heap_list hl_make();

void* hl_get(const struct heap_list* hl, size_t i);

void hl_resize(struct heap_list* hl, size_t n);

void hl_resize(struct heap_list* hl, size_t n);

void hl_append(struct heap_list* hl, void* x);

void hl_destroy(struct heap_list* hl);

struct string_map_entry {
    char* key;
    void* value;
    uint32_t _hash; // cache so we don't have to recompute when resizing
    struct string_map_entry* next;
};

struct string_map {
    size_t _count;
    size_t _entries;
    struct string_map_entry** table;
};

struct string_map sm_make();

void* sm_get(const struct string_map* sm, const char* key);

void sm_put(struct string_map* sm, const char* key, void* value);

void sm_destroy(struct string_map* sm, bool free_values);

void sm_print(const struct string_map* sm);

void sm_foreach(const struct string_map* sm,
        void (*func)(void*, const char*, void*), void* global);

#endif