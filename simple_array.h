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

/* Compute the header size rounded up to the alignment of the element type */
#define ARRAY_HEADER_SIZE(arr) \
    (((sizeof(array_header) + __alignof__(*(arr)) - 1) / __alignof__(*(arr))) * __alignof__(*(arr)))

/* Given a pointer to the user array, get a pointer to the hidden header */
#define ARRAY_HEADER(arr) ((array_header *)((char *)(arr) - ARRAY_HEADER_SIZE(arr)))

/* Query macros */
#define array_count(arr)         ((arr) ? ARRAY_HEADER(arr)->count : 0)
#define array_capacity(arr)      ((arr) ? ARRAY_HEADER(arr)->capacity : 0)
#define array_growth_factor(arr) ((arr) ? ARRAY_HEADER(arr)->growth_factor : ARRAY_GROWTH_FACTOR_DEFAULT)

/* Internal macro to resize the array to a new capacity.
 * new_cap must be a size_t.
 */
#define ARRAY_RESIZE(arr, new_cap)                                                                               \
    do {                                                                                                         \
        _Static_assert(__builtin_types_compatible_p(__typeof__(new_cap), size_t), "new_cap must be size_t");     \
        __typeof__(arr) _a = (arr);                                                                              \
        size_t _elem_size = sizeof(*(_a));                                                                       \
        size_t _header_size = ARRAY_HEADER_SIZE(_a);                                                             \
        array_header *_old_hdr = _a ? ARRAY_HEADER(_a) : NULL;                                                   \
        size_t _old_count = _old_hdr ? _old_hdr->count : 0;                                                      \
        array_header *_new_hdr = (array_header *)malloc(_header_size + (new_cap) * _elem_size);                  \
        _new_hdr->capacity = (new_cap);                                                                          \
        _new_hdr->count = _old_count;                                                                            \
        _new_hdr->growth_factor = _old_hdr ? _old_hdr->growth_factor : ARRAY_GROWTH_FACTOR_DEFAULT;              \
        void *_new_arr = (char *)_new_hdr + _header_size;                                                        \
        if (_old_hdr) {                                                                                          \
            memcpy(_new_arr, _a, _old_count * _elem_size);                                                       \
            free((char *)_old_hdr);                                                                              \
        }                                                                                                        \
        (arr) = _new_arr;                                                                                        \
    } while (0)

/* Append an item to the end of the array.
 * The type of 'item' must match the element type (i.e. *arr).
 */
#define array_push(arr, item)                                                                                                          \
    do {                                                                                                                               \
        __typeof__(arr) _a = (arr);                                                                                                    \
        { /* Use dummy to deduce type */                                                                                               \
            __typeof__(*( (__typeof__(arr))0 )) _dummy;                                                                                \
            _Static_assert(__builtin_types_compatible_p(__typeof__(item), __typeof__(_dummy)),                                         \
                           "item must be of the same type as *arr");                                                                   \
        }                                                                                                                              \
        if (!_a) {                                                                                                                     \
            size_t _cap = ARRAY_INIT_CAPACITY;                                                                                         \
            size_t _elem_size = sizeof(*(_a));                                                                                         \
            size_t _header_size = (((sizeof(array_header) + __alignof__(*(_a)) - 1) / __alignof__(*(_a))) * __alignof__(*(_a)));       \
            array_header *_hdr = (array_header *)malloc(_header_size + _cap * _elem_size);                                             \
            _hdr->capacity = _cap;                                                                                                     \
            _hdr->count = 0;                                                                                                           \
            _hdr->growth_factor = ARRAY_GROWTH_FACTOR_DEFAULT;                                                                         \
            _a = (void *)((char *)_hdr + _header_size);                                                                                \
            memset(_a, 0, _cap * _elem_size);                                                                                          \
        }                                                                                                                              \
        array_header *_hdr = ARRAY_HEADER(_a);                                                                                         \
        if (_hdr->count >= _hdr->capacity) {                                                                                           \
            size_t _new_cap = (size_t)(_hdr->capacity * _hdr->growth_factor);                                                          \
            if (_new_cap <= _hdr->capacity) _new_cap = _hdr->capacity + 1;                                                             \
            ARRAY_RESIZE(_a, _new_cap);                                                                                                \
            _hdr = ARRAY_HEADER(_a);                                                                                                   \
        }                                                                                                                              \
        _a[_hdr->count++] = (item);                                                                                                    \
        (arr) = _a;                                                                                                                    \
    } while (0)

/* Remove the last item from the array and output it to 'out'. */
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

/* Delete the element at the specified index, shifting subsequent elements. */
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

/* Ensure the array has at least min_cap capacity.
 * min_cap must be a size_t.
 */
#define array_set_min_capacity(arr, min_cap)                                                                                            \
    do {                                                                                                                                \
        _Static_assert(__builtin_types_compatible_p(__typeof__(min_cap), size_t), "min_cap must be size_t");                            \
        size_t _m_min_cap = (min_cap);                                                                                                  \
        __typeof__(arr) _a = (arr);                                                                                                     \
        if (!_a) {                                                                                                                      \
            size_t _elem_size = sizeof(*(_a));                                                                                          \
            size_t _header_size = (((sizeof(array_header) + __alignof__(*(_a)) - 1) / __alignof__(*(_a))) * __alignof__(*(_a)));        \
            array_header *_hdr = (array_header *)malloc(_header_size + _m_min_cap * _elem_size);                                        \
            _hdr->capacity = _m_min_cap;                                                                                                \
            _hdr->count = 0;                                                                                                            \
            _hdr->growth_factor = ARRAY_GROWTH_FACTOR_DEFAULT;                                                                          \
            _a = (void *)((char *)_hdr + _header_size);                                                                                 \
            memset(_a, 0, _m_min_cap * _elem_size);                                                                                     \
        } else {                                                                                                                        \
            array_header *_hdr = ARRAY_HEADER(_a);                                                                                      \
            if (_hdr->capacity < _m_min_cap) {                                                                                          \
                ARRAY_RESIZE(_a, _m_min_cap);                                                                                           \
            }                                                                                                                           \
        }                                                                                                                               \
        (arr) = _a;                                                                                                                     \
    } while (0)

/* Set the growth factor for the array.
 * factor must be a double.
 */
#define array_set_growth_factor(arr, factor)                                                                     \
    do {                                                                                                         \
        _Static_assert(__builtin_types_compatible_p(__typeof__(factor), double), "factor must be double");       \
        __typeof__(arr) _a = (arr);                                                                              \
        if (_a) {                                                                                                \
            ARRAY_HEADER(_a)->growth_factor = (factor);                                                          \
        }                                                                                                        \
    } while (0)

/* Duplicate the array (shallow copy). */
#define array_dup(arr)                                                                        \
    ({                                                                                        \
        __typeof__(arr) _orig = (arr);                                                        \
        __typeof__(arr) _dup = NULL;                                                          \
        if (_orig) {                                                                          \
            array_header *_orig_hdr = ARRAY_HEADER(_orig);                                    \
            size_t _cap = _orig_hdr->capacity;                                                \
            size_t _elem_size = sizeof(*(_orig));                                             \
            size_t _header_size = ARRAY_HEADER_SIZE(_orig);                                   \
            array_header *_new_hdr = malloc(_header_size + _cap * _elem_size);                \
            if (_new_hdr) {                                                                   \
                _new_hdr->count = _orig_hdr->count;                                           \
                _new_hdr->capacity = _cap;                                                    \
                _new_hdr->growth_factor = _orig_hdr->growth_factor;                           \
                void *_new_arr = (char *)_new_hdr + _header_size;                             \
                memcpy(_new_arr, _orig, _orig_hdr->count * _elem_size);                       \
                _dup = _new_arr;                                                              \
            }                                                                                 \
        }                                                                                     \
        _dup;                                                                                 \
    })

/* Free the array and its hidden header. */
#define array_free(arr)                                                                 \
    do {                                                                                \
        if (arr) {                                                                      \
            free((char *)(arr) - ARRAY_HEADER_SIZE(arr));                               \
            (arr) = NULL;                                                               \
        }                                                                               \
    } while (0)

/* Clear the array (set count to zero). */
#define array_clear(arr)                                                                \
    do {                                                                                \
        if (arr) {                                                                      \
            ARRAY_HEADER(arr)->count = 0;                                               \
        }                                                                               \
    } while (0)

#endif  /* SIMPLE_ARRAY_H */
