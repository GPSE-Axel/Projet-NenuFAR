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
    //configuration des GPIOs
    gpio_config_t io_out_tx_HFBR = {
        .pin_bit_mask = (1ULL << GPIO_TX_HFBR),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_out_tx_HFBR);

    gpio_config_t io_out_tx_TosLink= {
        .pin_bit_mask = (1ULL << GPIO_TX_PLT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    gpio_config(&io_out_tx_TosLink);

     gpio_config_t io_out_rx_HFBR = {
        .pin_bit_mask = (1ULL << GPIO_RX_HFBR),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    gpio_config(&io_out_tx_HFBR);

    gpio_config_t io_out_rx_TosLink= {
        .pin_bit_mask = (1ULL << GPIO_RX_PLR),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    gpio_config(&io_out_rx_TosLink);
    
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