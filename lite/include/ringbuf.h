#ifndef DLP_RINGBUF_H
#define DLP_RINGBUF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdatomic.h>

#include "error.h"

typedef struct dlp_ringbuf {
  void *const data;
  const size_t item_size;
  const size_t capacity;
  size_t write;
  size_t read;
} dlp_ringbuf_t;

dlp_error_t dlp_ringbuf_init(dlp_ringbuf_t *rb);
dlp_error_t dlp_ringbuf_push(dlp_ringbuf_t *rb, const void *item);
dlp_error_t dlp_ringbuf_pop(dlp_ringbuf_t *rb, void *item);

#ifdef __cplusplus
}
#endif

#endif // DLP_RING_BUFFER_H
