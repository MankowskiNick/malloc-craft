#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Generic hashmap Macro Definition
#define DEFINE_HASHMAP(name, key_type, value_type, hash_func, equals_func)             \
                                                                                      \
    typedef struct name##_entry {                                                      \
        key_type key;                                                                 \
        value_type value;                                                             \
        struct name##_entry* next;                                                    \
    } name##_entry;                                                                   \
                                                                                      \
    typedef struct {                                                                  \
        name##_entry** buckets;                                                       \
        size_t capacity;                                                              \
        size_t size;                                                                 \
    } name##_hashmap;                                                                 \
                                                                                      \
    name##_hashmap name##_init(size_t capacity) {                                     \
        name##_hashmap map;                                                           \
        map.buckets = calloc(capacity, sizeof(name##_entry*));                        \
        map.capacity = capacity;                                                      \
        map.size = 0;                                                                 \
        return map;                                                                   \
    }                                                                                 \
                                                                                      \
    void name##_insert(name##_hashmap* map, key_type key, value_type value) {         \
        size_t index = hash_func(key) % map->capacity;                                \
        name##_entry* current = map->buckets[index];                                  \
                                                                                      \
        while (current) {                                                             \
            if (equals_func(current->key, key)) {                                     \
                current->value = value;                                               \
                return;                                                               \
            }                                                                        \
            current = current->next;                                                  \
        }                                                                            \
                                                                                      \
        name##_entry* new_entry = malloc(sizeof(name##_entry));                       \
        new_entry->key = key;                                                         \
        new_entry->value = value;                                                     \
        new_entry->next = map->buckets[index];                                        \
        map->buckets[index] = new_entry;                                              \
        map->size++;                                                                  \
    }                                                                                 \
                                                                                      \
    value_type* name##_get(name##_hashmap* map, key_type key) {                       \
        size_t index = hash_func(key) % map->capacity;                                \
        name##_entry* current = map->buckets[index];                                  \
                                                                                      \
        while (current) {                                                             \
            if (equals_func(current->key, key)) {                                     \
                return &current->value;                                               \
            }                                                                        \
            current = current->next;                                                  \
        }                                                                            \
        return NULL;                                                                  \
    }                                                                                 \
                                                                                      \
    bool name##_remove(name##_hashmap* map, key_type key) {                           \
        size_t index = hash_func(key) % map->capacity;                                \
        name##_entry* current = map->buckets[index];                                  \
        name##_entry* prev = NULL;                                                    \
                                                                                      \
        while (current) {                                                             \
            if (equals_func(current->key, key)) {                                     \
                if (prev) {                                                           \
                    prev->next = current->next;                                       \
                } else {                                                              \
                    map->buckets[index] = current->next;                              \
                }                                                                    \
                free(current);                                                        \
                map->size--;                                                          \
                return true;                                                          \
            }                                                                        \
            prev = current;                                                           \
            current = current->next;                                                  \
        }                                                                            \
        return false;                                                                \
    }                                                                                 \
                                                                                      \
    void name##_free(name##_hashmap* map) {                                           \
        for (size_t i = 0; i < map->capacity; ++i) {                                  \
            name##_entry* current = map->buckets[i];                                  \
            while (current) {                                                         \
                name##_entry* temp = current;                                         \
                current = current->next;                                              \
                free(temp);                                                           \
            }                                                                        \
        }                                                                            \
        free(map->buckets);                                                           \
    }
#endif