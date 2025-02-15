/*
 * A simple open-addressing hash map implementation using linear probing.
 *
 * The hidden map header is stored immediately before the user array and includes:
 *   - count:          Number of elements stored in the map.
 *   - capacity:       Total number of buckets.
 *   - load_factor:    Maximum ratio of filled buckets to capacity before resizing.
 *   - growth_factor:  Multiplier used to increase capacity when resizing.
 *
 * Default configuration:
 *   - MAP_INIT_CAPACITY:          Initial number of buckets.
 *   - MAP_LOAD_FACTOR:            Default load factor threshold.
 *   - MAP_GROWTH_FACTOR_DEFAULT:  Default multiplier for map expansion.
 *
 * Usage notes:
 *   - The map’s element type must have a field named `key` (of type const char*)
 *     as its first member.
 *
 * Public API macros:
 *   - map_count(tbl):                      Gets the number of elements in the map.
 *   - map_capacity(tbl):                   Gets the capacity of the map.
 *   - map_load_factor(tbl):                Gets the load factor of the map.
 *   - map_growth_factor(tbl):              Gets the growth factor of the map.
 *   - map_put(tbl, item):                  Inserts or updates an element.
 *   - map_get(tbl, key):                   Retrieves a pointer to an element with the given key.
 *   - map_delete(tbl, key):                Removes the element with the given key.
 *   - map_set_min_capacity(tbl, min_cap):  Ensures a minimum map capacity.
 *   - map_set_growth_factor(tbl, factor):  Sets the map's growth factor.
 *   - map_set_load_factor(tbl, factor):    Sets the map's load factor.
 *   - map_dup(tbl):                        Duplicates the map.
 *   - map_free(tbl):                       Frees the map.
 */

#ifndef SIMPLE_MAP_H
#define SIMPLE_MAP_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Configuration: initial capacity, load factor, and growth factor */
#ifndef MAP_INIT_CAPACITY
#define MAP_INIT_CAPACITY 16
#endif

#ifndef MAP_LOAD_FACTOR
#define MAP_LOAD_FACTOR 0.75
#endif

#ifndef MAP_GROWTH_FACTOR_DEFAULT
#define MAP_GROWTH_FACTOR_DEFAULT 2.0
#endif

/* Hidden map header stored immediately before the user array.
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
} map_header;

/* Compute the aligned header size for a map, based on the alignment of the element type */
#define MAP_HEADER_SIZE(tbl) \
    (((sizeof(map_header) + __alignof__(*(tbl)) - 1) / __alignof__(*(tbl))) * __alignof__(*(tbl)))

/* Given a map pointer, MAP_HEADER returns a pointer to its hidden map header.
 * The header is stored immediately before the array of elements.
 */
#define MAP_HEADER(tbl) ((map_header *)((char *)(tbl) - MAP_HEADER_SIZE(tbl)))

/* Public macros to query map properties */
#define map_count(tbl)         ((tbl) ? MAP_HEADER(tbl)->count : 0)
#define map_capacity(tbl)      ((tbl) ? MAP_HEADER(tbl)->capacity : 0)
#define map_load_factor(tbl)   ((tbl) ? MAP_HEADER(tbl)->load_factor : MAP_LOAD_FACTOR)
#define map_growth_factor(tbl) ((tbl) ? MAP_HEADER(tbl)->growth_factor : MAP_GROWTH_FACTOR_DEFAULT)

/* djb2 hash function for string keys (remains unchanged) */
static inline size_t _hash_string(const char *str) {
    size_t hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
         hash = ((hash << 5) + hash) + c;
    return hash;
}

/* ------------------------------------------------------------------
   Internal function: _map_get_impl
   Looks up an element in the hash map by key.
   Assumes that the element's first field is a pointer (const char *) holding the key.
   Returns a pointer to the element, or NULL if not found.
------------------------------------------------------------------ */
static inline void *_map_get_impl(void *tbl_void, const char *key, size_t elem_size) {
    if (!tbl_void)
        return NULL;
    char *tbl = (char *)tbl_void;
    map_header *hdr = MAP_HEADER(tbl);
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
   Internal macro: MAP_RESIZE
   Resizes the hash map to a new capacity.
   Allocates a new block (with the updated capacity), re-inserts all existing
   items using linear probing, copies over the load_factor and growth_factor,
   frees the old block, and updates the map pointer.
------------------------------------------------------------------ */
#define MAP_RESIZE(tbl, new_cap)                                                                                                                         \
    do {                                                                                                                                                 \
        _Static_assert(__builtin_types_compatible_p(__typeof__(new_cap), size_t), "new_cap must be size_t");                                             \
        __typeof__(tbl) _ms_tbl = (tbl);                                                                                                                 \
        size_t _ms_new_cap = (new_cap);                                                                                                                  \
        size_t _ms_elem_size = sizeof(*(_ms_tbl));                                                                                                       \
        size_t _ms_header_size = MAP_HEADER_SIZE(_ms_tbl ? _ms_tbl : ((_ms_tbl = 0), (_ms_tbl)));                                                        \
        map_header *_ms_old_hdr = _ms_tbl ? MAP_HEADER(_ms_tbl) : NULL;                                                                                  \
        size_t _ms_old_cap = _ms_tbl ? _ms_old_hdr->capacity : 0;                                                                                        \
        void *_ms_old_ptr = _ms_tbl ? ((char *)(_ms_tbl) - MAP_HEADER_SIZE(_ms_tbl)) : NULL;                                                             \
        map_header *_ms_new_hdr = (map_header *)malloc(MAP_HEADER_SIZE(_ms_tbl ? _ms_tbl : ((_ms_tbl = 0), _ms_tbl)) + _ms_new_cap * _ms_elem_size);     \
        _ms_new_hdr->capacity = _ms_new_cap;                                                                                                             \
        _ms_new_hdr->count = 0;                                                                                                                          \
        _ms_new_hdr->load_factor = _ms_old_hdr ? _ms_old_hdr->load_factor : MAP_LOAD_FACTOR;                                                             \
        _ms_new_hdr->growth_factor = _ms_old_hdr ? _ms_old_hdr->growth_factor : MAP_GROWTH_FACTOR_DEFAULT;                                               \
        char *_ms_new_arr = (char *)_ms_new_hdr + MAP_HEADER_SIZE(_ms_tbl ? _ms_tbl : ((_ms_tbl = 0), _ms_tbl));                                         \
        memset(_ms_new_arr, 0, _ms_new_cap * _ms_elem_size);                                                                                             \
        if (_ms_old_ptr) {                                                                                                                               \
            char *old_items = (char *)_ms_old_ptr + MAP_HEADER_SIZE(_ms_tbl);                                                                            \
            for (size_t _ms_i = 0; _ms_i < _ms_old_cap; _ms_i++) {                                                                                       \
                char *cur = old_items + _ms_i * _ms_elem_size;                                                                                           \
                if (*((const char **)cur) != NULL) {                                                                                                     \
                    size_t _ms = _hash_string(*((const char **)cur)) % _ms_new_cap;                                                                      \
                    while (*((const char **)(_ms_new_arr + _ms * _ms_elem_size)) != NULL) {                                                              \
                        _ms = (_ms + 1) % _ms_new_cap;                                                                                                   \
                    }                                                                                                                                    \
                    memcpy(_ms_new_arr + _ms * _ms_elem_size, cur, _ms_elem_size);                                                                       \
                    _ms_new_hdr->count++;                                                                                                                \
                }                                                                                                                                        \
            }                                                                                                                                            \
            free(_ms_old_ptr);                                                                                                                           \
        }                                                                                                                                                \
        (tbl) = (void *)_ms_new_arr;                                                                                                                     \
    } while (0)

/* ------------------------------------------------------------------
   map_put(tbl, item)
   Inserts a new element (or updates an existing one) into the hash map.
   - If (tbl) is NULL, a new map is allocated with MAP_INIT_CAPACITY.
   - Expects that the element type's first field is a pointer (const char *)
     holding the key.
   - Also checks that the type of item matches the element type (i.e. *tbl).
   Example:
       Foo item = { "apple", 10, 3.14, 'A' };
       map_put(table, item);
------------------------------------------------------------------ */
#define map_put(tbl, item)                                                                                          \
    do {                                                                                                            \
        /* Use a dummy 0 pointer cast to the type of tbl to get the element type even if tbl is NULL */             \
        __typeof__(*( (__typeof__(tbl))0 )) _dummy;                                                                 \
        __typeof__(tbl) _mp_tbl = (tbl);                                                                            \
        __typeof__(item) _mp_item = (item);                                                                         \
        _Static_assert(__builtin_types_compatible_p(__typeof__(_mp_item), __typeof__(*((__typeof__(tbl))0))),       \
                       "item must be of the same type as *tbl");                                                    \
        _Static_assert(                                                                                             \
            __builtin_types_compatible_p(__typeof__((_mp_item).key), char *) ||                                     \
            __builtin_types_compatible_p(__typeof__((_mp_item).key), const char *),                                 \
            "item.key must be a char* or const char*"                                                               \
        );                                                                                                          \
        if (!_mp_tbl) {                                                                                             \
            size_t _mp_cap = MAP_INIT_CAPACITY;                                                                     \
            size_t _mp_elem_size = sizeof(*((__typeof__(tbl))0));                                                   \
            size_t _mp_header_size = (((sizeof(map_header) + __alignof__(*((__typeof__(tbl))0)) - 1)                \
                / __alignof__(*((__typeof__(tbl))0))) * __alignof__(*((__typeof__(tbl))0)));                        \
            map_header *_mp_hdr = (map_header *)malloc(_mp_header_size + _mp_cap * _mp_elem_size);                  \
            _mp_hdr->capacity = _mp_cap;                                                                            \
            _mp_hdr->count = 0;                                                                                     \
            _mp_hdr->load_factor = MAP_LOAD_FACTOR;                                                                 \
            _mp_hdr->growth_factor = MAP_GROWTH_FACTOR_DEFAULT;                                                     \
            _mp_tbl = (void *)((char *)_mp_hdr + _mp_header_size);                                                  \
            memset(_mp_tbl, 0, _mp_cap * _mp_elem_size);                                                            \
        }                                                                                                           \
        map_header *_mp_hdr = MAP_HEADER(_mp_tbl);                                                                  \
        if ((_mp_hdr->count + 1) >= (size_t)(_mp_hdr->capacity * _mp_hdr->load_factor)) {                           \
            size_t _mp_new_cap = (size_t)(_mp_hdr->capacity * _mp_hdr->growth_factor);                              \
            if (_mp_new_cap <= _mp_hdr->capacity) _mp_new_cap = _mp_hdr->capacity + 1;                              \
            MAP_RESIZE(_mp_tbl, _mp_new_cap);                                                                       \
            _mp_hdr = MAP_HEADER(_mp_tbl);                                                                          \
        }                                                                                                           \
        size_t _mp_cap = _mp_hdr->capacity;                                                                         \
        size_t _mp_h = _hash_string(_mp_item.key) % _mp_cap;                                                        \
        while (_mp_tbl[_mp_h].key) {                                                                                \
            if (strcmp(_mp_tbl[_mp_h].key, _mp_item.key) == 0) {                                                    \
                _mp_tbl[_mp_h] = _mp_item;                                                                          \
                break;                                                                                              \
            }                                                                                                       \
            _mp_h = (_mp_h + 1) % _mp_cap;                                                                          \
        }                                                                                                           \
        if (!_mp_tbl[_mp_h].key) {                                                                                  \
            _mp_tbl[_mp_h] = _mp_item;                                                                              \
            _mp_hdr->count++;                                                                                       \
        }                                                                                                           \
        (tbl) = _mp_tbl;                                                                                            \
    } while (0)

/* ------------------------------------------------------------------
   map_get(tbl, key)
   Retrieves a pointer to the element with the given key, or NULL if not found.
   The returned pointer is cast to the same type as (tbl).
------------------------------------------------------------------ */
#define map_get(tbl, key)                                                                \
    ({                                                                                   \
        _Static_assert(                                                                  \
            __builtin_types_compatible_p(__typeof__(key), char *) ||                     \
            __builtin_types_compatible_p(__typeof__(key), const char *) ||               \
            (__builtin_constant_p(key) && ((key) == 0)),                                 \
            "key must be a char* or const char* (or NULL)"                               \
        );                                                                               \
        __typeof__(tbl) _mg_tbl = (tbl);                                                 \
        const char *_mg_key = (key);                                                     \
        _mg_tbl && _mg_key                                                               \
            ? (__typeof__(tbl))_map_get_impl(_mg_tbl, _mg_key, sizeof(*(_mg_tbl)))       \
            : NULL;                                                                      \
    })

/* ------------------------------------------------------------------
   map_set_min_capacity(tbl, min_cap)
   Ensures that the hash map has at least 'min_cap' buckets.
   - If (tbl) is NULL, allocates a new map with capacity = max(MAP_INIT_CAPACITY, min_cap).
   - If the map exists but its capacity is less than min_cap, it is resized.
   Example:
       map_set_min_capacity(table, 64);
------------------------------------------------------------------ */
#define map_set_min_capacity(tbl, min_cap)                                                                        \
    do {                                                                                                          \
        _Static_assert(__builtin_types_compatible_p(__typeof__(min_cap), size_t), "min_cap must be size_t");      \
        size_t _m_min_cap = (min_cap);                                                                            \
        __typeof__(tbl) _m_tbl = (tbl);                                                                           \
        if (!_m_tbl) {                                                                                            \
            size_t _m_cap = _m_min_cap > MAP_INIT_CAPACITY ? _m_min_cap : MAP_INIT_CAPACITY;                      \
            size_t _m_elem_size = sizeof(*((__typeof__(tbl))0));                                                  \
            size_t _m_header_size = (((sizeof(map_header) + __alignof__(*((__typeof__(tbl))0)) - 1)               \
                / __alignof__(*((__typeof__(tbl))0))) * __alignof__(*((__typeof__(tbl))0)));                      \
            map_header *_m_hdr = (map_header *)malloc(_m_header_size + _m_cap * _m_elem_size);                    \
            _m_hdr->capacity = _m_cap;                                                                            \
            _m_hdr->count = 0;                                                                                    \
            _m_hdr->load_factor = MAP_LOAD_FACTOR;                                                                \
            _m_hdr->growth_factor = MAP_GROWTH_FACTOR_DEFAULT;                                                    \
            _m_tbl = (void *)((char *)_m_hdr + _m_header_size);                                                   \
            memset(_m_tbl, 0, _m_cap * _m_elem_size);                                                             \
        } else {                                                                                                  \
            map_header *_m_hdr = MAP_HEADER(_m_tbl);                                                              \
            if (_m_hdr->capacity < _m_min_cap) {                                                                  \
                MAP_RESIZE(_m_tbl, _m_min_cap);                                                                   \
            }                                                                                                     \
        }                                                                                                         \
        (tbl) = _m_tbl;                                                                                           \
    } while (0)

/* ------------------------------------------------------------------
   map_set_growth_factor(tbl, factor)
   Sets the growth factor for the hash map.
   The growth factor determines by what multiplier the map's capacity
   will be increased when resizing.
   Example:
       map_set_growth_factor(table, 3.0);
------------------------------------------------------------------ */
#define map_set_growth_factor(tbl, factor)                                                                     \
    do {                                                                                                       \
        _Static_assert(__builtin_types_compatible_p(__typeof__(factor), double), "factor must be double");     \
        __typeof__(tbl) _mg_tbl = (tbl);                                                                       \
        if (_mg_tbl) {                                                                                         \
            MAP_HEADER(_mg_tbl)->growth_factor = (factor);                                                     \
        }                                                                                                      \
    } while (0)

/* ------------------------------------------------------------------
   map_set_load_factor(tbl, factor)
   Sets the load factor for the hash map.
   The load factor determines the threshold at which the map will be resized.
   The value is stored in the map's header and used for future resize decisions.
   Example:
       map_set_load_factor(table, 0.8);
------------------------------------------------------------------ */
#define map_set_load_factor(tbl, factor)                                                                       \
    do {                                                                                                       \
        _Static_assert(__builtin_types_compatible_p(__typeof__(factor), double), "factor must be double");     \
        __typeof__(tbl) _ml_tbl = (tbl);                                                                       \
        if (_ml_tbl) {                                                                                         \
            MAP_HEADER(_ml_tbl)->load_factor = (factor);                                                       \
        }                                                                                                      \
    } while (0)

/* ------------------------------------------------------------------
   Internal function: map_delete_impl
   Removes the element with the given key from the hash map.
   It rehashes subsequent items in the cluster to maintain the integrity
   of the probing sequence.
------------------------------------------------------------------ */
static inline void map_delete_impl(void *tbl_void, size_t elem_size, const char *key) {
    if (!tbl_void)
        return;
    char *tbl = (char *)tbl_void;
    map_header *hdr = MAP_HEADER(tbl);
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
   map_delete(tbl, key)
   Removes the element with the given key from the hash map.
   Example:
       map_delete(table, "apple");
------------------------------------------------------------------ */
#define map_delete(tbl, key)                                                          \
    do {                                                                              \
        _Static_assert(                                                               \
            __builtin_types_compatible_p(__typeof__(key), char *) ||                  \
            __builtin_types_compatible_p(__typeof__(key), const char *) ||            \
            (__builtin_constant_p(key) && ((key) == 0)),                              \
            "key must be a char* or const char* (or NULL)"                            \
        );                                                                            \
        __typeof__(tbl) _md_tbl = (tbl);                                              \
        const char *_md_key = (key);                                                  \
        if (_md_tbl) {                                                                \
            map_delete_impl((void *)(_md_tbl), sizeof(*(_md_tbl)), _md_key);          \
        }                                                                             \
        (tbl) = _md_tbl;                                                              \
    } while (0)

/* ------------------------------------------------------------------
   map_dup(tbl)
   Duplicates the entire hash map (including its hidden map header) and
   returns a pointer to the new map.
   - The map is shallow-copied: pointer values (including keys) are duplicated.
   - Returns NULL if (tbl) is NULL or if memory allocation fails.
   Example:
       MyType *new_map = map_dup(old_map);
------------------------------------------------------------------ */
#define map_dup(tbl)                                                                        \
    ({ __typeof__(tbl) _orig = (tbl);                                                       \
       __typeof__(tbl) _dup = NULL;                                                         \
       if (_orig) {                                                                         \
           map_header *_orig_hdr = MAP_HEADER(_orig);                                       \
           size_t _cap = _orig_hdr->capacity;                                               \
           size_t _elem_size = sizeof(*(_orig));                                            \
           size_t _header_size = MAP_HEADER_SIZE(_orig);                                    \
           map_header *_new_hdr = malloc(_header_size + _cap * _elem_size);                 \
           if (_new_hdr) {                                                                  \
               _new_hdr->count = _orig_hdr->count;                                          \
               _new_hdr->capacity = _cap;                                                   \
               _new_hdr->load_factor = _orig_hdr->load_factor;                              \
               _new_hdr->growth_factor = _orig_hdr->growth_factor;                          \
               void *_new_arr = (char *)_new_hdr + _header_size;                            \
               memcpy(_new_arr, _orig, _cap * _elem_size);                                  \
               _dup = _new_arr;                                                             \
           }                                                                                \
       }                                                                                    \
       _dup; })

/* ------------------------------------------------------------------
   map_free(tbl)
   Frees the entire hash map (including its hidden map header) and sets tbl to NULL.
------------------------------------------------------------------ */
#define map_free(tbl)                                                                 \
    do {                                                                              \
       if (tbl) { free((char *)(tbl) - MAP_HEADER_SIZE(tbl)); (tbl) = NULL; }         \
    } while (0)

#endif  /* SIMPLE_MAP_H */
