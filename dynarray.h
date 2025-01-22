#ifndef DYNARRAY_H_
#define DYNARRAY_H_

#include <stddef.h> // size_t, NULL
#include <stdlib.h> // alloc
#include <string.h> // memcpy

/*
  protocol:
  struct {
      T * items;
      size_t count;
      size_t capacity;
      ...
  };
 */

static const size_t DA_INIT_CAP = 2; // lots of binary operations

// dynamic array append
#define da_append(da, item)                                             \
    do {                                                                \
        if ((da)->count >= (da)->capacity) {                            \
            size_t new_capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2; \
            (da)->items = reallocf((da)->items, new_capacity * sizeof((da)->items[0])); \
            (da)->capacity = new_capacity;                              \
        }                                                               \
        (da)->items[(da)->count++] = (item);                            \
    } while (0)

#define da_free(da)                             \
    do {                                        \
        free((da)->items);                      \
        (da)->items = NULL;                     \
        (da)->count = 0;                        \
        (da)->capacity = 0;                     \
    } while (0)

#define da_copy(dst, src)                                               \
    do {                                                                \
        (dst)->items = reallocf((dst)->items, (src)->capacity * sizeof((src)->items[0])); \
        memcpy((dst)->items, (src)->items, (src)->capacity * sizeof((src)->items[0])); \
    } while (0)

#endif // DYNARRAY_H_
