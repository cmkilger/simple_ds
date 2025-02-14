# Simple Data Structures Library

A header-only C library that provides two lightweight data structures:
1. A **hash map** (via `simple_map.h`)
2. A **dynamic array** (via `simple_array.h`)

These data structures offer flexible, dynamic behavior, automatically resizing as needed. They are designed to be simple, fast, and easy to integrate into your project.

Inspired by [stb_ds](https://github.com/nothings/stb/blob/master/stb_ds.h) and [uthash](https://github.com/troydhanson/uthash), this library enforces its own usage requirements and was primarily written by ChatGPT (including this file).

---

## Hash Map (`simple_map.h`)

The hash map uses open addressing with linear probing for collision resolution. It automatically resizes based on configurable load factors and growth factors. **Usage Note:** Each element stored in the map **must** have a field named `key` (of type `const char *`) as its **first member**.

### Features

- **Header-only Implementation:** Simply include the header.
- **Dynamic Resizing:** Automatically expands when the load factor threshold is exceeded.
- **Open Addressing with Linear Probing:** Efficient collision resolution.
- **Flexible API Macros:** Macros for insertion, retrieval, deletion, duplication, and configuration.
- **Configurable Parameters:** Override initial capacity, load factor, and growth factor.

### Installation

```c
#include "simple_map.h"
```

### API Overview

- `map_count(tbl)`: Returns the number of elements in the map.  
- `map_capacity(tbl)`: Returns the total number of buckets.  
- `map_load_factor(tbl)`: Returns the current load factor.  
- `map_growth_factor(tbl)`: Returns the current growth factor.  
- `map_put(tbl, item)`: Inserts a new element or updates an existing one.  
- `map_get(tbl, key)`: Retrieves a pointer to the element with the given key.  
- `map_delete(tbl, key)`: Removes the element with the given key.  
- `map_set_min_capacity(tbl, min_cap)`: Ensures that the map has at least `min_cap` buckets.  
- `map_set_growth_factor(tbl, factor)`: Sets the map's growth factor.  
- `map_set_load_factor(tbl, factor)`: Sets the map's load factor threshold.  
- `map_dup(tbl)`: Duplicates the map (shallow copy).  
- `map_free(tbl)`: Frees the map and resets the pointer to `NULL`.

### Example Usage

```c
#include <stdio.h>
#include "simple_map.h"

typedef struct {
    const char* key;  // Must be the first member
    int value;
} MyItem;

// Function that accepts a pointer to the map pointer
void insert_item(MyItem **map_ptr, MyItem item) {
    map_put(*map_ptr, item);
}

int main(void) {
    MyItem *map = NULL;

    MyItem apple = { "apple", 10 };
    MyItem banana = { "banana", 20 };
    map_put(map, apple);
    map_put(map, banana);

    MyItem cherry = { "cherry", 30 };
    insert_item(&map, cherry);

    MyItem *item = map_get(map, "apple");
    if (item) {
        printf("Found %s: %d\n", item->key, item->value);
    } else {
        printf("Item not found\n");
    }

    map_delete(map, "banana");
    MyItem *dup_map = map_dup(map);

    printf("Original map count: %zu, capacity: %zu\n", map_count(map), map_capacity(map));
    printf("Duplicate map count: %zu, capacity: %zu\n", map_count(dup_map), map_capacity(dup_map));

    map_free(map);
    map_free(dup_map);
    return 0;
}
```

### Configuration

You can override the default settings **before** including the header:

```c
#define MAP_INIT_CAPACITY 32
#define MAP_LOAD_FACTOR 0.8
#define MAP_GROWTH_FACTOR_DEFAULT 3.0
#include "simple_map.h"
```

---

## Dynamic Array (`simple_array.h`)

The dynamic array is a flexible vector implementation that automatically expands as needed. Elements are stored contiguously, allowing you to use standard indexing (e.g., `array[i]`).

### Features

- **Header-only Implementation:** Simply include the header.
- **Dynamic Resizing:** Automatically expands when needed.
- **Direct Indexing:** Access elements with `array[i]`.
- **Flexible API Macros:** Macros for appending, popping, deleting, duplicating, and more.
- **Configurable Parameters:** Override initial capacity and growth factor.

### Installation

```c
#include "simple_array.h"
```

### API Overview

- `array_count(arr)`: Returns the number of elements in the array.  
- `array_capacity(arr)`: Returns the total capacity of the array.  
- `array_growth_factor(arr)`: Returns the current growth factor.  
- `array_push(arr, item)`: Appends an item to the array.  
- `array_pop(arr, out)`: Removes the last item from the array and outputs it.  
- `array_delete(arr, index)`: Deletes the element at the specified index (shifting subsequent elements).  
- `array_set_min_capacity(arr, min_cap)`: Ensures the array has at least `min_cap` capacity.  
- `array_set_growth_factor(arr, factor)`: Sets the array's growth factor.  
- `array_dup(arr)`: Duplicates the array (shallow copy).  
- `array_free(arr)`: Frees the array and resets the pointer to `NULL`.  
- `array_clear(arr)`: Clears the array (sets the element count to zero).

### Example Usage

```c
#include <stdio.h>
#include "simple_array.h"

typedef struct {
    int x;
    int y;
} Point;

int main(void) {
    Point *points = NULL;

    Point p1 = { 1, 2 };
    Point p2 = { 3, 4 };
    Point p3 = { 5, 6 };
    array_push(points, p1);
    array_push(points, p2);
    array_push(points, p3);

    for (size_t i = 0; i < array_count(points); i++) {
        printf("Point %zu: (%d, %d)\n", i, points[i].x, points[i].y);
    }

    array_delete(points, 1);
    Point *dup_points = array_dup(points);

    Point last;
    array_pop(points, last);
    printf("Popped point: (%d, %d)\n", last.x, last.y);

    array_free(points);
    array_free(dup_points);
    return 0;
}
```

### Configuration

You can override the default settings **before** including the header:

```c
#define ARRAY_INIT_CAPACITY 32
#define ARRAY_GROWTH_FACTOR_DEFAULT 2.5
#include "simple_array.h"
```

---

## Disclaimer

These libraries are provided as-is without any warranties. Use them at your own risk.