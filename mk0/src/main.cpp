#include <cstdio>
#include <string>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_wwdg.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "logging.hpp"

#include "uart_stream.hpp"

#include "drv/wm8960.hpp"
#include "tasks/tasks.hpp"

static void ConfigureSystemClock(void);
static void ErrorHandler(void);
static void CoreLoopTask(void *pvParameters);

const size_t task_stack_size = configMINIMAL_STACK_SIZE * 2;
static StaticTask_t task_buffer;
static StackType_t task_stack[task_stack_size];

static uint32_t tx_data[22000] = {0};

int main(void) {
  // STM32F4xx HAL library initialization:
  //    - Configure the Flash prefetch and Buffer caches
  //    - Systick timer is configured by default as source of time base, but
  //      user can eventually implement his proper time base source (a general
  //      purpose timer for example or other time source), keeping in mind
  //      that Time base duration should be kept 1ms since PPP_TIMEOUT_VALUEs
  //      are defined and handled in milliseconds basis.
  //    - Low Level Initialization
  HAL_Init();
  ConfigureSystemClock();

  deloop::Error err = deloop::UartStream::Init();
  if (err != deloop::Error::kOk) {
    ErrorHandler();
  }

  // Initialize LED.
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  xTaskCreateStatic(CoreLoopTask, "Core Loop", task_stack_size, NULL, 1,
                    &(task_stack[0]), &task_buffer);
  // AddSineWaveTask();

  err = deloop::WM8960::Init();
  if (err != deloop::Error::kOk) {
    DELOOP_LOG_ERROR("Failed to initialize WM8960: %d", err);
  }

  // Start the FreeRTOS scheduler.
  vTaskStartScheduler();

  // Infinite loop. We should never get here.
  while (1) {}
}

static void CoreLoopTask(void *pvParameters) {
  (void)pvParameters;

  DELOOP_LOG_INFO("Starting core loop...");

  // uint8_t tx_data[6] = {0xDE, 0xAD, 0x12, 0x01, 0x02, 0x03};
  uint8_t data[4] = {0};
  uint32_t avg_left = 0;
  uint32_t avg_right = 0;
  uint32_t num_ticks = 0;
  deloop::WM8960::ExchangeData((uint8_t *) tx_data, data, 22000);
  while (1) {
    DELOOP_LOG_INFO("Core loop...");
    vTaskDelay(100);
  }
}

// System Clock Configuration.
//   System Clock source            = PLL (HSE)
//   SYSCLK(Hz)                     = 180000000
//   HCLK(Hz)                       = 180000000
//   AHB Prescaler                  = 1
//   APB1 Prescaler                 = 4
//   APB2 Prescaler                 = 2
//   HSE Frequency(Hz)              = 8000000
//   PLL_M                          = 8
//   PLL_N                          = 360
//   PLL_P                          = 2
//   PLL_Q                          = 7
//   PLL_R                          = 2
//   VDD(V)                         = 3.3
//   Main regulator output voltage  = Scale1 mode
//   Flash Latency(WS)              = 5
static void ConfigureSystemClock(void) {
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  // Enable Power Control clock.
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device
     is clocked below the maximum system frequency, to update the voltage
     scaling value regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    ErrorHandler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    ErrorHandler();
  }
}

static void ErrorHandler(void) {
  while (1) {
  }
}
