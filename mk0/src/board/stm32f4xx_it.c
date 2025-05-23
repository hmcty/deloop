#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_uart.h>

#include "stm32f446xx.h"
#include "stm32f4xx_hal_sai.h"
#include "stm32f4xx_it.h"

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef uart2_handle;

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void) {}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void) {
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1) {
  }
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void) {
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1) {
  }
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void) {
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1) {
  }
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void) {
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1) {
  }
}

/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
// void SVC_Handler(void) {}

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void) {}

/**
 * @brief  This function handles PendSVC exception.
 * @param  None
 * @retval None
 */
// void PendSV_Handler(void) {}

/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */
// void SysTick_Handler(void) { HAL_IncTick(); }

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
 * @brief  This function handles USART2 interrupt request.
 * @param  None
 * @retval None
 */
void USART2_IRQHandler(void) { HAL_UART_IRQHandler(&uart2_handle); }

/**
 * @brief  This function handles DMA2 stream 5 interrupt request.
 * @param  None
 * @retval None
 */
void DMA2_Stream5_IRQHandler(void) { Audio_DMA_Tx_IRQHandler(); }

/**
 * @brief  This function handles DMA2 stream 6 interrupt request.
 * @param  None
 * @retval None
 */
void DMA2_Stream3_IRQHandler(void) { Audio_DMA_Rx_IRQHandler(); }
