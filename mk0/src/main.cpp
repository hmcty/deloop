#include <cstdio>
#include <string>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "drv/wm8960.hpp"
#include "tasks/tasks.hpp"

static void ConfigureSystemClock(void);
static void ErrorHandler(void);
static void LEDBlinkTask(void *pvParameters);

const size_t task_stack_size = configMINIMAL_STACK_SIZE * 2;
static StaticTask_t task_buffer;
static StackType_t task_stack[task_stack_size];

static UART_HandleTypeDef UART_handle;

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

  // Initialize UART.
  UART_handle.Instance = USART2;
  UART_handle.Init.BaudRate = 115200;
  UART_handle.Init.WordLength = UART_WORDLENGTH_8B;
  UART_handle.Init.StopBits = UART_STOPBITS_1;
  UART_handle.Init.Parity = UART_PARITY_NONE;
  UART_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  UART_handle.Init.Mode = UART_MODE_TX_RX;
  if (HAL_UART_Init(&UART_handle) != HAL_OK) {
    ErrorHandler();
  }

//HAL_UART_Transmit(
//  &UART_handle,
//  reinterpret_cast<uint8_t *>(const_cast<char *>(hello.c_str())),
//  hello.size(),
//  0xFFFF);

  // Initialize LED.
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  xTaskCreateStatic(LEDBlinkTask, "Task 1", task_stack_size, NULL, 1,
                    &(task_stack[0]), &task_buffer);

//if (WM8960::Init() != 0) {
//  ErrorHandler();
//}
//AddSineWaveTask();

  // Start the FreeRTOS scheduler.
  vTaskStartScheduler();

  // Infinite loop
  while (1) {
  //HAL_UART_Transmit(
  //  &UART_handle,
  //  reinterpret_cast<uint8_t *>(const_cast<char *>(hello.c_str())),
  //  hello.size(),
  //  0xFFFFFFFF);
  //HAL_Delay(1000);
  }
}

// int __io_putchar(int ch) {
//   /* Place your implementation of fputc here */
//   /* e.g. write a character to the USART3 and Loop until the end of
//   transmission */ HAL_UART_Transmit(&UART_handle, (uint8_t *)&ch, 1, 0xFFFF);

//  return ch;
//}

static void LEDBlinkTask(void *pvParameters) {
  while (1) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    // HAL_Delay(500);
    // vTaskDelay(5000);
    std::string hello = "Hello, World!\r\n";
    HAL_UART_Transmit(
      &UART_handle,
      reinterpret_cast<uint8_t *>(const_cast<char *>(hello.c_str())),
      hello.size(),
      0xFFFFFFFF);
    vTaskDelay(5000);
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
  while (1) {}
}
