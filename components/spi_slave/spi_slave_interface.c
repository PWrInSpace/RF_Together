#include "driver/spi_slave.h"
#include "spi_slave_interface.h"
#include "esp_attr.h"
#include "esp_log.h"
#include <string.h>

#define HOST_SPI SPI2_HOST  // VSPI to SPI3_HOST, SPI2_HOST to HSPI, zazwyczaj SPI2_HOST do slave
#define PIN_NUM_MISO 19     // Ustaw poprawne piny zgodnie z ESP32 WROOM-32
#define PIN_NUM_MOSI 23
#define PIN_NUM_SCLK 18
#define PIN_NUM_CS   5

static const char *TAG = "SPI_SLAVE";

WORD_ALIGNED_ATTR static uint8_t recvbuf[128];
WORD_ALIGNED_ATTR static uint8_t sendbuf[128];

static spi_data_received_cb_t data_received_callback = NULL;

static void spi_task(void *pvParameters) {
    while(1) {
        spi_slave_transaction_t t;
        memset(&t, 0, sizeof(t));
        
        t.length = sizeof(recvbuf) * 8; 
        t.tx_buffer = sendbuf;
        t.rx_buffer = recvbuf;

        esp_err_t ret = spi_slave_transmit(HOST_SPI, &t, portMAX_DELAY);
        if (ret == ESP_OK) {
            if (t.trans_len > 0 && data_received_callback != NULL) {
                data_received_callback(recvbuf, t.trans_len / 8);
            }
        } else {
            ESP_LOGE(TAG, "spi_slave_transmit failed: %s", esp_err_to_name(ret));
        }
    }
}

esp_err_t spi_slave_init_(spi_data_received_cb_t callback) {
    data_received_callback = callback;

    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 128,
    };

    spi_slave_interface_config_t slvcfg = {
        .spics_io_num = PIN_NUM_CS,
        .flags = 0,
        .queue_size = 5,
        .mode = 0,
        .post_setup_cb = NULL,
        .post_trans_cb = NULL,
    };

    esp_err_t res = spi_slave_initialize(HOST_SPI, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "SPI Slave init failed: %s", esp_err_to_name(res));
        return res;
    }

    xTaskCreate(spi_task, "spi_task", 4096, NULL, 10, NULL);

    ESP_LOGI(TAG, "SPI Slave zainicjalizowany");

    return ESP_OK;
}

esp_err_t spi_slave_send_to_mcb(uint8_t *data, size_t len) {
    if (len > sizeof(sendbuf)) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(sendbuf, data, len);
    ESP_LOGI(TAG, "Przygotowano %d bajtów do wysłania do MCB.", len);
    return ESP_OK;
}
