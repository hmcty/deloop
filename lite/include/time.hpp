#pragma once

// libdaisy
#include <sys/system.h>
#define GET_NOW_MS() daisy::System::GetNow()
#define DELAY_MS(ms) daisy::System::Delay(ms)
