#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_i2s.h"

/*
#define USARTx                           USART3
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART3_CLK_ENABLE();
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOD_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOD_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __HAL_RCC_USART3_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART3_RELEASE_RESET()

#define USARTx_TX_PIN                    GPIO_PIN_8
#define USARTx_TX_GPIO_PORT              GPIOD
#define USARTx_TX_AF                     GPIO_AF7_USART3
#define USARTx_RX_PIN                    GPIO_PIN_9
#define USARTx_RX_GPIO_PORT              GPIOD
#define USARTx_RX_AF                     GPIO_AF7_USART3
*/

#define USARTx USART2
#define USARTx_CLK_ENABLE() __HAL_RCC_USART2_CLK_ENABLE();
#define USARTx_RX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()

#define USARTx_FORCE_RESET() __HAL_RCC_USART2_FORCE_RESET()
#define USARTx_RELEASE_RESET() __HAL_RCC_USART2_RELEASE_RESET()

#define USARTx_TX_PIN GPIO_PIN_2
#define USARTx_TX_GPIO_PORT GPIOA
#define USARTx_TX_AF GPIO_AF7_USART2
#define USARTx_RX_PIN GPIO_PIN_3
#define USARTx_RX_GPIO_PORT GPIOA
#define USARTx_RX_AF GPIO_AF7_USART2

void HAL_MspInit(void) {
  // Ensure all priority bits are assigned as preemption priority bits.
  // Required by FreeRTOS.
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  // TODO: Set system interrupt priorities?
}

void HAL_MspDeInit(void) {}

void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s) {
  if (hi2s->Instance != SPI2) {
    return;
  }

  // PLL configuration for I2S clock.
  // PLLI2SM - Division factor for PLL VCO input (must be between 2 and 63).
  // PLLI2SN - Multiplication factor for PLLI2S VCO output (must be between
  //           50 and 432).
  // PLLI2SR - Division factor for I2S clock (must be between 2 and 7).
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S_APB1;
  PeriphClkInitStruct.PLLI2S.PLLI2SM = 8;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

  __HAL_RCC_SPI2_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // I2S2 GPIO Configuration
  // * PB_15 -> I2S_DAC / SPI2_MOSI
  // * PB_14 -> I2S_ADC / SPI2_MISO
  // * PC_13  -> I2S_CLK / SPI2_SCK
  // * PB_12  -> I2S_WS  / SPI2_CS
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = GPIO_PIN_15 | GPIO_PIN_14 | GPIO_PIN_13 | GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Enable I2S DMA clock.
  __HAL_RCC_DMA1_CLK_ENABLE();

  // Configure the DMA handler for transmission process
  static DMA_HandleTypeDef hdma_tx;
  hdma_tx.Instance = DMA1_Stream4;
  hdma_tx.Init.Channel = DMA_CHANNEL_0;
  hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_tx.Init.Mode = DMA_CIRCULAR;
  hdma_tx.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  HAL_DMA_Init(&hdma_tx);
  __HAL_LINKDMA(hi2s, hdmatx, hdma_tx);

  // Configure the DMA handler for reception process
  static DMA_HandleTypeDef hdma_rx;
  hdma_rx.Instance = DMA1_Stream3;
  hdma_rx.Init.Channel = DMA_CHANNEL_0;
  hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_rx.Init.Mode = DMA_CIRCULAR;
  hdma_rx.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  HAL_DMA_Init(&hdma_rx);
  __HAL_LINKDMA(hi2s, hdmarx, hdma_rx);
}

void HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s) {
  if (hi2s->Instance == SPI2) {
    __HAL_RCC_SPI2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_15 | GPIO_PIN_14 | GPIO_PIN_13 | GPIO_PIN_12);
    HAL_DMA_DeInit(hi2s->hdmatx);
    HAL_DMA_DeInit(hi2s->hdmarx);
    __HAL_RCC_DMA1_CLK_DISABLE();
  }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance != I2C1) {
    return;
  }

  GPIO_InitTypeDef GPIO_InitStruct;
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // I2C1 GPIO Configuration
  // * PB_8 -> I2C1_SCL
  // * PB_9 -> I2C1_SDA
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  __HAL_RCC_I2C1_CLK_ENABLE();
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();
    __HAL_RCC_I2C1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);
  }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
  (void)huart;

  GPIO_InitTypeDef GPIO_InitStruct;

  // Enable GPIO TX/RX clock
  USARTx_TX_GPIO_CLK_ENABLE();
  USARTx_RX_GPIO_CLK_ENABLE();

  // Enable USARTx clock
  USARTx_CLK_ENABLE();

  // UART TX GPIO pin configuration
  GPIO_InitStruct.Pin = USARTx_TX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = USARTx_TX_AF;
  HAL_GPIO_Init(USARTx_TX_GPIO_PORT, &GPIO_InitStruct);

  // UART RX GPIO pin configuration
  GPIO_InitStruct.Pin = USARTx_RX_PIN;
  GPIO_InitStruct.Alternate = USARTx_RX_AF;
  HAL_GPIO_Init(USARTx_RX_GPIO_PORT, &GPIO_InitStruct);

  // Enable DMA clock
  //__HAL_RCC_DMA1_CLK_ENABLE();

  //// Configure the DMA handler for transmission process
  // static DMA_HandleTypeDef hdma_tx;
  // hdma_tx.Instance = DMA1_Stream6;
  // hdma_tx.Init.Channel = DMA1_Channel7;
  // hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  // hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  // hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
  // hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  // hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  // hdma_tx.Init.Mode = DMA_NORMAL;
  // hdma_tx.Init.Priority = DMA_PRIORITY_LOW;
  // HAL_DMA_Init(&hdma_tx);

  //__HAL_LINKDMA(huart, hdmatx, hdma_tx);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
  (void)huart;

  USARTx_FORCE_RESET();
  USARTx_RELEASE_RESET();

  // Configure UART Tx as alternate function
  HAL_GPIO_DeInit(USARTx_TX_GPIO_PORT, USARTx_TX_PIN);
  // Configure UART Rx as alternate function
  HAL_GPIO_DeInit(USARTx_RX_GPIO_PORT, USARTx_RX_PIN);
}
