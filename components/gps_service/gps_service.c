#include "gps_service.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>

//zmienic piny
#define I2C_MASTER_PORT     I2C_NUM_0
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_SCL_IO   22
#define I2C_MASTER_FREQ_HZ  400000 

// Domyślny adres I2C 
#define UBLOX_GPS_ADDR      0x42

#define UBLOX_REG_BYTES_AVAIL_H 0xFD 
#define UBLOX_REG_BYTES_AVAIL_L 0xFE 
#define UBLOX_REG_DATA_STREAM   0xFF 

static const char *TAG = "GPS_SERVICE_M10S";

static gps_data_t g_gps_data;
static SemaphoreHandle_t g_gps_data_mutex;


static void parse_gga_sentence(const char *sentence) {
    float lat, lon, alt;
    int fix, sats;
    char ns, ew;

    if (strncmp(sentence + 3, "GGA", 3) != 0) {
        return;
    }

    int parsed_items = sscanf(sentence, "$%*5s,%*f,%f,%c,%f,%c,%d,%d,%*f,%f", &lat, &ns, &lon, &ew, &fix, &sats, &alt);

    if (parsed_items >= 7) { 
        if (xSemaphoreTake(g_gps_data_mutex, portMAX_DELAY) == pdTRUE) {
            g_gps_data.is_fixed = (fix > 0);
            g_gps_data.satellites_in_view = sats;

            if (g_gps_data.is_fixed) {
                
                g_gps_data.latitude = (int)(lat / 100) + (fmod(lat, 100.0) / 60.0);
                if (ns == 'S') g_gps_data.latitude = -g_gps_data.latitude;

                g_gps_data.longitude = (int)(lon / 100) + (fmod(lon, 100.0) / 60.0);
                if (ew == 'W') g_gps_data.longitude = -g_gps_data.longitude;
                
                g_gps_data.altitude = alt;
            }
            xSemaphoreGive(g_gps_data_mutex);
        }
    }
}

static void gps_task(void *pvParameters) {
    uint8_t data_buffer[256];
    char sentence_buffer[100];
    int sentence_pos = 0;

    while (1) {
        uint8_t bytes_avail_buf[2];
        uint16_t bytes_to_read = 0;

        uint8_t reg_addr = UBLOX_REG_BYTES_AVAIL_H;
        esp_err_t ret = i2c_master_write_read_device(
            I2C_MASTER_PORT,
            UBLOX_GPS_ADDR,
            &reg_addr, 1,
            bytes_avail_buf, 2,
            pdMS_TO_TICKS(1000)
        );
        
        
        if (ret == ESP_OK) {
            bytes_to_read = ((uint16_t)bytes_avail_buf[0] << 8) | bytes_avail_buf[1];
        } else {
             ESP_LOGE(TAG, "Błąd odczytu liczby dostępnych bajtów: %s", esp_err_to_name(ret));
             vTaskDelay(pdMS_TO_TICKS(1000));
             continue;
        }

        if (bytes_to_read > 0 && bytes_to_read < sizeof(data_buffer)) {
            uint8_t reg = UBLOX_REG_DATA_STREAM;
            ret = i2c_master_write_read_device(
                I2C_MASTER_PORT,
                UBLOX_GPS_ADDR,
                &reg, 1,
                data_buffer, bytes_to_read,
                pdMS_TO_TICKS(1000)
            );


            if (ret == ESP_OK) {
               
                for (int i = 0; i < bytes_to_read; i++) {
                    char c = data_buffer[i];
                    if (c == '$') {
                        sentence_pos = 0;
                        sentence_buffer[sentence_pos++] = c;
                    } else if (sentence_pos > 0) {
                        if (c == '\n' && sentence_buffer[sentence_pos-1] == '\r') {
                            sentence_buffer[sentence_pos-1] = '\0'; 
                            parse_gga_sentence(sentence_buffer);
                            sentence_pos = 0;
                        } else if (sentence_pos < sizeof(sentence_buffer) - 1) {
                            sentence_buffer[sentence_pos++] = c;
                        } else {
                            sentence_pos = 0;
                        }
                    }
                }
            }
        }
    
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

esp_err_t gps_service_start() {
    g_gps_data_mutex = xSemaphoreCreateMutex();
    if(g_gps_data_mutex == NULL) return ESP_FAIL;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,  // lub DISABLE, zależnie od hardware
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Błąd konfiguracji I2C: %s", esp_err_to_name(err));
        return err;
    }
    err = i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Błąd instalacji sterownika I2C: %s", esp_err_to_name(err));
        return err;
    }   
    
    ESP_LOGI(TAG, "Sterownik I2C dla u-blox MAX-M10S zainicjalizowany.");
    
    xTaskCreate(gps_task, "gps_task", 4096, NULL, 5, NULL);
    return ESP_OK;
}

BaseType_t gps_service_get_data(gps_data_t *out_data) {
    if (g_gps_data_mutex != NULL && xSemaphoreTake(g_gps_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(out_data, &g_gps_data, sizeof(gps_data_t));
        xSemaphoreGive(g_gps_data_mutex);
        return pdTRUE;
    }
    return pdFALSE;
}