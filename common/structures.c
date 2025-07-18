#include "structures.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

struct string_map sm_make() {
    struct string_map sm = {
        ._count = 0,
        ._entries = 0,
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

bool sm_haskey(const struct string_map* sm, const char* key) {
    if(sm->_entries == 0) return false;

    uint32_t hash = sm_hash(key);
    struct string_map_entry* e = sm->table[hash % sm->_entries];
    while(1) {
        if(e == NULL) {
            return false;
        }
        // check the hash first to avoid strcmp when we know it'll fail
        if(e->_hash == hash && !strcmp(key, e->key)) {
            return true;
        }
        e = e->next;
    }
}

void* sm_get(const struct string_map* sm, const char* key) {
    if(sm->_entries == 0) return NULL;

    uint32_t hash = sm_hash(key);
    struct string_map_entry* e = sm->table[hash % sm->_entries];
    while(1) {
        if(e == NULL) {
            return NULL;
        }
        // check the hash first to avoid strcmp when we know it'll fail
        if(e->_hash == hash && !strcmp(key, e->key)) {
            return e->value;
        }
        e = e->next;
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

void sm_put(struct string_map* sm, const char* key, void* value) {
    // very crude load factor of 1 saves us from doing floating point
    if(sm->_count >= sm->_entries) {
        sm_expand(sm);
    }

    uint32_t hash = sm_hash(key);

    struct string_map_entry* newent = malloc(sizeof(struct string_map_entry));
    // make sure our key can outlive the key we get passed
    newent->key = malloc(strlen(key) + 1);
    // assuming malloc doesn't fail, this should be a guaranteed safe strcpy
    strcpy(newent->key, key);
    newent->value = value;
    newent->_hash = hash;
    newent->next = NULL;

    struct string_map_entry** e = &sm->table[hash % sm->_entries];
    sm_putent(e, newent);
    sm->_count ++;
}

static void sm_destroy_ent(struct string_map_entry* e, bool free_values) {
    if(e) {
        free(e->key);
        if(free_values) free(e->value);
        struct string_map_entry* next = e->next;
        free(e);
        // make it tail recursive for good vibes
        sm_destroy_ent(next, free_values);
    }
}

void sm_destroy(struct string_map* sm, bool free_values) {
    for(size_t i = 0; i < sm->_entries; i++) {
        sm_destroy_ent(sm->table[i], free_values);
    }
    if(sm->table) free(sm->table);
}


static void sm_printent(const struct string_map_entry* e) {
    if(e) {
        printf("%s:%p -> ", e->key, e->value);
        sm_printent(e->next);
    } else {
        printf("NULL");
    }
}

void sm_print(const struct string_map* sm) {
    for(size_t i = 0; i < sm->_entries; i++) {
        sm_printent(sm->table[i]);
        printf("\n");
    }
}

void sm_foreach(const struct string_map* sm,
        void (*func)(void*, const char*, void*), void* global) {
    for(size_t i = 0; i < sm->_entries; i++) {
        struct string_map_entry* e = sm->table[i];
        while(e) {
            func(global, e->key, e->value);
            e = e->next;
        }
    }
}

struct string_map arr_inv_to_sm(const char **arr, size_t len) {
    struct string_map sm = sm_make();
    for(size_t i = 0; i < len; i++) {
        if(arr[i]) sm_put(&sm, arr[i], (void*) i);
    }
    return sm;
}

struct string_map hl_inv_to_sm(const struct heap_list* hl) {
    return arr_inv_to_sm((const char**) hl->buf, hl->len);
}