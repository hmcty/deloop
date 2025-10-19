#pragma once

#include <daisy_seed.h>

void setup_display(daisy::SpiHandle *spi, daisy::GPIO *dc, daisy::GPIO *rst);
void display_tick();
