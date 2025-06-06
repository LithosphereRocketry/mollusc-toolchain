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

/**
 * Returns whether the given key is present in the string_map. A key whose 
 * corresponding value is NULL is still considered present.
 */
bool sm_haskey(const struct string_map* sm, const char* key);

/**
 * Retrieve the value associated with the given key. If the key isn't present,
 * returns NULL; note that this is not uniquely distinguishable from a value
 * of (void*) NULL. For that, use sm_haskey.
 */
void* sm_get(const struct string_map* sm, const char* key);

/**
 * Associate the given value with the given key. If the key is already present,
 * set its value to what is given.
 * 
 * NOTE: overwriting a heap-allocated value causes it to be leaked; to be fixed
 * in future revisions.
 */
void sm_put(struct string_map* sm, const char* key, void* value);

/**
 * Free the memory allocated by the stringmap. Frees value pointers if
 * free_values is set; otherwise assumes them to be managed elsewhere or type-
 * punned (e.g. (void*) some_sizet). Does not free sm itself if it is heap-
 * allocated; do that manually with free().
 */
void sm_destroy(struct string_map* sm, bool free_values);

// Print the internal representation of the stringmap. For debugging.
void sm_print(const struct string_map* sm);

// UTILITY FUNCTIONS

/**
 * Run a function of the form func(void* global, const char* key, void* value)
 * on each key-value pair of the stringmap.
 */
void sm_foreach(const struct string_map* sm,
        void (*func)(void*, const char*, void*), void* global);

/**
 * Produces a punned size_t map from elements to indices of the given array.
 * Assumes the values of the array are null-terminated strings; skips NULL
 * entries. If duplicates appear, will select the last one in order.
 */
struct string_map arr_inv_to_sm(const char** arr, size_t len);

/**
 * Produces a punned size_t map from elements to indices of the given heap_list.
 * Assumes the values of the heap_list are null-terminated strings; skips NULL
 * entries. If duplicates appear, will select the last one in order.
 */
struct string_map hl_inv_to_sm(const struct heap_list* hl);

#endif