#ifndef GPS_SERVICE_H
#define GPS_SERVICE_H

#include "esp_err.h"
#include <stdbool.h>

typedef struct {
    float latitude;
    float longitude;
    float altitude;
    uint8_t satellites_in_view;
    bool is_fixed;
} gps_data_t;

esp_err_t gps_service_start();
esp_err_t gps_service_get_data(gps_data_t *gps_data);

#endif 