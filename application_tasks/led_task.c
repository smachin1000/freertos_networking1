#include "../drivers/mss_gpio/mss_gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <math.h>

#include "../main.h"

/**
 * This task reads analog input values from the queue, then updates the LEDs
 * to display the value like a bar graph.
 */
void led_initialization()
{
    /* Configuration of GPIOs */
    MSS_GPIO_config(MSS_GPIO_0, MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config(MSS_GPIO_1, MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config(MSS_GPIO_2, MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config(MSS_GPIO_3, MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config(MSS_GPIO_4, MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config(MSS_GPIO_5, MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config(MSS_GPIO_6 , MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config(MSS_GPIO_7, MSS_GPIO_OUTPUT_MODE );
}

void led_task(void* args)
{
    while (1) {
    }
}
