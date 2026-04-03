#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define GPIO_TX_HFBR 5
#define GPIO_RX_HFBR 4
#define GPIO_TX_PLT 6
#define GPIO_RX_PLR 7


void app_main(void)
{
    int state = 0; //variable donnant l'état envoyé aux TX
    int level; //variable recevant le niveau lu sur les RX
    while(1){

        printf("\n\n");
        printf("\nSending 0 : \n");
        state=0; //d'abord on envoie un 0
        gpio_set_level(GPIO_TX_HFBR, state); 
        gpio_set_level(GPIO_TX_PLT, state);
        vTaskDelay(pdMS_TO_TICKS(1000)); //wait 1 seconde
    

        level = gpio_get_level(GPIO_RX_HFBR); //on lit le HFBR RX
        printf("Sending 0 on HFBR TX: %u\n", state);
        printf("Received by HFBR RX: %u\n\n", level);
        level = gpio_get_level(GPIO_RX_PLR); //on lit le TosLink RX
        printf("Sending 0 on TosLink TX: %u\n", state);
        printf("Received by TosLink RX: %u\n", level);
        vTaskDelay(pdMS_TO_TICKS(1000)); //wait 1 seconde

        printf("\nSending 1 : \n");
        state=1; //ensuite on envoie un 1
        gpio_set_level(GPIO_TX_HFBR, state); 
        gpio_set_level(GPIO_TX_PLT, state);
        vTaskDelay(pdMS_TO_TICKS(1000)); //wait 1 seconde


        level = gpio_get_level(GPIO_RX_HFBR);  //on lit le HFBR RX
        printf("Sending 1 on HFBR TX: %u\n", state);
        printf("Received by HFBR RX: %u\n\n", level);
        level = gpio_get_level(GPIO_RX_PLR); //on lit le TosLink RX
        printf("Sending 1 on TosLink TX: %u\n", state);
        printf("Received by TosLink RX: %u\n", level);
        vTaskDelay(pdMS_TO_TICKS(1000)); //wait 1 seconde
    
    }
    
}   