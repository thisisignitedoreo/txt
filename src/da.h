
// da.h by aciddev
//
// Inspired by dynarray.h (https://github.com/eignnx/dynarray/), but done as
// header only library, and with more functionality
//
// Instuctions:
// To use: Define DA_IMPL and include this header ONCE.
// In all other files just include it, without any definitions.

#ifndef DA_H_
#define DA_H_

// Header file

#include <stdlib.h>
#include <string.h>

#define DA_START_SIZE 1

#define DA_STRIDE 0
#define DA_LENGTH 1
#define DA_CAPACITY 2
#define DA_FIELDS 3

#define da_new(type) \
    _da_new(DA_START_SIZE, sizeof(type))
// -> Create new array of type TYPE. Define as TYPE*.

#define da_push(arr, x) \
    do { \
        __auto_type temp = x; \
        arr = _da_push(arr, &temp); \
    } while (0)
// -> Push an item or rval into array. Will evaluate an rval before doing so.

#define da_length(arr) _da_get(arr, DA_LENGTH)
// -> Get an amount of items in an array

#define da_capacity(arr) _da_get(arr, DA_CAPACITY)
// -> Get amount of items allocated 

#define da_stride(arr) _da_get(arr, DA_STRIDE)
// -> Get size of an item in bytes

void* _da_new(size_t size, size_t stride);
// -> Create new array of size SIZE and stride STRIDE

void* _da_resize(void* array);
// -> Resize the array to double its capacity

size_t _da_get(void* array, int field);
// -> Get a header field

void _da_set(void* array, int field, size_t value);
// -> Set a header field

void* _da_push(void* array, void* elementptr);
// -> Push an item onto array through its pointer

void da_pop(void* array, void* toptr);
// -> Pop off the last array element into TOPTR. It can be nullptr, if so, nothing will be written to it.

void da_free(void* array);
// -> Destroy and free the array

#ifdef DA_IMPL

void* _da_new(size_t capacity, size_t stride) {
    int size = sizeof(size_t) * DA_FIELDS + capacity*stride;
    size_t* array = malloc(size);
    array[DA_STRIDE] = stride;
    array[DA_LENGTH] = 0;
    array[DA_CAPACITY] = capacity;
    return array + DA_FIELDS;
}

void _da_set(void* array, int field, size_t value) {
    (((size_t*)array) - DA_FIELDS)[field] = value;
}

size_t _da_get(void* array, int field) {
    return (((size_t*)array) - DA_FIELDS)[field];
}

void da_free(void* array) {
    free(array - sizeof(size_t) * DA_FIELDS);
}

void* _da_resize(void* array) {
    void* temp = _da_new(da_capacity(array) * 2, da_stride(array));
    memcpy(temp, array, da_length(array) * da_stride(temp));
    _da_set(temp, DA_LENGTH, da_length(array));
    da_free(array);
    return temp;
}

void* _da_push(void* array, void* elementptr) {
    if (da_length(array) >= da_capacity(array)) array = _da_resize(array);
    
    memcpy(array + da_length(array)*da_stride(array), elementptr, da_stride(array));

    _da_set(array, DA_LENGTH, da_length(array)+1);
    return array;
}

void da_pop(void* array, void* toptr) {
    if (toptr != 0) memcpy(toptr, array + (da_length(array)-1)*da_stride(array), da_stride(array));

    _da_set(array, DA_LENGTH, da_length(array)-1);
}

void* da_push_many(void* array, void* items, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        array = _da_push(array, items + i * da_stride(array));
    }
    return array;
}

#endif // DA_IMPL

#endif // DA_H_

