/*
 * A simple dynamic array library with a hidden header.
 *
 * The hidden array header is stored immediately before the user array and includes:
 *   - count:          Number of elements in the array.
 *   - capacity:       Total number of elements allocated.
 *   - growth_factor:  Multiplier used to increase capacity when resizing.
 *
 * Default configuration:
 *   - ARRAY_INIT_CAPACITY:          Initial number of elements.
 *   - ARRAY_GROWTH_FACTOR_DEFAULT:  Default multiplier for array expansion.
 *
 * Usage notes:
 *   - The array’s element type can be any type.
 *   - The array pointer returned points to the first element and can be indexed directly (e.g., array[i]).
 *
 * Public API macros:
 *   - array_count(arr):                      Returns the number of elements in the array.
 *   - array_capacity(arr):                   Returns the total capacity of the array.
 *   - array_growth_factor(arr):              Returns the current growth factor.
 *   - array_push(arr, item):                 Appends an item to the array.
 *   - array_pop(arr, out):                   Removes the last item from the array and outputs it.
 *   - array_delete(arr, index):              Deletes the element at the specified index.
 *   - array_set_min_capacity(arr, min_cap):  Ensures the array has at least min_cap capacity.
 *   - array_set_growth_factor(arr, factor):  Sets the array's growth factor.
 *   - array_dup(arr):                        Duplicates the array (shallow copy).
 *   - array_free(arr):                       Frees the array.
 *   - array_clear(arr):                      Resets the array's count to zero.
 */

#ifndef SIMPLE_ARRAY_H
#define SIMPLE_ARRAY_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef ARRAY_INIT_CAPACITY
#define ARRAY_INIT_CAPACITY 16
#endif

#ifndef ARRAY_GROWTH_FACTOR_DEFAULT
#define ARRAY_GROWTH_FACTOR_DEFAULT 2.0
#endif

typedef struct {
    size_t count;
    size_t capacity;
    double growth_factor;
} array_header;

#define ARRAY_HEADER(arr) ((array_header *)((char *)(arr) - sizeof(array_header)))

#define array_count(arr)         ((arr) ? ARRAY_HEADER(arr)->count : 0)
#define array_capacity(arr)      ((arr) ? ARRAY_HEADER(arr)->capacity : 0)
#define array_growth_factor(arr) ((arr) ? ARRAY_HEADER(arr)->growth_factor : ARRAY_GROWTH_FACTOR_DEFAULT)

/* Internal macro to resize the array to a new capacity */
#define ARRAY_RESIZE(arr, new_cap)                                                                           \
    do {                                                                                                     \
        __typeof__(arr) _a = (arr);                                                                          \
        size_t _elem_size = sizeof(*(_a));                                                                   \
        array_header *_old_hdr = _a ? ARRAY_HEADER(_a) : NULL;                                               \
        size_t _old_count = _old_hdr ? _old_hdr->count : 0;                                                  \
        array_header *_new_hdr = (array_header *)malloc(sizeof(array_header) +                               \
                                            (new_cap) * _elem_size);                                         \
        _new_hdr->capacity = (new_cap);                                                                      \
        _new_hdr->count = _old_count;                                                                        \
        _new_hdr->growth_factor = _old_hdr ? _old_hdr->growth_factor : ARRAY_GROWTH_FACTOR_DEFAULT;          \
        void *_new_arr = (char *)_new_hdr + sizeof(array_header);                                            \
        if (_old_hdr) {                                                                                      \
            memcpy(_new_arr, _a, _old_count * _elem_size);                                                   \
            free((char *)_old_hdr);                                                                          \
        }                                                                                                    \
        (arr) = _new_arr;                                                                                    \
    } while (0)

/* Append an item to the end of the array */
#define array_push(arr, item)                                                                                \
    do {                                                                                                     \
        __typeof__(arr) _a = (arr);                                                                          \
        if (!_a) {                                                                                           \
            size_t _cap = ARRAY_INIT_CAPACITY;                                                               \
            size_t _elem_size = sizeof(*(_a));                                                               \
            array_header *_hdr = (array_header *)malloc(sizeof(array_header) + _cap * _elem_size);           \
            _hdr->capacity = _cap;                                                                           \
            _hdr->count = 0;                                                                                 \
            _hdr->growth_factor = ARRAY_GROWTH_FACTOR_DEFAULT;                                               \
            _a = (void *)((char *)_hdr + sizeof(array_header));                                              \
            memset(_a, 0, _cap * _elem_size);                                                                \
        }                                                                                                    \
        array_header *_hdr = ARRAY_HEADER(_a);                                                               \
        if (_hdr->count >= _hdr->capacity) {                                                                 \
            size_t _new_cap = (size_t)(_hdr->capacity * _hdr->growth_factor);                                \
            if (_new_cap <= _hdr->capacity) _new_cap = _hdr->capacity + 1;                                   \
            ARRAY_RESIZE(_a, _new_cap);                                                                      \
            _hdr = ARRAY_HEADER(_a);                                                                         \
        }                                                                                                    \
        _a[_hdr->count++] = (item);                                                                          \
        (arr) = _a;                                                                                          \
    } while (0)

/* Remove the last item from the array and output it to 'out' */
#define array_pop(arr, out)                                                            \
    do {                                                                               \
        __typeof__(arr) _a = (arr);                                                    \
        if (_a) {                                                                      \
            array_header *_hdr = ARRAY_HEADER(_a);                                     \
            if (_hdr->count > 0) {                                                     \
                (out) = _a[_hdr->count - 1];                                           \
                _hdr->count--;                                                         \
            } else {                                                                   \
                (out) = 0;                                                             \
            }                                                                          \
        } else {                                                                       \
            (out) = 0;                                                                 \
        }                                                                              \
    } while (0)

/* Delete the element at the specified index, shifting subsequent elements */
#define array_delete(arr, index)                                                                    \
    do {                                                                                            \
        __typeof__(arr) _a = (arr);                                                                 \
        if (_a) {                                                                                   \
            array_header *_hdr = ARRAY_HEADER(_a);                                                  \
            size_t _idx = (index);                                                                  \
            if (_idx < _hdr->count) {                                                               \
                memmove(&_a[_idx], &_a[_idx + 1], (_hdr->count - _idx - 1) * sizeof(*(_a)));        \
                _hdr->count--;                                                                      \
            }                                                                                       \
        }                                                                                           \
    } while (0)

/* Ensure the array has at least min_cap capacity */
#define array_set_min_capacity(arr, min_cap)                                                                  \
    do {                                                                                                      \
        size_t _min_cap = (min_cap);                                                                          \
        __typeof__(arr) _a = (arr);                                                                           \
        if (!_a) {                                                                                            \
            size_t _elem_size = sizeof(*(_a));                                                                \
            array_header *_hdr = (array_header *)malloc(sizeof(array_header) + _min_cap * _elem_size);        \
            _hdr->capacity = _min_cap;                                                                        \
            _hdr->count = 0;                                                                                  \
            _hdr->growth_factor = ARRAY_GROWTH_FACTOR_DEFAULT;                                                \
            _a = (void *)((char *)_hdr + sizeof(array_header));                                               \
            memset(_a, 0, _min_cap * _elem_size);                                                             \
        } else {                                                                                              \
            array_header *_hdr = ARRAY_HEADER(_a);                                                            \
            if (_hdr->capacity < _min_cap) {                                                                  \
                ARRAY_RESIZE(_a, _min_cap);                                                                   \
            }                                                                                                 \
        }                                                                                                     \
        (arr) = _a;                                                                                           \
    } while (0)

/* Set the growth factor for the array */
#define array_set_growth_factor(arr, factor)                                          \
    do {                                                                              \
        __typeof__(arr) _a = (arr);                                                   \
        if (_a) {                                                                     \
            ARRAY_HEADER(_a)->growth_factor = (factor);                               \
        }                                                                             \
    } while (0)

/* Duplicate the array (shallow copy) */
#define array_dup(arr)                                                                        \
    ({                                                                                        \
        __typeof__(arr) _orig = (arr);                                                        \
        __typeof__(arr) _dup = NULL;                                                          \
        if (_orig) {                                                                          \
            array_header *_orig_hdr = ARRAY_HEADER(_orig);                                    \
            size_t _cap = _orig_hdr->capacity;                                                \
            size_t _elem_size = sizeof(*(_orig));                                             \
            array_header *_new_hdr = malloc(sizeof(array_header) + _cap * _elem_size);        \
            if (_new_hdr) {                                                                   \
                _new_hdr->count = _orig_hdr->count;                                           \
                _new_hdr->capacity = _cap;                                                    \
                _new_hdr->growth_factor = _orig_hdr->growth_factor;                           \
                void *_new_arr = (char *)_new_hdr + sizeof(array_header);                     \
                memcpy(_new_arr, _orig, _orig_hdr->count * _elem_size);                       \
                _dup = _new_arr;                                                              \
            }                                                                                 \
        }                                                                                     \
        _dup;                                                                                 \
    })

/* Free the array and its hidden header */
#define array_free(arr)                                                                 \
    do {                                                                                \
        if (arr) {                                                                      \
            free((char *)(arr) - sizeof(array_header));                                 \
            (arr) = NULL;                                                               \
        }                                                                               \
    } while (0)

/* Clear the array (set count to zero) */
#define array_clear(arr)                                                                \
    do {                                                                                \
        if (arr) {                                                                      \
            ARRAY_HEADER(arr)->count = 0;                                               \
        }                                                                               \
    } while (0)

#endif  /* SIMPLE_ARRAY_H */