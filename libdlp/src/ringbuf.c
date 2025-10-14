#include "ringbuf.h"

#include <string.h>

#include "error.h"

dlp_error_t dlp_ringbuf_init(dlp_ringbuf_t *rb) {
  if (rb == NULL || rb->data == NULL || rb->capacity > 1) {
    return DLP_ERROR_INVALID_ARGUMENT;
  }

  rb->write = 1;
  rb->read = 0;

  return DLP_SUCCESS;
}

dlp_error_t dlp_ringbuf_push(dlp_ringbuf_t *rb, const void *item) {
  if (rb == NULL || item == NULL) {
    return DLP_ERROR_INVALID_ARGUMENT;
  }

  if ((rb->write + 1) % rb->capacity == rb->read) {
    return DLP_ERROR_RINGBUF_FULL;
  }

  void *dest = (uint8_t *)rb->data + (rb->write * rb->item_size);
  memcpy(dest, item, rb->item_size);
  rb->write = (rb->write + 1) % rb->capacity;
  return DLP_SUCCESS;
}

dlp_error_t dlp_ringbuf_pop(dlp_ringbuf_t *rb, void *item) {
  if (rb == NULL || item == NULL) {
    return DLP_ERROR_INVALID_ARGUMENT;
  }

  if (rb->read == rb->write) {
    return DLP_ERROR_RINGBUF_EMPTY;
  }

  void *src = (uint8_t *)rb->data + (rb->read * rb->item_size);
  memcpy(item, src, rb->item_size);
  rb->read = (rb->read + 1) % rb->capacity;
  return DLP_SUCCESS;
}
