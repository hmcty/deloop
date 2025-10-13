#ifndef DLP_COUNTER_H
#define DLP_COUNTER_H

#include <stdint.h>

#include "error.h"
#include "track.h"

typedef struct dlp_counter {
  uint64_t cnt;
  uint64_t len;
} dlp_counter_t;

dlp_error_t dlp_counter_init(uint64_t rate);
void dlp_counter_advance_all(uint64_t step);
void dlp_counter_advance(dlp_track_id_t id, uint64_t step);

uint64_t dlp_counter_absolute(dlp_track_id_t id);
uint64_t dlp_counter_next_loop(dlp_track_id_t id);

void dlp_counter_set_cnt(dlp_track_id_t id, uint64_t val);
void dlp_counter_set_len(dlp_track_id_t id, uint64_t val);

#endif
