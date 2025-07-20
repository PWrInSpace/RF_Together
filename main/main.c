#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "spi_slave_interface.h"
#include "gps_service.h"
#include "lora_task_config.h"
#include "esp_now_comms.h"
#include "data_protocol.h"
#include "command_handler.h"
#include "command_config.h"

static const char *TAG = "MAIN_APP";

static mcb_telemetry_t g_mcb_data;
static SemaphoreHandle_t g_mcb_data_mutex;


void on_mcb_data_received(uint8_t *data, size_t len) {
    if (len != sizeof(mcb_telemetry_t)) {
        ESP_LOGW(TAG, "Otrzymano od MCB pakiet o złym rozmiarze: %d B", len);
        return;
    }
    
    if (xSemaphoreTake(g_mcb_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        memcpy(&g_mcb_data, data, sizeof(mcb_telemetry_t));
        xSemaphoreGive(g_mcb_data_mutex);
        ESP_LOGI(TAG, "Odebrano nową ramkę telemetryczną z MCB.");
    }
}

void on_ground_station_data_received(const uint8_t *mac_addr, const uint8_t *data, int len) {
    ESP_LOGI(TAG, "Odebrano %d bajtów od stacji naziemnej.", len);

    command_handler_process_packet(data, len);
}



//pakowanie i wysylka
void radio_tx_task(void *pvParameters) {
    telemetry_frame_t frame_to_send;
    gps_data_t current_gps_data;
    mcb_telemetry_t current_mcb_data;


    memset(&frame_to_send, 0, sizeof(telemetry_frame_t));
    memset(&current_gps_data, 0, sizeof(gps_data_t));
    memset(&current_mcb_data, 0, sizeof(mcb_telemetry_t));

    while (1) {

        vTaskDelay(pdMS_TO_TICKS(1000));

  
        bool gps_ok = (gps_service_get_data(&current_gps_data) == pdTRUE);

        bool mcb_ok = false;
        if (xSemaphoreTake(g_mcb_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            current_mcb_data = g_mcb_data;
            xSemaphoreGive(g_mcb_data_mutex);
            mcb_ok = true;
        }


        frame_to_send.packet_type = PACKET_TYPE_TELEMETRY;
        frame_to_send.timestamp_ms = esp_log_timestamp();

        if (gps_ok) {
            frame_to_send.gps_fix = current_gps_data.is_fixed;
            frame_to_send.latitude = current_gps_data.latitude;
            frame_to_send.longitude = current_gps_data.longitude;
            frame_to_send.altitude = current_gps_data.altitude;
            frame_to_send.satellites = current_gps_data.satellites_in_view;
        }

        if (mcb_ok) {
            frame_to_send.mcb_data = current_mcb_data;
        }


        size_t crc_payload_len = sizeof(telemetry_frame_t) - sizeof(uint16_t);
        frame_to_send.crc16 = command_handler_calculate_crc16((uint8_t*)&frame_to_send, crc_payload_len);


        esp_err_t result = esp_now_comms_send((uint8_t*)&frame_to_send, sizeof(telemetry_frame_t));
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "Ramka telemetryczna wysłana pomyślnie");
        } else {
            ESP_LOGE(TAG, "Błąd podczas wysyłania ramki: %s", esp_err_to_name(result));
        }
    }
}

void app_main(void) {

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    g_mcb_data_mutex = xSemaphoreCreateMutex();


    ESP_ERROR_CHECK(spi_slave_init(on_mcb_data_received));
    ESP_ERROR_CHECK(gps_service_start());
    ESP_ERROR_CHECK(esp_now_comms_init(on_ground_station_data_received));
    
    ESP_LOGI(TAG, "Wszystkie komponenty zainicjalizowane");


    xTaskCreate(radio_tx_task, "radio_tx_task", 4096, NULL, 5, NULL);
}