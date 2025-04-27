#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_i2s.h"
#include "stm32f4xx_hal_sai.h"

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

// Pinout definitions for SAI1
// - SCK -> PB12
// - FS -> PB9
// - SDA -> PA9
// - SDB -> PB2
#define SAIx_TX_BLOCK SAI1_Block_B
#define SAIx_RX_BLOCK SAI1_Block_A
#define SAIx_CLK_ENABLE() __HAL_RCC_SAI1_CLK_ENABLE()
#define SAIx_TX_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define SAIx_RX_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()

// B9 - FS
#define SAIx_FS_GPIO_PORT GPIOB
#define SAIx_FS_AF GPIO_AF6_SAI1
#define SAIx_FS_PIN GPIO_PIN_9

// B12 - SCK
#define SAIx_SCK_GPIO_PORT GPIOB
#define SAIx_SCK_AF GPIO_AF6_SAI1
#define SAIx_SCK_PIN GPIO_PIN_12

// A9 - TX-SD
#define SAIx_TX_SD_GPIO_PORT GPIOA
#define SAIx_TX_SD_AF GPIO_AF6_SAI1
#define SAIx_TX_SD_PIN GPIO_PIN_9

// B2 - RX-SD
#define SAIx_RX_SD_GPIO_PORT GPIOB
#define SAIx_RX_SD_AF GPIO_AF6_SAI1
#define SAIx_RX_SD_PIN GPIO_PIN_2

void HAL_MspInit(void) {
  // Ensure all priority bits are assigned as preemption priority bits.
  // Required by FreeRTOS.
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
  HAL_NVIC_SetPriority(SysTick_IRQn, 15U, 0U);

  // TODO: Set system interrupt priorities?
}

void HAL_MspDeInit(void) {}

void HAL_SAI_MspInit(SAI_HandleTypeDef *hsai) {
  GPIO_InitTypeDef GPIO_InitStruct;

  // PLL configuration for SAI clock.
  // PLLSAI1M - Division factor for PLL VCO input (must be between 2 and 63).
  // PLLSAI1N - Multiplication factor for PLLSAI1 VCO output (must be between
  //         50 and 432).
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
  PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLLSAI;
  // PeriphClkInitStruct.PLLSAI.PLLSAIM = 8;
  // PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  // PeriphClkInitStruct.PLLSAI.PLLSAIR = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIM = 8;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 429;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAIDivQ = 19;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

  __HAL_RCC_SAI1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  __HAL_RCC_DMA2_CLK_ENABLE();

  // FS -> PA3
  // SDB (RX) -> PA9
  // SDA (TX) -> PB2
  // SCK -> PB10

  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  if (hsai->Instance == SAIx_TX_BLOCK) {
    // SAI1_A_FS
    GPIO_InitStruct.Pin = SAIx_FS_PIN;
    GPIO_InitStruct.Alternate = SAIx_FS_AF;
    HAL_GPIO_Init(SAIx_FS_GPIO_PORT, &GPIO_InitStruct);

    // SAI1_A_SCK
    GPIO_InitStruct.Pin = SAIx_SCK_PIN;
    GPIO_InitStruct.Alternate = SAIx_SCK_AF;
    HAL_GPIO_Init(SAIx_SCK_GPIO_PORT, &GPIO_InitStruct);

    // SAI1_A_SD
    GPIO_InitStruct.Pin = SAIx_TX_SD_PIN;
    GPIO_InitStruct.Alternate = SAIx_TX_SD_AF;
    HAL_GPIO_Init(SAIx_TX_SD_GPIO_PORT, &GPIO_InitStruct);

    // Configure the DMA handler for transmission process
    static DMA_HandleTypeDef hdma_tx;
    hdma_tx.Instance = DMA2_Stream5;
    hdma_tx.Init.Channel = DMA_CHANNEL_0;
    hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_tx.Init.Mode = DMA_CIRCULAR;
    hdma_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_tx.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;

    __HAL_LINKDMA(hsai, hdmatx, hdma_tx);
    HAL_DMA_DeInit(&hdma_tx);
    HAL_DMA_Init(&hdma_tx);

    //HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 0x09, 0);
    //HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
  } else if (hsai->Instance == SAIx_RX_BLOCK) {
    // SAI1_B_FS
    GPIO_InitStruct.Pin = SAIx_RX_SD_PIN;
    GPIO_InitStruct.Alternate = SAIx_RX_SD_AF;
    HAL_GPIO_Init(SAIx_RX_SD_GPIO_PORT, &GPIO_InitStruct);

    // Configure the DMA handler for reception process
    static DMA_HandleTypeDef hdma_rx;
    hdma_rx.Instance = DMA2_Stream3;
    hdma_rx.Init.Channel = DMA_CHANNEL_0;
    hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_rx.Init.Mode = DMA_CIRCULAR;
    hdma_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_rx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_rx.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_rx.Init.PeriphBurst = DMA_PBURST_SINGLE;

    __HAL_LINKDMA(hsai, hdmarx, hdma_rx);
    HAL_DMA_DeInit(&hdma_rx);
    HAL_DMA_Init(&hdma_rx);
  }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance != I2C1) {
    return;
  }

  GPIO_InitTypeDef GPIO_InitStruct;
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  // I2C1 GPIO Configuration
  // * PB_6 -> I2C1_SCL
  // * PB_7 -> I2C1_SDA
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  __HAL_RCC_I2C1_CLK_ENABLE();
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();
    __HAL_RCC_I2C1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);
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
