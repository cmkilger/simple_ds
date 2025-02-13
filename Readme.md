# Simple Hash Library

A header-only C library implementing an open-addressing hash table with linear probing. This lightweight library provides a flexible, dynamic hash table that automatically resizes based on configurable load factors and growth factors.

Inspired by [std_bs](https://github.com/nothings/stb/blob/master/stb_ds.h) and [uthash](https://github.com/troydhanson/uthash), this library enforces its own usage requirements and was primarily written by ChatGPT (including this file).

## Overview

The library stores a hidden header immediately before the user array to track:
- **Count:** The number of elements in the table.
- **Capacity:** The total number of buckets.
- **Load Factor:** The threshold ratio of filled buckets that triggers resizing.
- **Growth Factor:** The multiplier used to increase capacity when resizing.

It uses the djb2 hash function for string keys and supports dynamic resizing to maintain performance.

## Features

- **Header-only Implementation:** Simply include the header in your project.
- **Dynamic Resizing:** Automatically grows the table when the load factor is exceeded.
- **Open Addressing with Linear Probing:** Efficient collision resolution.
- **Flexible API Macros:** Macros to insert, retrieve, delete, duplicate, and configure the hash table.
- **Configurable Parameters:** Override initial capacity, load factor, and growth factor at compile time.

## Installation

Copy the `simple_hash.h` header file into your project directory and include it in your source files:

```c
#include "simple_hash.h"
```

## Element Requirements and Gotchas

- **Element Structure Requirement:**  
  Each element stored in the table **must** have a field named `key` (of type `const char*`) as its **first member**. For example:

  ```c
  typedef struct {
      const char* key;  // Must be the first member
      int value;
  } MyItem;
  ```

- **Macro Argument Requirements:**  
  When calling any of the provided macros (e.g., `hash_put`, `hash_get`, etc.), the table must be passed as an **lvalue** (a modifiable variable). This is important because many macros update the table pointer internally (for instance, during a resize).

- **Passing the Table to Functions:**  
  If you need to pass the table to another function for modification, pass a pointer to the table pointer (i.e., a pointer to a pointer). This ensures that any updates made by the macros (such as reallocation or resizing) are reflected in the caller’s variable.

- **Memory Management:**  
  The `hash_free` macro frees the entire hash table (including its hidden header) and sets the table pointer to `NULL`. Be cautious not to use the table after freeing it without reinitialization.

- **Side Effects:**  
  Since the API relies heavily on macros, avoid passing expressions with side effects as arguments to these macros.

## API Overview

- `hash_count(tbl)`: Returns the number of elements in the table.
- `hash_capacity(tbl)`: Returns the total number of buckets.
- `hash_load_factor(tbl)`: Returns the current load factor.
- `hash_growth_factor(tbl)`: Returns the current growth factor.
- `hash_put(tbl, item)`: Inserts a new element or updates an existing one.
- `hash_get(tbl, key)`: Retrieves a pointer to the element with the given key.
- `hash_delete(tbl, key)`: Removes the element with the given key.
- `hash_set_min_capacity(tbl, min_cap)`: Ensures that the table has at least `min_cap` buckets.
- `hash_set_growth_factor(tbl, factor)`: Sets the table's growth factor.
- `hash_set_load_factor(tbl, factor)`: Sets the table's load factor threshold.
- `hash_dup(tbl)`: Duplicates the table (shallow copy).
- `hash_free(tbl)`: Frees the table and resets the pointer to `NULL`.

## Example Usage

Below is an example demonstrating basic operations:

```c
#include <stdio.h>
#include "simple_hash.h"

typedef struct {
    const char* key;  // Must be the first field
    int value;
} MyItem;

// Function that accepts a pointer to the table pointer
void insert_item(MyItem **table_ptr, MyItem item) {
    // Note: Pass the table as an lvalue so that the macro can update it if necessary.
    hash_put(*table_ptr, item);
}

int main(void) {
    // Initialize an empty table.
    MyItem *table = NULL;
    
    // Insert items into the table.
    MyItem apple = { "apple", 10 };
    MyItem banana = { "banana", 20 };
    hash_put(table, apple);
    hash_put(table, banana);
    
    // Alternatively, use a function that modifies the table.
    MyItem cherry = { "cherry", 30 };
    insert_item(&table, cherry);
    
    // Retrieve an item.
    MyItem *item = hash_get(table, "apple");
    if (item) {
        printf("Found %s: %d\n", item->key, item->value);
    } else {
        printf("Item not found\n");
    }
    
    // Delete an item.
    hash_delete(table, "banana");
    
    // Duplicate the table.
    MyItem *dup_table = hash_dup(table);
    
    // Print table info.
    printf("Original table count: %zu, capacity: %zu\n", hash_count(table), hash_capacity(table));
    printf("Duplicate table count: %zu, capacity: %zu\n", hash_count(dup_table), hash_capacity(dup_table));
    
    // Free the tables.
    hash_free(table);
    hash_free(dup_table);
    
    return 0;
}
```

## Configuration

You can override the default settings at compile time by defining the following macros **before** including the header:

- **Initial Capacity:**  
  `HASH_INIT_CAPACITY` (default: 16)

- **Load Factor:**  
  `HASH_LOAD_FACTOR` (default: 0.75)

- **Growth Factor:**  
  `HASH_GROWTH_FACTOR_DEFAULT` (default: 2.0)

For example:

```c
#define HASH_INIT_CAPACITY 32
#define HASH_LOAD_FACTOR 0.8
#define HASH_GROWTH_FACTOR_DEFAULT 3.0
#include "simple_hash.h"
```

## Disclaimer

This library is provided as-is without any warranties. Use it at your own risk.