#ifndef SPI_SLAVE_INTERFACE_H
#define SPI_SLAVE_INTERFACE_H

#include "esp_err.h"


typedef void (*spi_data_received_cb_t)(uint8_t *data, size_t len);

esp_err_t spi_slave_init(spi_data_received_cb_t callback);
esp_err_t spi_slave_send_to_mcb(uint8_t *data, size_t len);
