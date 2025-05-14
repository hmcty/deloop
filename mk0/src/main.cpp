#include <cstdio>

#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_gpio.h>
#include <stm32f4xx_hal_i2c.h>
#include <stm32f4xx_hal_sai.h>
#include <stm32f4xx_hal_uart.h>

// #include "FreeRTOSConfig.h"
#include <FreeRTOS.h> // Must appear before other FreeRTOS includes
#include <queue.h>
#include <task.h>

#include "audio/luts.hpp"
#include "command.pb.h"
#include "drv/wm8960.hpp"
#include "logging.hpp"
#include "uart_stream.hpp"

extern const std::array<int32_t, SINE_TABLE_SIZE> SINE_TABLE;

static void ConfigureSystemClock(void);
static void ConfigureHALPeripherals(void);
static void ErrorHandler(void);
static void CommandHandler(const Command &cmd);
static void CoreLoopTask(void *pvParameters);

const size_t task_stack_size = configMINIMAL_STACK_SIZE * 2;
static StaticTask_t task_buffer;
static StackType_t task_stack[task_stack_size];

const uint16_t kAudioBufSize = 100;
static uint32_t audio_buf[kAudioBufSize] = {0};
static bool recording = false;
static bool playback = false;

UART_HandleTypeDef uart2_handle = {0};

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
  ConfigureHALPeripherals();

  deloop::Error err = deloop::UartStream::init(&uart2_handle);
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

  // Start the FreeRTOS scheduler.
  vTaskStartScheduler();
  while (1) {
    // Infinite loop. We should never get here.
  };
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

static void ConfigureHALPeripherals(void) {
  // USART2
  uart2_handle.Instance = USART2;
  uart2_handle.Init.BaudRate = 115200;
  uart2_handle.Init.WordLength = UART_WORDLENGTH_8B;
  uart2_handle.Init.StopBits = UART_STOPBITS_1;
  uart2_handle.Init.Parity = UART_PARITY_NONE;
  uart2_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  uart2_handle.Init.Mode = UART_MODE_TX_RX;
  if (HAL_UART_Init(&uart2_handle) != HAL_OK) {
    ErrorHandler();
  }

  HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);

  // TODO: SAI2
}

static void ErrorHandler(void) {
  // Flash LED to indicate error
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
  while (1) {
    // Infinite loop
  }
}

static void CommandHandler(deloop::WM8960 &wm8960, const Command &cmd) {
  // TODO: Breakout larger requests into separate functions.
  switch (cmd.which_request) {
  case Command_reset_tag:
    // Call soft reset function
    NVIC_SystemReset();
    break;

  case Command_configure_recording_tag:
    if (cmd.request.configure_recording.enable != recording) {
      auto error = wm8960.stopRecording();
      if (error != deloop::Error::kOk) {
        DELOOP_LOG_ERROR_FROM_ISR("Failed to stop recording: %d", error);
        deloop::UartStream::sendCommandResponse(CommandResponse{
            .cmd_id = cmd.cmd_id,
            .status = CommandStatus_ERR_INTERNAL,
        });
        break;
      }

      recording = cmd.request.configure_recording.enable;
      if (recording) {
        error = wm8960.startRecording((uint8_t *)audio_buf, kAudioBufSize);
        if (error != deloop::Error::kOk) {
          DELOOP_LOG_ERROR_FROM_ISR("Failed to start recording: %d", error);
          deloop::UartStream::sendCommandResponse(CommandResponse{
              .cmd_id = cmd.cmd_id,
              .status = CommandStatus_ERR_INTERNAL,
          });
          recording = false;
          break;
        }

        DELOOP_LOG_INFO_FROM_ISR("Successfully started recording");
      }

      deloop::UartStream::sendCommandResponse(CommandResponse{
          .cmd_id = cmd.cmd_id,
          .status = CommandStatus_SUCCESS,
      });
    }
    break;
  case Command_configure_playback_tag: {
    ConfigurePlaybackCommand config_request = cmd.request.configure_playback;

    // Handle volume change if provided
    if (config_request.has_volume) {
      float volume = config_request.volume;
      // Clamp volume between 0.0 and 1.0
      if (volume < 0.0f)
        volume = 0.0f;
      if (volume > 1.0f)
        volume = 1.0f;

      auto error = wm8960.setVolume(volume);
      if (error != deloop::Error::kOk) {
        DELOOP_LOG_ERROR_FROM_ISR("Failed to set volume: %d", error);
        deloop::UartStream::sendCommandResponse(CommandResponse{
            .cmd_id = cmd.cmd_id,
            .status = CommandStatus_ERR_INTERNAL,
        });
        break;
      }

      DELOOP_LOG_INFO_FROM_ISR("Volume set to %d%%", (int)(volume * 100));
    }

    // Handle playback state change if needed
    if (config_request.has_enable && config_request.enable != playback) {
      auto error = wm8960.stopPlayback();
      if (error != deloop::Error::kOk) {
        DELOOP_LOG_ERROR_FROM_ISR("Failed to stop playback: %d", error);
        deloop::UartStream::sendCommandResponse(CommandResponse{
            .cmd_id = cmd.cmd_id,
            .status = CommandStatus_ERR_INTERNAL,
        });
        break;
      }

      playback = config_request.enable;
      if (playback) {
        error =
            wm8960.startPlayback((uint8_t *)SINE_TABLE.data(), SINE_TABLE_SIZE);
        // error =
        //     deloop::WM8960::StartPlayback((uint8_t *)audio_buf,
        //     kAudioBufSize);
        if (error != deloop::Error::kOk) {
          DELOOP_LOG_ERROR_FROM_ISR("Failed to start playback: %d", error);
          deloop::UartStream::sendCommandResponse(CommandResponse{
              .cmd_id = cmd.cmd_id,
              .status = CommandStatus_ERR_INTERNAL,
          });
          playback = false;
          break;
        }

        DELOOP_LOG_INFO_FROM_ISR("Successfully started playback");
      }
    }

    deloop::UartStream::sendCommandResponse(CommandResponse{
        .cmd_id = cmd.cmd_id,
        .status = CommandStatus_SUCCESS,
    });
  } break;
  default:
    DELOOP_LOG_ERROR_FROM_ISR("Unknown command received");
    break;
  }
}

static void CoreLoopTask(void *pvParameters) {
  (void)pvParameters;

  DELOOP_LOG_INFO("Starting core loop...");
  deloop::WM8960 wm8960;
  auto err = wm8960.init(I2C1, SAI1_Block_A, SAI1_Block_B);
  if (err != deloop::Error::kOk) {
    DELOOP_LOG_ERROR("Failed to initialize WM8960: %d", err);
    ErrorHandler();
  }

  Command cmd = Command_init_zero;
  QueueHandle_t cmd_queue = deloop::UartStream::getCmdQueue();

  while (1) {
    if (xQueueReceive(cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
      CommandHandler(wm8960, cmd);
    }
  }
}
