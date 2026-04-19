#include "structures.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define KEY_BUFFER_PREALLOC 16

struct heap_list hl_make() {
    struct heap_list hl = {
        .buf = NULL,
        .len = 0,
        ._cap = 0
    };
    return hl;
}

void* hl_get(const struct heap_list* hl, size_t i) {
    return hl->buf[i];
}

void hl_append(struct heap_list* hl, void* x) {
    if(hl->_cap == 0) {
        hl->_cap = 1;
        hl->buf = malloc(sizeof(void*));
    } else if(hl->len == hl->_cap) {
        hl->_cap *= 2;
        hl->buf = realloc(hl->buf, hl->_cap * sizeof(void*));
    }
    hl->buf[hl->len] = x;
    hl->len ++;
}

void hl_destroy(struct heap_list* hl, bool free_values) {
    if(hl->buf) {
        if(free_values) {
            for(size_t i = 0; i < hl->len; i++) {
                if(hl->buf[i]) free(hl->buf[i]);
            }
        }
        free(hl->buf);
    }
    hl->buf = NULL;
    hl->len = 0;
    hl->_cap = 0;
}

struct string_map_entry {
    size_t key_offs;
    void* value;
    uint32_t _hash; // cache so we don't have to recompute when resizing
    struct string_map_entry* next;
    bool value_heap;
};

struct string_map sm_make() {
    struct string_map sm = {
        ._count = 0,
        ._entries = 0,
        ._key_buffer = NULL,
        ._key_buffer_len = 0,
        ._key_buffer_cap = 0,
        .table = NULL
    };
    return sm;
}

static uint32_t sm_hash(const char* str) {
    // ElfHash via Wikipedia https://en.wikipedia.org/wiki/PJW_hash_function
    uint32_t h = 0, high;
    while (*str) {
        h = (h << 4) + *str;
        high = h & 0xF0000000;
        if(high) h ^= high >> 24;
        h &= ~high;
        str++;
    }
    return h;
}

static struct string_map_entry* sm_getent(const struct string_map* sm, const char* key) {
    if(sm->_entries == 0) return NULL;
    uint32_t hash = sm_hash(key);
    struct string_map_entry* e = sm->table[hash % sm->_entries];
    while(1) {
        if(e == NULL) return NULL;
        // check the hash first to avoid strcmp when we know it'll fail
        if(e->_hash == hash && !strcmp(key, sm->_key_buffer + e->key_offs)) {
            return e;
        }
        e = e->next;
    }
}

bool sm_haskey(const struct string_map* sm, const char* key) {
    return !!sm_getent(sm, key);
}

void* sm_get(const struct string_map* sm, const char* key) {
    struct string_map_entry* e = sm_getent(sm, key);
    if(e) {
        return e->value;
    } else {
        return NULL;
    }
}

static void sm_putent(struct string_map_entry** location, struct string_map_entry* ent) {
    // Walk down the linked list until we find an entry that's null
    while(*location) location = &((*location)->next);
    *location = ent;
}

static void sm_expand(struct string_map* sm) {
    if(sm->_entries == 0) {
        sm->_entries = 1;
        sm->table = malloc(sizeof(struct string_map_entry*));
        *sm->table = NULL;
    } else {
        // Stash old table for rearrangement
        struct string_map_entry** old_table = sm->table;
        size_t old_ent = sm->_entries;
        // Make a new table, initially all null
        sm->_entries *= 2;
        sm->table = malloc(sm->_entries * sizeof(struct string_map_entry*));
        memset(sm->table, 0, sm->_entries * sizeof(struct string_map_entry*));
        // Move all of our old entries into the new table
        for(size_t i = 0; i < old_ent; i++) {
            struct string_map_entry* current = old_table[i];
            while(current) {
                uint32_t newind = current->_hash % sm->_entries;
                struct string_map_entry* next = current->next;
                current->next = NULL;
                sm_putent(sm->table + newind, current);
                current = next;
            }
        }
        free(old_table);
    }
}

void sm_put(struct string_map* sm, const char* key, void* value, bool value_heap) {
    // very crude load factor of 1 saves us from doing floating point
    if(sm->_count >= sm->_entries) {
        sm_expand(sm);
    }

    uint32_t hash = sm_hash(key);

    struct string_map_entry* newent = malloc(sizeof(struct string_map_entry));
    // make sure our key can outlive the key we get passed
    if(!sm->_key_buffer) {
        sm->_key_buffer = malloc(sizeof(char)*KEY_BUFFER_PREALLOC);
        sm->_key_buffer_cap = KEY_BUFFER_PREALLOC;
    }
    size_t key_len = strlen(key) + 1;
    // in case we allocate a really big key that requires multiple resizes
    while(sm->_key_buffer_len + key_len > sm->_key_buffer_cap) {
        sm->_key_buffer_cap *= 2;
        sm->_key_buffer = realloc(sm->_key_buffer, sm->_key_buffer_cap);
    }
    // assuming malloc doesn't fail, this should be a guaranteed safe strcpy
    newent->key_offs = sm->_key_buffer_len;
    strcpy(sm->_key_buffer + sm->_key_buffer_len, key);
    sm->_key_buffer_len += key_len;
    newent->value = value;
    newent->value_heap = value_heap;
    newent->_hash = hash;
    newent->next = NULL;

    struct string_map_entry** e = &sm->table[hash % sm->_entries];
    sm_putent(e, newent);
    sm->_count ++;
}

static void sm_destroy_ent(struct string_map_entry* e) {
    if(e) {
        if(e->value_heap) free(e->value);
        struct string_map_entry* next = e->next;
        free(e);
        // make it tail recursive for good vibes
        sm_destroy_ent(next);
    }
}

void sm_destroy(struct string_map* sm) {
    for(size_t i = 0; i < sm->_entries; i++) {
        sm_destroy_ent(sm->table[i]);
    }
    if(sm->table) {
        free(sm->table);
        sm->table = NULL;
    }
    if(sm->_key_buffer) {
        free(sm->_key_buffer);
        sm->_key_buffer = NULL;
        sm->_key_buffer_len = 0;
        sm->_key_buffer_cap = 0;
    }
}


static void sm_printent(const struct string_map_entry* e, const char* key_buf) {
    if(e) {
        printf("%s:%p -> ", key_buf + e->key_offs, e->value);
        sm_printent(e->next, key_buf);
    } else {
        printf("NULL");
    }
}

void sm_print(const struct string_map* sm) {
    for(size_t i = 0; i < sm->_entries; i++) {
        sm_printent(sm->table[i], sm->_key_buffer);
        printf("\n");
    }
}

size_t sm_stsize(const struct string_map* sm) {
    return sm->_key_buffer_len;
}

const char* sm_stringtable(const struct string_map* sm) {
    return sm->_key_buffer;
}

size_t sm_stoffs(const struct string_map* sm, const char* key) {
    if(key >= sm->_key_buffer && key < sm->_key_buffer + sm->_key_buffer_len) {
        // Fast path for cases where the string is literally inside the string
        // table
        return key - sm->_key_buffer;
    }
    struct string_map_entry* ent = sm_getent(sm, key);
    if(ent) {
        return ent->key_offs;
    } else {
        return -1;
    }
}

void sm_foreach(const struct string_map* sm,
        void (*func)(void*, const char*, void*), void* global) {
    for(size_t i = 0; i < sm->_entries; i++) {
        struct string_map_entry* e = sm->table[i];
        while(e) {
            func(global, sm->_key_buffer + e->key_offs, e->value);
            e = e->next;
        }
    }
}

size_t sm_size(const struct string_map* sm) {
    return sm->_count;
}

void sm_filter(struct string_map* sm, bool (*func)(const char*, const void*)) {
    for(size_t i = 0; i < sm->_entries; i++) {
        struct string_map_entry** eptr = sm->table + i;
        while(*eptr) {
            if(!func(sm->_key_buffer + (*eptr)->key_offs, (*eptr)->value)) {
                struct string_map_entry* e = *eptr;
                *eptr = e->next;
                e->next = NULL;
                sm_destroy_ent(e);
            } else {
                eptr = &((*eptr)->next);
            }
        }
    }
}

struct string_map arr_inv_to_sm(const char **arr, size_t len) {
    struct string_map sm = sm_make();
    for(size_t i = 0; i < len; i++) {
        if(arr[i]) sm_put(&sm, arr[i], (void*) i, false);
    }
    return sm;
}

struct string_map hl_inv_to_sm(const struct heap_list* hl) {
    return arr_inv_to_sm((const char**) hl->buf, hl->len);
}