#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2s.h"
#include "stm32f4xx_hal_dma.h"

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

#define USARTx                           USART2
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART2_CLK_ENABLE();
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __HAL_RCC_USART2_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART2_RELEASE_RESET()

#define USARTx_TX_PIN                    GPIO_PIN_2
#define USARTx_TX_GPIO_PORT              GPIOA
#define USARTx_TX_AF                     GPIO_AF7_USART2
#define USARTx_RX_PIN                    GPIO_PIN_3
#define USARTx_RX_GPIO_PORT              GPIOA
#define USARTx_RX_AF                     GPIO_AF7_USART2

void HAL_MspInit(void) {
  // Ensure all priority bits are assigned as preemption priority bits.
  // Required by FreeRTOS.
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  // TODO: Set system interrupt priorities?
}

void HAL_MspDeInit(void) {}

void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s) {
  GPIO_InitTypeDef GPIO_InitStruct;
  if (hi2s->Instance == SPI2) {
    __HAL_RCC_SPI2_CLK_ENABLE();

    // I2S2 GPIO Configuration
    // * PC_1  -> I2S2_SDO
    // * PC_2  -> I2S2_SDI
    // * PB_12 -> I2S2_WS
    // * PB_10 -> I2S2_MCK
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

void HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s) {
  if (hi2s->Instance == SPI2) {
    __HAL_RCC_SPI2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_3 | GPIO_PIN_6);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12 | GPIO_PIN_13);
  }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
  GPIO_InitTypeDef GPIO_InitStruct;
  if (hi2c->Instance == I2C1) {
    __HAL_RCC_I2C1_CLK_ENABLE();

    // I2C1 GPIO Configuration
    // * PB_8 -> I2C1_SCL
    // * PB_9 -> I2C1_SDA
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    __HAL_RCC_I2C1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8 | GPIO_PIN_9);
  }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
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
//static DMA_HandleTypeDef hdma_tx;
//hdma_tx.Instance = DMA1_Stream6;
//hdma_tx.Init.Channel = DMA1_Channel7;
//hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
//hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
//hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
//hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
//hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
//hdma_tx.Init.Mode = DMA_NORMAL;
//hdma_tx.Init.Priority = DMA_PRIORITY_LOW;
//HAL_DMA_Init(&hdma_tx);

//__HAL_LINKDMA(huart, hdmatx, hdma_tx);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
  USARTx_FORCE_RESET();
  USARTx_RELEASE_RESET();

  // Configure UART Tx as alternate function
  HAL_GPIO_DeInit(USARTx_TX_GPIO_PORT, USARTx_TX_PIN);
  // Configure UART Rx as alternate function
  HAL_GPIO_DeInit(USARTx_RX_GPIO_PORT, USARTx_RX_PIN);
}
