#pragma once

#include <stdint.h>

#include <bizzy/track.h>

int bizzy_init();
void bizzy_start();
void bizzy_stop();
void bizzy_cleanup();
bizzy_track_t *bizzy_get_track();
void bizzy_set_track_duration(bizzy_track_t *track, uint32_t duration_s);


