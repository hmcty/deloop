#pragma once

#include <stdint.h>

#include <bizzy/track.h>

int bizzy_init();
void bizzy_start();
void bizzy_stop();
void bizzy_cleanup();
bizzy_track_t *bizzy_get_track();


