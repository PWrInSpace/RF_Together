#include "esp_now_comms.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ESP_NOW_COMMS";

// ustawic adres mac
static uint8_t ground_station_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static esp_now_receive_cb_t data_receive_callback = NULL;


static void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        ESP_LOGW(TAG, "Wysyłanie danych nie powiodło się.");
    }
}


static void recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (data_receive_callback != NULL) {
        data_receive_callback(mac_addr, data, len);
    }
}

esp_err_t esp_now_comms_init(esp_now_receive_cb_t callback) {
    data_receive_callback = callback;


    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());


    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));


    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, ground_station_mac, ESP_NOW_ETH_ALEN);
    peer_info.channel = 0; 
    peer_info.encrypt = false; 
    
    if (esp_now_add_peer(&peer_info) != ESP_OK){
        ESP_LOGE(TAG, "Nie udało się dodać peera (stacji naziemnej).");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "ESP-NOW zainicjalizowany pomyślnie.");
    return ESP_OK;
}

esp_err_t esp_now_comms_send(const uint8_t *data, size_t len) {
    return esp_now_send(ground_station_mac, data, len);
}