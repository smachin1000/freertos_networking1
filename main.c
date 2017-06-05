#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "a2fxxxm3.h"
#include "./drivers/mss_gpio/mss_gpio.h"
#include "./drivers/mss_watchdog/mss_watchdog.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"

#define SYS_TICK_CTRL_AND_STATUS_REG      0xE000E010
#define SYS_TICK_CONFIG_REG               0xE0042038
#define SYS_TICK_FCLK_DIV_32_NO_REF_CLK   0x31000000
#define ENABLE_SYS_TICK                   0x7

extern void led_task(void *para);
extern void led_initialization(void);

static void init_system()
{
    /* Disable the Watch Dog Timer */
    MSS_WD_disable( );
    /* Initialize the GPIO */
    MSS_GPIO_init();
    /* GPIO inits for the LEDs */
    led_initialization();
} 

int main()
{
    int c;
    /* Initialization all necessary hardware components */
    init_system();

    // Note that we create two tasks with the same priority.
    // FreeRTOS manages time slicing between equal priority tasks.
    c = xTaskCreate( led_task,                          // task "run" function
                     ( signed portCHAR * ) "led_task",  // task name
                     configMINIMAL_STACK_SIZE,          // task stack size in 32 bit words (not bytes)
                     NULL,                              // param to pass to run function
                     tskIDLE_PRIORITY + 1,              // task priority
                     NULL );                            // task handle

    if (c != pdPASS) {
        printf("task create led_task failed with code %d, exiting\r\n", c);
        return EXIT_FAILURE;
    }

    /* Enable the SYS TICK Timer and provide the divider and clock source
     * this is required to enable the RTOS tick */
    *(volatile unsigned long *)SYS_TICK_CTRL_AND_STATUS_REG = ENABLE_SYS_TICK;
    *(volatile unsigned long *)SYS_TICK_CONFIG_REG          = SYS_TICK_FCLK_DIV_32_NO_REF_CLK;

    /* Start the scheduler. */
    vTaskStartScheduler();

    /* Will only get here if there was not enough heap space to create the
    idle task. */
    printf("\r\nScheduler has quit, should never come here \n\r");
    return EXIT_FAILURE;
}
