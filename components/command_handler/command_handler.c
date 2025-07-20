#include "command_handler.h"
#include "data_protocol.h"
#include "spi_slave_interface.h" 
#include "esp_log.h"
#include <string.h>

static const char *TAG = "CMD_HANDLER";


uint16_t command_handler_calculate_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void command_handler_process_packet(const uint8_t *data, size_t len) {
    if (len != sizeof(command_frame_t)) {
        ESP_LOGW(TAG, "Odebrano pakiet o złym rozmiarze dla komendy: %d B", len);
        return;
    }

    command_frame_t cmd;
    memcpy(&cmd, data, sizeof(command_frame_t));


    if (cmd.packet_type != PACKET_TYPE_COMMAND) {
        ESP_LOGW(TAG, "Odebrano pakiet, który nie jest komendą (typ: 0x%02X).", cmd.packet_type);
        return;
    }


    uint16_t calculated_crc = command_handler_calculate_crc16(data, sizeof(command_frame_t) - sizeof(uint16_t));
    if (calculated_crc != cmd.crc16) {
        ESP_LOGE(TAG, "Błąd CRC w komendzie! Odebrano: 0x%04X, Obliczono: 0x%04X", cmd.crc16, calculated_crc);
        return;
    }

    ESP_LOGI(TAG, "Odebrano poprawną komendę ID: %d, Argument: %lu", cmd.command_id, cmd.command_arg);


    switch (cmd.command_id) {
        case CMD_SET_MODE:
            ESP_LOGI(TAG, "Komenda: Ustaw tryb na %lu", cmd.command_arg);

            spi_slave_send_to_mcb((uint8_t*)&cmd, sizeof(command_frame_t));
            break;
        
        case CMD_PING:
            ESP_LOGI(TAG, "Komenda: Ping! Odpowiadam");
            
            break;

        default:
            ESP_LOGW(TAG, "Odebrano nieznaną komendę o ID: %d", cmd.command_id);
            break;
    }
}