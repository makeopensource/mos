#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Header only generic ring buffer data structure
// to use larger elements, multiply your size by sizeof(data_t),
// then call push/pop sizeof(data_t) times. Beware of endianess.

// The magic generic macro, size > 0, otherwise behavior is undefined
// we do have to store the length since C lacks generic support.
// Type generic version.
#define ring_buffer_g(size, data_t)                                            \
    struct {                                                                   \
        size_t start;                                                          \
        size_t end;                                                            \
        size_t capacity;                                                       \
        size_t used;                                                           \
                                                                               \
        data_t buffer[size];                                                   \
    }

#define ring_buffer(size) ring_buffer_g(size, uint8_t)

// generic initializer for a ringbuffer. Sets all data to initialVal
#define ring_buffer_init(ringbuf, initialVal)                                  \
    {                                                                          \
        (ringbuf)->start = 0;                                                  \
        (ringbuf)->end = 0;                                                    \
        (ringbuf)->capacity =                                                  \
            sizeof((ringbuf)->buffer) / sizeof((ringbuf)->buffer[0]);          \
        (ringbuf)->used = 0;                                                   \
        for (int rbig_i = 0; rbig_i < (ringbuf)->capacity; ++rbig_i) {         \
            (ringbuf)->buffer[rbig_i] = initialVal;                            \
        }                                                                      \
    }

bool ring_buffer_full(const void *ringbuf);
bool ring_buffer_empty(const void *ringbuf);

// internal use function. DO NOT CALL THIS.
void _ring_buffer_push_stub(void *ringbuf);

// appends a new element to the ring buffer,
// works with any type
#define ring_buffer_push_g(ringbuf, elem)                                      \
    {                                                                          \
        (ringbuf)->buffer[(ringbuf)->end] = elem;                              \
        _ring_buffer_push_stub(ringbuf);                                       \
    }

// appends a new element to the ring buffer,
// will overwrite when at capacity
void ring_buffer_push(void *ringbuf, uint8_t elem);

// returns the top element of the buffer. DOES NOT CHECK FOR EMPTY
#define ring_buffer_top_g(ringbuf) ((ringbuf)->buffer[(ringbuf)->start])

// returns the top element of the buffer, 0 if empty
uint8_t ring_buffer_top(const void *ringbuf);

// internal use function. DO NOT CALL THIS.
void _ring_buffer_pop_stub(void *ringbuf);

// removes the top element of the buffer. DOES NOT CHECK FOR EMPTY
#define ring_buffer_pop_g(ringbuf, dest)                                       \
    {                                                                          \
        dest = ring_buffer_top_g(ringbuf);                                     \
        _ring_buffer_pop_stub(ringbuf);                                        \
    }

// removes the top element of the buffer, 0 if empty
uint8_t ring_buffer_pop(void *ringbuf);

#endif