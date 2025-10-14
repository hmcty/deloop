#ifndef DLP_COUNTER_H
#define DLP_COUNTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "error.h"
#include "track.h"

typedef struct dlp_counter {
  uint64_t cnt;
  uint64_t len;
} dlp_counter_t;

dlp_error_t dlp_counter_init(void);
dlp_error_t dlp_counter_deinit(void);
void dlp_counter_advance_all(uint64_t step);
void dlp_counter_advance(dlp_track_id_t id, uint64_t step);

uint64_t dlp_counter_absolute(dlp_track_id_t id);
uint64_t dlp_counter_next_loop(dlp_track_id_t id);

void dlp_counter_set_cnt(dlp_track_id_t id, uint64_t val);
void dlp_counter_set_len(dlp_track_id_t id, uint64_t val);

#ifdef __cplusplus
}
#endif

#endif // DLP_COUNTER_H
