#ifndef DYNARRAY_H_
#define DYNARRAY_H_

#include <stddef.h> // size_t
#include <stdlib.h> // memory

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
            (da)->items = realloc((da)->items, new_capacity*sizeof(*(da)->items)); \
            (da)->capacity = new_capacity;                              \
        }                                                               \
        (da)->items[(da)->count++] = (item);                            \
    } while (0)

#define da_free(da)                             \
    do {                                        \
        free((da)->items);                      \
        (da)->count = 0;                        \
        (da)->capacity = 0;                     \
    } while (0)

#endif // DYNARRAY_H_
