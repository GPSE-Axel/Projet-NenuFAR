#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define GPIO_TX_HFBR 5
#define GPIO_RX_HFBR 4
#define GPIO_TX_PLT 6
#define GPIO_RX_PLR 7
#define MAX_FRAME_BYTES   256
#define MAX_SYMBOLS       (MAX_FRAME_BYTES * 8)
#define RMT_HZ 40000000
#define BITRATE 40000
#define TICKS_PER_BIT (RMT_HZ / BITRATE)
#define PREAMBLE_BYTES 4
#define MAX_FRAME_BYTES 256



rmt_symbol_word_t symbols1[MAX_SYMBOLS];
rmt_symbol_word_t symbols2[MAX_SYMBOLS];
static uint8_t tx_buffer[MAX_FRAME_BYTES];
static uint8_t rx_frame[32];
static size_t rx_len = 0;
static uint8_t nb_octets = 0;

// Construit une trame dans frame_buf à partir d'un payload
// Retourne la longueur totale de la trame
size_t build_frame(uint8_t *tx_buffer,
                   const uint8_t *payload,
                   size_t payload_len)
{
    uint8_t *p = tx_buffer;

    // Préambule 0xAA 0xAA 0xAA 0xAA
    for (int i = 0; i < PREAMBLE_BYTES; i++) {
        *p++ = 0xAA;
    }
    
    // SFD 0xD5
    *p++ = 0xD5;

    // Longueur (1 octet)
    *p++ = (uint8_t)payload_len;
    printf("payload length: %u",(uint8_t)payload_len);

    // Payload
    for(int i = 0; i < payload_len; i++){
        *p++ = payload[i];
    }

    return (size_t)(p - tx_buffer);
}


// Encode un buffer d'octets en symboles NRZ pour RMT
size_t bytes_to_rmt_symbols(const uint8_t *data,
                            size_t len,
                            rmt_symbol_word_t *symbols,
                            size_t max_symbols)
{
    size_t sym_idx = 0;
    int taille=0;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (int b = 7; b >= 0; b-=2) {
            if (sym_idx >= max_symbols) {
                return sym_idx; // protection
            }

            //on récupère les bits de chaque octet
            bool bit1 = (byte >> b) & 0x01;
            bool bit2 = (byte >> (b - 1)) & 0x01;
            rmt_symbol_word_t *s = &symbols[sym_idx++];
            
            //on encode les bits 2 à 2 dans un symbole RMT
            s->level0 = bit1; 
            s->duration0 = TICKS_PER_BIT; 
            s->level1 = bit2; 
            s->duration1 = TICKS_PER_BIT;
            taille++;
        }
    }

    printf("Nb symb: %u\n", taille);
    return sym_idx;
}

// Fonction de callback utilisée à la réception d'un signal RMT
static bool rmt_rx_callback(rmt_channel_handle_t channel,
                            const rmt_rx_done_event_data_t *edata,
                            void *arg)
{
    const rmt_symbol_word_t *sym = edata->received_symbols;
    size_t count = edata->num_symbols;

    static int bit_pos = 7;
    static uint8_t current_byte = 0;

    for (size_t i = 1; i < count; i++) {
        uint8_t nb_bits = sym[i].duration0/TICKS_PER_BIT;

        //on retransforme les symboles RMT en bits
        for(uint8_t j = 0; j < nb_bits; j++){
            current_byte |= ((uint8_t)sym[i].level0 << bit_pos);
            bit_pos--;

            //on ajoute les bits les uns à la suite des autres et à chaque octet complet, on ajoute l'octet dans un tableau
            if (bit_pos < 0) {
                rx_frame[rx_len++] = current_byte;
                current_byte = 0;
                bit_pos = 7;
                nb_octets++;
            }
        }

        nb_bits = round(sym[i].duration1/TICKS_PER_BIT);

        //on retransforme les symboles RMT en bits
        for(uint8_t j = 0; j < nb_bits; j++){
            current_byte |= ((uint8_t)sym[i].level1 << bit_pos);
            bit_pos--;

            //on ajoute les bits les uns à la suite des autres et à chaque octet complet, on ajoute l'octet dans un tableau
            if (bit_pos < 0) {
                rx_frame[rx_len++] = current_byte;
                current_byte = 0;
                bit_pos = 7;
                nb_octets++;
            }
        }
    }

    // Flush final : si des bits restent, on pousse l’octet 
    if (bit_pos != 7) { 
        rx_frame[rx_len++] = current_byte; 
        current_byte = 0; 
        bit_pos = 7; 
        nb_octets++; 
    }

    return false; 
}

void app_main(void)
{
    
    // Configuration canal TX 
    rmt_channel_handle_t tx_chan = NULL;
    rmt_tx_channel_config_t tx_cfg = {
        .gpio_num = GPIO_TX_PLT,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_HZ,
        .mem_block_symbols = 92,
        .trans_queue_depth = 1,
    };
    
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_cfg, &tx_chan));
    ESP_ERROR_CHECK(rmt_enable(tx_chan));

    // Copy encoder 
    rmt_encoder_handle_t copy_encoder = NULL;
    rmt_copy_encoder_config_t copy_cfg = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_cfg, &copy_encoder));

    // Payload 
    uint8_t payload[] = {   
                            0xA3, 0x5F, 0x09, 0xC1, 0x7B, 0x44, 0x8E, 0x12, 0xD9, 0x6A,
                            0x3C, 0xF0, 0x27, 0x81, 0xB4, 0x5A, 0x0D, 0x99, 0xE7, 0x42,
                            0x1F, 0xC8, 0x73, 0xAD, 0x06, 0x58, 0x9E, 0x24, 0xD3
                        };

    size_t payload_len = sizeof(payload);
    

    // Buffer trame
    size_t frame_len = build_frame(tx_buffer, payload, payload_len);


    // Conversion en symboles RMT
    size_t nb_symbols = bytes_to_rmt_symbols(tx_buffer, frame_len, symbols1, MAX_SYMBOLS);

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,            //pas de bouclage
    };

    // Configuration canal RX 
    rmt_channel_handle_t rx_chan = NULL;
    rmt_rx_channel_config_t rx_cfg = {
        .gpio_num = GPIO_RX_PLR,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_HZ,
        .mem_block_symbols = 64,
    };
    
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_cfg, &rx_chan));
    
    // Configuration callback
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rmt_rx_callback,
    };

    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_chan, &cbs, NULL));
    
    ESP_ERROR_CHECK(rmt_enable(rx_chan));
    
    // Buffer de réception 
    static rmt_symbol_word_t rx_buffer[100];

    rmt_receive_config_t rx_config = {
        .signal_range_min_ns = 75,     
        .signal_range_max_ns = 100000,
    };
    
    // Lancer la réception 
    ESP_ERROR_CHECK(rmt_receive(rx_chan, rx_buffer, sizeof(rx_buffer), &rx_config));

    printf("RMT RX démarré...\n");

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // Lancer la transmission
    ESP_ERROR_CHECK(rmt_transmit(tx_chan, copy_encoder,
                             symbols1, nb_symbols * sizeof(rmt_symbol_word_t),
                             &tx_config));
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Afichage de la réception
    while (1) { 
        vTaskDelay(100 / portTICK_PERIOD_MS); 
        if (rx_len > 0) { 
            printf("Reçu %u octets :\n", nb_octets); 
            for (int i = 0; i < rx_len; i++) { 
                printf("0x%02X, ", rx_frame[i]); 
            } 
            printf("\n"); rx_len = 0; 
        } 
    }
    
}   