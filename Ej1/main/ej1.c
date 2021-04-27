#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"

 gpio_config_t io_conf;
// This task will execute forever and blink LED
// Note that internal high priority tasks still
// execute - such as WiFi stack routines
//toque comentario

void gpioSetup(){
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = GPIO_Pin_2;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    //configure GPIO with the given settings
    gpio_config(&io_conf);
      printf("Configuró gpio\n");
}

void LEDBlinkTask (void *pvParameters)
{
    int cnt = 0;

    while(1)
    {
      vTaskDelay(500 / portTICK_RATE_MS);
      gpio_set_level(2, cnt % 0);
      gpio_set_level(2, cnt % 1);
    }
}

// User function
// All user code goes here.
// Note that the user function should exit and not block execution
void app_main(void)
{
    // This task blinks the LED continuously
    xTaskCreate(LEDBlinkTask, "Blink", 256, NULL, 2, NULL);
}
