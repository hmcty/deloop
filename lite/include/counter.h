#ifndef DELOOP_COUNTER_H
#define DELOOP_COUNTER_H

#include <stdint.h>

typedef struct deloop_counter {
  uint64_t cnt;
  uint64_t len;
} deloop_counter_t;

void deloop_counter_init(uint64_t rate);
void deloop_counter_advance_all(uint64_t step);
void deloop_counter_advance(deloop_track_id_t id, uint64_t step);

uint64_t deloop_counter_absolute(deloop_track_id_t id);
uint64_t deloop_counter_next_loop(deloop_track_id_t id);

void deloop_counter_set_cnt(deloop_track_id_t id, uint64_t val);
void deloop_counter_set_len(deloop_track_id_t id, uint64_t val);

#endif
