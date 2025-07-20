#ifndef ESP_NOW_COMMS_H
#define ESP_NOW_COMMS_H

#include "esp_err.h"


typedef void (*esp_now_receive_cb_t)(const uint8_t *mac_addr, const uint8_t *data, int len);

esp_err_t esp_now_comms_init(esp_now_receive_cb_t callback);
esp_err_t esp_now_comms_send(const uint8_t *data, size_t len);

#endif 