#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "stm32f4xx_hal.h"

// Forward declaration
void vAssertCalled(const char *const pcFileName, unsigned long ulLine);

void vApplicationMallocFailedHook(void) {
  // Called if a call to pvPortMalloc() fails
  vAssertCalled(__FILE__, __LINE__);
}

void vApplicationIdleHook(void) {
  // This hook is called on each iteration of the idle task
  // Implementation is intentionally empty
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) {
  (void)pcTaskName;
  (void)pxTask;

  vAssertCalled(__FILE__, __LINE__);
}

void vApplicationTickHook(void) {
  // This function will be called by each tick interrupt if
  // configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
  // added here, but the tick hook is called from an interrupt context, so
  // code must not attempt to block, and only the interrupt safe FreeRTOS API
  // functions can be used (those that end in FromISR()).
  HAL_IncTick();
}

void vLoggingPrintf(const char *pcFormat, ...) {
  // Intentionally empty - logging is handled elsewhere
  (void)pcFormat;
}

void vAssertCalled(const char *const pcFileName, unsigned long ulLine) {
  static BaseType_t xPrinted = pdFALSE;
  volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

  /* Called if an assertion passed to configASSERT() fails.  See
   * https://www.FreeRTOS.org/a00110.html#configASSERT for more information. */

  /* Parameters are not used. */
  (void)ulLine;
  (void)pcFileName;

  taskENTER_CRITICAL();
  {
    /* Stop the trace recording. */
    if (xPrinted == pdFALSE) {
      xPrinted = pdTRUE;
    }

    /* You can step out of this function to debug the assertion by using
     * the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
     * value. */
    while (ulSetToNonZeroInDebuggerToContinue == 0) {
      __asm volatile("NOP");
      __asm volatile("NOP");
    }
  }
  taskEXIT_CRITICAL();
}
