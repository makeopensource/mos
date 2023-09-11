#include "ring_buffer.h"

// yay, undefined behavior reliance!
typedef ring_buffer(2) _ring_buffer_placeholder_t;

bool ring_buffer_full(const void *ringbuf) {
    const _ring_buffer_placeholder_t *rb =
        (_ring_buffer_placeholder_t *)(ringbuf);
    return rb->used == rb->capacity;
}

bool ring_buffer_empty(const void *ringbuf) {
    const _ring_buffer_placeholder_t *rb =
        (_ring_buffer_placeholder_t *)(ringbuf);
    return rb->used == 0;
}

void _ring_buffer_push_stub(void *ringbuf) {
    _ring_buffer_placeholder_t *rb = (_ring_buffer_placeholder_t *)(ringbuf);

    // update end
    ++rb->end;
    rb->end %= rb->capacity;

    // update used amount
    ++rb->used;
    if (rb->used > rb->capacity) {
        // adjust start
        rb->start += rb->used - rb->capacity;
        rb->start %= rb->capacity;

        rb->used = rb->capacity;
    }
}

// appends a new element to the ring buffer,
// will overwrite when at capacity
void ring_buffer_push(void *ringbuf, uint8_t elem) {
    _ring_buffer_placeholder_t *rb = (_ring_buffer_placeholder_t *)(ringbuf);

    ring_buffer_push_g(rb, elem);
}

// returns the top element of the buffer, 0 if empty
uint8_t ring_buffer_top(const void *ringbuf) {
    const _ring_buffer_placeholder_t *rb =
        (_ring_buffer_placeholder_t *)(ringbuf);

    if (ring_buffer_empty(ringbuf))
        return 0;

    return ring_buffer_top_g(rb);
}

void _ring_buffer_pop_stub(void *ringbuf) {
    _ring_buffer_placeholder_t *rb = (_ring_buffer_placeholder_t *)(ringbuf);

    ++rb->start;
    rb->start %= rb->capacity;

    --rb->used;
}

// removes the top element of the buffer, 0 if empty
uint8_t ring_buffer_pop(void *ringbuf) {
    // edge-case of empty buffer
    if (ring_buffer_empty(ringbuf))
        return 0;

    _ring_buffer_placeholder_t *rb = (_ring_buffer_placeholder_t *)(ringbuf);

    uint8_t out;
    ring_buffer_pop_g(rb, out);

    return out;
}