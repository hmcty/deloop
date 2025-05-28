#pragma once

#if defined(STM32F446xx)
#include <stm32f4xx.h>
#elif defined(STM32H750xx)
#include <stm32h7xx.h>
#else
#error "Unsupported STM32 target."
#endif
