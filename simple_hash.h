/*
 * A simple open-addressing hash table implementation using linear probing.
 *
 * The hidden header is stored immediately before the user array and includes:
 *   - count:          Number of elements stored in the table.
 *   - capacity:       Total number of buckets.
 *   - load_factor:    Maximum ratio of filled buckets to capacity before resizing.
 *   - growth_factor:  Multiplier used to increase capacity when resizing.
 *
 * Default configuration:
 *   - HASH_INIT_CAPACITY:          Initial number of buckets.
 *   - HASH_LOAD_FACTOR:            Default load factor threshold.
 *   - HASH_GROWTH_FACTOR_DEFAULT:  Default multiplier for table expansion.
 *
 * Usage notes:
 *   - The table’s element type must have a field named `key` (of type const char*)
 *     as its first member.
 *
 * Public API macros:
 *   - hash_count(tbl):                      Gets the number of elements in the table.
 *   - hash_capacity(tbl):                   Gets the capacity of the table.
 *   - hash_load_factor:                     Gets the load factor of the table.
 *   - hash_growth_factor:                   Gets the growth factor of the table.
 *   - hash_put(tbl, item):                  Inserts or updates an element.
 *   - hash_get(tbl, key):                   Retrieves a pointer to an element with the given key.
 *   - hash_delete(tbl, key):                Removes the element with the given key.
 *   - hash_set_min_capacity(tbl, min_cap):  Ensures a minimum table capacity.
 *   - hash_set_growth_factor(tbl, factor):  Sets the table's growth factor.
 *   - hash_set_load_factor(tbl, factor):    Sets the table's load factor.
 *   - hash_dup(tbl):                        Duplicates the table.
 *   - hash_free(tbl):                       Frees the table.
 */

#ifndef SIMPLE_HASH_H
#define SIMPLE_HASH_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Configuration: initial capacity, load factor, and growth factor */
#ifndef HASH_INIT_CAPACITY
#define HASH_INIT_CAPACITY 16
#endif

#ifndef HASH_LOAD_FACTOR
#define HASH_LOAD_FACTOR 0.75
#endif

#ifndef HASH_GROWTH_FACTOR_DEFAULT
#define HASH_GROWTH_FACTOR_DEFAULT 2.0
#endif

/* Hidden header stored immediately before the user array.
 * Fields:
 *   - count:         Number of elements stored.
 *   - capacity:      Total number of buckets.
 *   - load_factor:   Load factor threshold for resizing.
 *   - growth_factor: Multiplier used for expanding capacity.
 */
typedef struct {
    size_t count;
    size_t capacity;
    double load_factor;
    double growth_factor;
} hash_header;

/* Given a table pointer, HASH_HEADER returns a pointer to its hidden header.
 * The header is stored immediately before the array of elements.
 */
#define HASH_HEADER(tbl) ((hash_header *)((char *)(tbl) - sizeof(hash_header)))

/* Public macros to query table properties */
#define hash_count(tbl)    ((tbl) ? HASH_HEADER(tbl)->count : 0)
#define hash_capacity(tbl) ((tbl) ? HASH_HEADER(tbl)->capacity : 0)
#define hash_load_factor(tbl) ((tbl) ? HASH_HEADER(tbl)->load_factor : HASH_LOAD_FACTOR)
#define hash_growth_factor(tbl) ((tbl) ? HASH_HEADER(tbl)->growth_factor : HASH_GROWTH_FACTOR_DEFAULT)

/* djb2 hash function for string keys */
static inline size_t _hash_string(const char *str) {
    size_t hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
         hash = ((hash << 5) + hash) + c;
    return hash;
}

/* ------------------------------------------------------------------
   Internal function: _hash_get_impl
   Looks up an element in the hash table by key.
   Assumes that the element's first field is a pointer (const char *) holding the key.
   Returns a pointer to the element, or NULL if not found.
------------------------------------------------------------------ */
static inline void *_hash_get_impl(void *tbl_void, const char *key, size_t elem_size) {
    if (!tbl_void)
        return NULL;
    char *tbl = (char *)tbl_void;
    hash_header *hdr = HASH_HEADER(tbl);
    size_t cap = hdr->capacity;
    size_t h = _hash_string(key) % cap;
    size_t start = h;
    while (1) {
         void *elem_ptr = tbl + h * elem_size;
         const char *elem_key = *((const char **)elem_ptr);
         if (!elem_key)
             break;
         if (strcmp(elem_key, key) == 0)
             return elem_ptr;
         h = (h + 1) % cap;
         if (h == start)
             break;
    }
    return NULL;
}

/* ------------------------------------------------------------------
   HASH_RESIZE macro:
   Resizes the hash table to a new capacity.
   Allocates a new block (with the updated capacity), re-inserts all existing
   items using linear probing, copies over the load_factor and growth_factor,
   frees the old block, and updates the table pointer.
------------------------------------------------------------------ */
#define HASH_RESIZE(tbl, new_cap)                                                                                     \
    do {                                                                                                              \
        __typeof__(tbl) _hs_tbl = (tbl);                                                                              \
        size_t _hs_new_cap = (new_cap);                                                                               \
        size_t _hs_elem_size = sizeof(*(_hs_tbl));                                                                    \
        hash_header *_hs_old_hdr = _hs_tbl ? HASH_HEADER(_hs_tbl) : NULL;                                             \
        size_t _hs_old_cap = _hs_tbl ? _hs_old_hdr->capacity : 0;                                                     \
        void *_hs_old_ptr = _hs_tbl ? ((char *)(_hs_tbl) - sizeof(hash_header)) : NULL;                               \
        hash_header *_hs_new_hdr = (hash_header *)malloc(sizeof(hash_header) + _hs_new_cap * _hs_elem_size);          \
        _hs_new_hdr->capacity = _hs_new_cap;                                                                          \
        _hs_new_hdr->count = 0;                                                                                       \
        _hs_new_hdr->load_factor = _hs_old_hdr ? _hs_old_hdr->load_factor : HASH_LOAD_FACTOR;                         \
        _hs_new_hdr->growth_factor = _hs_old_hdr ? _hs_old_hdr->growth_factor : HASH_GROWTH_FACTOR_DEFAULT;           \
        char *_hs_new_arr = (char *)_hs_new_hdr + sizeof(hash_header);                                                \
        memset(_hs_new_arr, 0, _hs_new_cap * _hs_elem_size);                                                          \
        if (_hs_old_ptr) {                                                                                            \
            char *old_items = (char *)_hs_old_ptr + sizeof(hash_header);                                              \
            for (size_t _hs_i = 0; _hs_i < _hs_old_cap; _hs_i++) {                                                    \
                char *cur = old_items + _hs_i * _hs_elem_size;                                                        \
                if (*((const char **)cur) != NULL) {                                                                  \
                    size_t _hs = _hash_string(*((const char **)cur)) % _hs_new_cap;                                   \
                    while (*((const char **)(_hs_new_arr + _hs * _hs_elem_size)) != NULL) {                           \
                        _hs = (_hs + 1) % _hs_new_cap;                                                                \
                    }                                                                                                 \
                    memcpy(_hs_new_arr + _hs * _hs_elem_size, cur, _hs_elem_size);                                    \
                    _hs_new_hdr->count++;                                                                             \
                }                                                                                                     \
            }                                                                                                         \
            free(_hs_old_ptr);                                                                                        \
        }                                                                                                             \
        (tbl) = (void *)_hs_new_arr;                                                                                  \
    } while (0)

/* ------------------------------------------------------------------
   hash_put(tbl, item)
   Inserts a new element (or updates an existing one) into the hash table.
   - If (tbl) is NULL, a new table is allocated with HASH_INIT_CAPACITY.
   - Expects that the element type's first field is a pointer (const char *)
     holding the key.
   Example:
       Foo item = { "apple", 10, 3.14, 'A' };
       hash_put(table, item);
------------------------------------------------------------------ */
#define hash_put(tbl, item)                                                                       \
    do {                                                                                          \
        __typeof__(tbl) _hp_tbl = (tbl);                                                          \
        __typeof__(item) _hp_item = (item);                                                       \
        if (!_hp_tbl) {                                                                           \
            size_t _hp_cap = HASH_INIT_CAPACITY;                                                  \
            size_t _hp_elem_size = sizeof(*(_hp_tbl));                                            \
            hash_header *_hp_hdr = (hash_header *)malloc(sizeof(hash_header) +                    \
                                            _hp_cap * _hp_elem_size);                             \
            _hp_hdr->capacity = _hp_cap;                                                          \
            _hp_hdr->count = 0;                                                                   \
            _hp_hdr->load_factor = HASH_LOAD_FACTOR;                                              \
            _hp_hdr->growth_factor = HASH_GROWTH_FACTOR_DEFAULT;                                  \
            _hp_tbl = (void *)((char *)_hp_hdr + sizeof(hash_header));                            \
            memset(_hp_tbl, 0, _hp_cap * _hp_elem_size);                                          \
        }                                                                                         \
        hash_header *_hp_hdr = HASH_HEADER(_hp_tbl);                                              \
        if ((_hp_hdr->count + 1) >= (size_t)(_hp_hdr->capacity * _hp_hdr->load_factor)) {         \
            size_t _hp_new_cap = (size_t)(_hp_hdr->capacity * _hp_hdr->growth_factor);            \
            if (_hp_new_cap <= _hp_hdr->capacity) _hp_new_cap = _hp_hdr->capacity + 1;            \
            HASH_RESIZE(_hp_tbl, _hp_new_cap);                                                    \
            _hp_hdr = HASH_HEADER(_hp_tbl);                                                       \
        }                                                                                         \
        size_t _hp_cap = _hp_hdr->capacity;                                                       \
        size_t _hp_h = _hash_string(_hp_item.key) % _hp_cap;                                      \
        while (_hp_tbl[_hp_h].key) {                                                              \
            if (strcmp(_hp_tbl[_hp_h].key, _hp_item.key) == 0) {                                  \
                _hp_tbl[_hp_h] = _hp_item;                                                        \
                break;                                                                            \
            }                                                                                     \
            _hp_h = (_hp_h + 1) % _hp_cap;                                                        \
        }                                                                                         \
        if (!_hp_tbl[_hp_h].key) {                                                                \
            _hp_tbl[_hp_h] = _hp_item;                                                            \
            _hp_hdr->count++;                                                                     \
        }                                                                                         \
        (tbl) = _hp_tbl;                                                                          \
    } while (0)

/* ------------------------------------------------------------------
   hash_get(tbl, key)
   Retrieves a pointer to the element with the given key, or NULL if not found.
   The returned pointer is of type void*; cast it to the appropriate element type.
------------------------------------------------------------------ */
#define hash_get(tbl, key)                                                            \
    ({ __typeof__(tbl) _hg_tbl = (tbl);                                               \
       _hg_tbl ? _hash_get_impl(_hg_tbl, (key), sizeof(*(_hg_tbl))) : (void *)0; })

/* ------------------------------------------------------------------
   hash_set_min_capacity(tbl, min_cap)
   Ensures that the hash table has at least 'min_cap' buckets.
   - If (tbl) is NULL, allocates a new table with capacity = max(HASH_INIT_CAPACITY, min_cap).
   - If the table exists but its capacity is less than min_cap, it is resized.
   Example:
       hash_set_min_capacity(table, 64);
------------------------------------------------------------------ */
#define hash_set_min_capacity(tbl, min_cap)                                                             \
    do {                                                                                                \
        size_t _h_min_cap = (min_cap);                                                                  \
        __typeof__(tbl) _h_tbl = (tbl);                                                                 \
        if (!_h_tbl) {                                                                                  \
            size_t _h_cap = _h_min_cap > HASH_INIT_CAPACITY ? _h_min_cap : HASH_INIT_CAPACITY;          \
            size_t _h_elem_size = sizeof(*(_h_tbl));                                                    \
            hash_header *_h_hdr = (hash_header *)malloc(sizeof(hash_header) +                           \
                                        _h_cap * _h_elem_size);                                         \
            _h_hdr->capacity = _h_cap;                                                                  \
            _h_hdr->count = 0;                                                                          \
            _h_hdr->load_factor = HASH_LOAD_FACTOR;                                                     \
            _h_hdr->growth_factor = HASH_GROWTH_FACTOR_DEFAULT;                                         \
            _h_tbl = (void *)((char *)_h_hdr + sizeof(hash_header));                                    \
            memset(_h_tbl, 0, _h_cap * _h_elem_size);                                                   \
        } else {                                                                                        \
            hash_header *_h_hdr = HASH_HEADER(_h_tbl);                                                  \
            if (_h_hdr->capacity < _h_min_cap) {                                                        \
                HASH_RESIZE(_h_tbl, _h_min_cap);                                                        \
            }                                                                                           \
        }                                                                                               \
        (tbl) = _h_tbl;                                                                                 \
    } while (0)

/* ------------------------------------------------------------------
   hash_set_growth_factor(tbl, factor)
   Sets the growth factor for the hash table.
   The growth factor determines by what multiplier the table's capacity
   will be increased when resizing.
   The value is stored in the table's header and used for subsequent expansions.
   Example:
       hash_set_growth_factor(table, 3.0);
------------------------------------------------------------------ */
#define hash_set_growth_factor(tbl, factor)                                            \
    do {                                                                               \
        __typeof__(tbl) _hg_tbl = (tbl);                                               \
        if (_hg_tbl) {                                                                 \
            HASH_HEADER(_hg_tbl)->growth_factor = (factor);                            \
        }                                                                              \
    } while (0)

/* ------------------------------------------------------------------
   hash_set_load_factor(tbl, factor)
   Sets the load factor for the hash table.
   The load factor determines the threshold at which the table will be resized.
   The value is stored in the table's header and used for future resize decisions.
   Example:
       hash_set_load_factor(table, 0.8);
------------------------------------------------------------------ */
#define hash_set_load_factor(tbl, factor)                                              \
    do {                                                                               \
        __typeof__(tbl) _hl_tbl = (tbl);                                               \
        if (_hl_tbl) {                                                                 \
            HASH_HEADER(_hl_tbl)->load_factor = (factor);                              \
        }                                                                              \
    } while (0)

/* ------------------------------------------------------------------
   hash_delete_impl: Internal helper to delete an element by key.
   Removes the element with the given key from the table.
   It rehashes subsequent items in the cluster to maintain the integrity
   of the probing sequence.
------------------------------------------------------------------ */
static inline void hash_delete_impl(void *tbl_void, size_t elem_size, const char *key) {
    if (!tbl_void)
        return;
    char *tbl = (char *)tbl_void;
    hash_header *hdr = HASH_HEADER(tbl);
    size_t cap = hdr->capacity;
    size_t h = _hash_string(key) % cap;
    size_t start = h;
    while (1) {
        void *elem_ptr = tbl + h * elem_size;
        const char *elem_key = *((const char **)elem_ptr);
        if (!elem_key)
            break;
        if (strcmp(elem_key, key) == 0) {
            /* Found the element to delete. Clear this bucket. */
            *((const char **)elem_ptr) = NULL;
            hdr->count--;
            /* Rehash items in the cluster */
            size_t j = (h + 1) % cap;
            while (1) {
                void *item_ptr = tbl + j * elem_size;
                const char *ik = *((const char **)item_ptr);
                if (!ik)
                    break;
                /* Copy the item into a temporary buffer */
                char temp[elem_size];
                memcpy(temp, item_ptr, elem_size);
                /* Clear the bucket */
                *((const char **)item_ptr) = NULL;
                hdr->count--;
                /* Find new position for the item */
                size_t new_h = _hash_string(*((const char **)temp)) % cap;
                while (1) {
                    void *new_item_ptr = tbl + new_h * elem_size;
                    if (!*((const char **)new_item_ptr))
                        break;
                    new_h = (new_h + 1) % cap;
                }
                memcpy(tbl + new_h * elem_size, temp, elem_size);
                hdr->count++;
                j = (j + 1) % cap;
            }
            break;
        }
        h = (h + 1) % cap;
        if (h == start)
            break;
    }
}

/* ------------------------------------------------------------------
   hash_delete(tbl, key)
   Removes the element with the given key from the hash table.
   Example:
       hash_delete(table, "apple");
------------------------------------------------------------------ */
#define hash_delete(tbl, key)                                                          \
    do {                                                                               \
        __typeof__(tbl) _hd_tbl = (tbl);                                               \
        const char *_hd_key = (key);                                                   \
        if (_hd_tbl) {                                                                 \
            hash_delete_impl((void *)(_hd_tbl), sizeof(*(_hd_tbl)), _hd_key);          \
        }                                                                              \
        (tbl) = _hd_tbl;                                                               \
    } while (0)

/* ------------------------------------------------------------------
   hash_dup(tbl)
   Duplicates the entire hash table (including its hidden header) and
   returns a pointer to the new table.
   - The table is shallow-copied: pointer values (including keys) are duplicated.
   - Returns NULL if (tbl) is NULL or if memory allocation fails.
   Example:
       MyType *new_table = hash_dup(old_table);
------------------------------------------------------------------ */
#define hash_dup(tbl)                                                                      \
    ({ __typeof__(tbl) _orig = (tbl);                                                      \
       __typeof__(tbl) _dup = NULL;                                                        \
       if (_orig) {                                                                        \
           hash_header *_orig_hdr = HASH_HEADER(_orig);                                    \
           size_t _cap = _orig_hdr->capacity;                                              \
           size_t _elem_size = sizeof(*(_orig));                                           \
           hash_header *_new_hdr = malloc(sizeof(hash_header) + _cap * _elem_size);        \
           if (_new_hdr) {                                                                 \
               _new_hdr->count = _orig_hdr->count;                                         \
               _new_hdr->capacity = _cap;                                                  \
               _new_hdr->load_factor = _orig_hdr->load_factor;                             \
               _new_hdr->growth_factor = _orig_hdr->growth_factor;                         \
               void *_new_arr = (char *)_new_hdr + sizeof(hash_header);                    \
               memcpy(_new_arr, _orig, _cap * _elem_size);                                 \
               _dup = _new_arr;                                                            \
           }                                                                               \
       }                                                                                   \
       _dup; })

/* ------------------------------------------------------------------
   hash_free(tbl)
   Frees the entire hash table (including its hidden header) and sets tbl to NULL.
------------------------------------------------------------------ */
#define hash_free(tbl)                                                                 \
    do {                                                                               \
       if (tbl) { free((char *)(tbl) - sizeof(hash_header)); (tbl) = NULL; }           \
    } while (0)

#endif  /* SIMPLE_HASH_H */
