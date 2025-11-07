#ifndef DLP_ERROR_H
#define DLP_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef enum dlp_error {
  DLP_SUCCESS = 0,
  DLP_ERROR_INVALID_ARGUMENT,
  DLP_ERROR_NOT_INITIALIZED,
  DLP_ERROR_ALREADY_INITIALIZED,
  DLP_ERROR_RINGBUF_FULL,
  DLP_ERROR_RINGBUF_EMPTY,
  DLP_ERROR_INTERNAL,
  DLP_ERROR_NO_TRACK_STATUS,
} dlp_error_t;

#ifdef __cplusplus
}
#endif

#endif // DLP_ERROR_H
