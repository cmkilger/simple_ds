# Simple Map Library

A header-only C library implementing an open-addressing hash map with linear probing. This lightweight library provides a flexible, dynamic hash map that automatically resizes based on configurable load factors and growth factors.

Inspired by [stb_ds](https://github.com/nothings/stb/blob/master/stb_ds.h) and [uthash](https://github.com/troydhanson/uthash), this library enforces its own usage requirements and was primarily written by ChatGPT (including this file).

## Overview

The library stores a hidden header immediately before the user array to track:
- **Count:** The number of elements in the map.
- **Capacity:** The total number of buckets.
- **Load Factor:** The threshold ratio of filled buckets that triggers resizing.
- **Growth Factor:** The multiplier used to increase capacity when resizing.

It uses the djb2 hash function for string keys and supports dynamic resizing to maintain performance.

## Features

- **Header-only Implementation:** Simply include the header in your project.
- **Dynamic Resizing:** Automatically grows the map when the load factor is exceeded.
- **Open Addressing with Linear Probing:** Efficient collision resolution.
- **Flexible API Macros:** Macros to insert, retrieve, delete, duplicate, and configure the hash map.
- **Configurable Parameters:** Override initial capacity, load factor, and growth factor at compile time.

## Installation

Copy the `simple_map.h` header file into your project directory and include it in your source files:

```c
#include "simple_map.h"
```

## Element Requirements and Gotchas

- **Element Structure Requirement:**  
  Each element stored in the map **must** have a field named `key` (of type `const char*`) as its **first member**. For example:

```c
typedef struct {
    const char* key;  // Must be the first member
    int value;
} MyItem;
```

- **Macro Argument Requirements:**  
  When calling any of the provided macros (e.g., `map_put`, `map_get`, etc.), the map must be passed as an **lvalue** (a modifiable variable). This is important because many macros update the map pointer internally (for instance, during a resize).

- **Passing the Map to Functions:**  
  If you need to pass the map to another function for modification, pass a pointer to the map pointer (i.e., a pointer to a pointer). This ensures that any updates made by the macros (such as reallocation or resizing) are reflected in the caller’s variable.

- **Memory Management:**  
  The `map_free` macro frees the entire hash map (including its hidden header) and sets the map pointer to `NULL`. Be cautious not to use the map after freeing it without reinitialization.

- **Side Effects:**  
  Since the API relies heavily on macros, avoid passing expressions with side effects as arguments to these macros.

## API Overview

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

## Example Usage

Below is an example demonstrating basic operations:

```c
#include <stdio.h>
#include "simple_map.h"

typedef struct {
    const char* key;  // Must be the first field
    int value;
} MyItem;

// Function that accepts a pointer to the map pointer
void insert_item(MyItem **map_ptr, MyItem item) {
    // Note: Pass the map as an lvalue so that the macro can update it if necessary.
    map_put(*map_ptr, item);
}

int main(void) {
    // Initialize an empty map.
    MyItem *map = NULL;
    
    // Insert items into the map.
    MyItem apple = { "apple", 10 };
    MyItem banana = { "banana", 20 };
    map_put(map, apple);
    map_put(map, banana);
    
    // Alternatively, use a function that modifies the map.
    MyItem cherry = { "cherry", 30 };
    insert_item(&map, cherry);
    
    // Retrieve an item.
    MyItem *item = map_get(map, "apple");
    if (item) {
        printf("Found %s: %d\n", item->key, item->value);
    } else {
        printf("Item not found\n");
    }
    
    // Delete an item.
    map_delete(map, "banana");
    
    // Duplicate the map.
    MyItem *dup_map = map_dup(map);
    
    // Print map info.
    printf("Original map count: %zu, capacity: %zu\n", map_count(map), map_capacity(map));
    printf("Duplicate map count: %zu, capacity: %zu\n", map_count(dup_map), map_capacity(dup_map));
    
    // Free the maps.
    map_free(map);
    map_free(dup_map);
    
    return 0;
}
```

## Configuration

You can override the default settings at compile time by defining the following macros **before** including the header:

- **Initial Capacity:**  
  `MAP_INIT_CAPACITY` (default: 16)

- **Load Factor:**  
  `MAP_LOAD_FACTOR` (default: 0.75)

- **Growth Factor:**  
  `MAP_GROWTH_FACTOR_DEFAULT` (default: 2.0)

For example:

```c
#define MAP_INIT_CAPACITY 32
#define MAP_LOAD_FACTOR 0.8
#define MAP_GROWTH_FACTOR_DEFAULT 3.0
#include "simple_map.h"
```

## Disclaimer

This library is provided as-is without any warranties. Use it at your own risk.