// Simple implementation of SPSC ring buffer.
//
// Inspired by: github.com/mgeier/rtrb (MIT License)

#include "ringbuf.h"

// #include <stdatomic.h>
#include <string.h>

#include "error.h"

#define COLLAPSE(i, cap) (i < cap ? i : i - cap)
#define WRITE_PTR(rb)                                                          \
  ((uint8_t *)rb->data + (COLLAPSE(rb->write, rb->capacity) * rb->item_size))
#define READ_PTR(rb)                                                           \
  ((uint8_t *)rb->data + (COLLAPSE(rb->read, rb->capacity) * rb->item_size))
#define DISTANCE(a, b, cap) (a <= b ? b - a : 2 * cap - a + b)
#define TOTAL_ALLOCATION(rb) (DISTANCE(rb->read, rb->write, rb->capacity))
#define NEXT(i, cap) (i < (2 * cap - 1) ? i + 1 : 0)
#define NEXT_WRITE(rb) NEXT((rb)->write, (rb)->capacity)
#define NEXT_READ(rb) NEXT((rb)->read, (rb)->capacity)

dlp_error_t dlp_ringbuf_init(dlp_ringbuf_t *rb) {
  if (rb == NULL || rb->data == NULL || rb->capacity <= 1) {
    return DLP_ERROR_INVALID_ARGUMENT;
  }

  rb->write = 0;
  rb->read = 0;

  return DLP_SUCCESS;
}

dlp_error_t dlp_ringbuf_push(dlp_ringbuf_t *rb, const void *item) {
  if (rb == NULL || item == NULL) {
    return DLP_ERROR_INVALID_ARGUMENT;
  } else if (TOTAL_ALLOCATION(rb) >= rb->capacity) {
    return DLP_ERROR_RINGBUF_FULL;
  }

  memcpy(WRITE_PTR(rb), item, rb->item_size);
  rb->write = NEXT_WRITE(rb);
  return DLP_SUCCESS;
}

dlp_error_t dlp_ringbuf_pop(dlp_ringbuf_t *rb, void *item) {
  if (rb == NULL || item == NULL) {
    return DLP_ERROR_INVALID_ARGUMENT;
  } else if (rb->read == rb->write) {
    return DLP_ERROR_RINGBUF_EMPTY;
  }

  memcpy(item, READ_PTR(rb), rb->item_size);
  rb->read = NEXT_READ(rb);
  return DLP_SUCCESS;
}
