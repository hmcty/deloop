#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "stm32f4xx_hal.h"

void vApplicationMallocFailedHook(void) {
  // vApplicationMallocFailedHook() will only be called if
  // configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
  // function that will get called if a call to pvPortMalloc() fails.
  // pvPortMalloc() is called internally by the kernel whenever a task, queue,
  // timer or semaphore is created.  It is also called by various parts of the
  // demo application.  If heap_1.c, heap_2.c or heap_4.c is being used, then
  // the size of the    heap available to pvPortMalloc() is defined by
  // configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
  // API function can be used to query the size of free heap space that remains
  // (although it does not provide information on how the remaining heap might
  // be fragmented).  See http://www.freertos.org/a00111.html for more
  // information.
  // vAssertCalled(__FILE__, __LINE__);
}

void vApplicationIdleHook(void) {
  // vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
  // to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
  // task.  It is essential that code added to this hook function never attempts
  // to block in any way (for example, call xQueueReceive() with a block time
  // specified, or call vTaskDelay()).  If application tasks make use of the
  // vTaskDelete() API function to delete themselves then it is also important
  // that vApplicationIdleHook() is permitted to return to its calling function,
  // because it is the responsibility of the idle task to clean up memory
  // allocated by the kernel to any task that has since deleted itself.
  // usleep(15000);
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) {
  (void)pcTaskName;
  (void)pxTask;

  // vAssertCalled(__FILE__, __LINE__);
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
  (void)pcFormat;
  // va_list arg;
  // va_start(arg, pcFormat);
  // vprintf(pcFormat, arg);
  // va_end(arg);
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
