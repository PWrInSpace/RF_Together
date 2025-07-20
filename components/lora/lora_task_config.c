#include "lora_task_config.h"

#include "commands_config.h"
#include "data_to_protobuf.h"
#include "errors_config.h"
#include "esp_log.h"
#include "lora.pb-c.h"
#include "lora_hw_config.h"
#include "sdkconfig.h"
#include "system_timer_config.h"
#include "utils.h"

#define TAG "LORA_C"

static bool settings_frame = false;

void lora_send_settings_frame(void) { settings_frame = true; }

static bool check_prefix(uint8_t* packet, size_t packet_size) {
    if (packet_size < sizeof(PACKET_PREFIX)) {
        return false;
    }

    uint8_t prefix[] = PACKET_PREFIX;
    for (int i = 0; i < sizeof(PACKET_PREFIX) - 1; ++i) {
        if (packet[i] != prefix[i]) {
            return false;
        }
    }

    return true;
}

static uint8_t calculate_checksum(uint8_t* buffer, size_t size) {
    uint8_t sum = 0;
    for (size_t i = 0; i < size; ++i) {
        sum += buffer[i];
    }

    return sum;
}

static void lora_process(uint8_t* packet, size_t packet_size) {
    if (packet_size > 40) {
        ESP_LOGI(TAG, "Recevied packet is too big");
        errors_set(ERROR_TYPE_LAST_EXCEPTION, ERROR_EXCP_LORA_DECODE, 100);
        return;
    }

    if (check_prefix(packet, packet_size) == false) {
        ESP_LOGE(TAG, "LoRa invalid prefix");
        return;
    }


    uint8_t prefix_size = sizeof(PACKET_PREFIX) - 1;
    if (calculate_checksum(packet + prefix_size, packet_size - prefix_size - 1) != packet[packet_size - 1]) {
        ESP_LOGE(TAG, "Invalid checksum");
        return;
    }

    LoRaCommand* received =
        lo_ra_command__unpack(NULL, packet_size - prefix_size - 1, packet + prefix_size);
    if (received != NULL) {
        ESP_LOGI(TAG, "Received LORA_ID %d, DEV_ID %d, COMMAND %d, PLD %d", received->lora_dev_id,
                 received->sys_dev_id, received->command, received->payload);

        cmd_message_t received_command = cmd_create_message(received->command, received->payload);
        lo_ra_command__free_unpacked(received, NULL);
        if (lora_cmd_process_command(received->lora_dev_id, received->sys_dev_id,
                                     &received_command) == false) {
            errors_add(ERROR_TYPE_LAST_EXCEPTION, ERROR_EXCP_COMMAND_NOT_FOUND, 200);
            ESP_LOGE(TAG, "Unable to prcess command :C");
            return;
        }

        if (sys_timer_restart(TIMER_DISCONNECT, DISCONNECT_TIMER_PERIOD_MS) == false) {
            ESP_LOGE(TAG, "Unable to restart timer");
        }
    } else {
        errors_add(ERROR_TYPE_LAST_EXCEPTION, ERROR_EXCP_LORA_DECODE, 200);
        ESP_LOGE(TAG, "Unable to decode received package");
    }
}

static size_t add_prefix(uint8_t* buffer, size_t size) {
    if (size < 6) {
        return 0;
    }

    memcpy(buffer, PACKET_PREFIX, sizeof(PACKET_PREFIX) - 1);

    return sizeof(PACKET_PREFIX) - 1;
}

static size_t lora_create_settings_packet(uint8_t* buffer, size_t size) {
    LoRaSettings frame = LO_RA_SETTINGS__INIT;
    create_protobuf_settings_frame(&frame);

    uint8_t data_size = 0;
    uint8_t prefix_size = 0;
    prefix_size = add_prefix(buffer, size);
    data_size = lo_ra_settings__pack(&frame, buffer + prefix_size);

    return prefix_size + data_size;
}

static size_t lora_create_data_packet(uint8_t* buffer, size_t size) {
    LoRaFrame frame;
    create_porotobuf_data_frame(&frame);

    uint8_t data_size = 0;
    uint8_t prefix_size = 0;
    prefix_size = add_prefix(buffer, size);
    data_size = lo_ra_frame__pack(&frame, buffer + prefix_size);

    return prefix_size + data_size;
}

static size_t lora_packet(uint8_t* buffer, size_t buffer_size) {
    size_t size = 0;

    if (settings_frame == true) {
        size = lora_create_settings_packet(buffer, buffer_size);
        settings_frame = false;
        ESP_LOGI(TAG, "Transmiting settings frame");
    } else {
        size = lora_create_data_packet(buffer, buffer_size);
    }

    ESP_LOGI(TAG, "Sending LoRa frame -> size: %d", size);

    return size;
}

bool initialize_lora(uint32_t frequency_khz, uint32_t transmiting_period) {
    RETURN_ON_FALSE(lora_hw_spi_add_device(VSPI_HOST));
    RETURN_ON_FALSE(lora_hw_set_gpio());
    RETURN_ON_FALSE(lora_hw_attach_d0_interrupt(lora_task_irq_notify));
    lora_struct_t lora = {._spi_transmit = lora_hw_spi_transmit,
                          ._delay = lora_hw_delay,
                          ._gpio_set_level = lora_hw_gpio_set_level,
                          .log = lora_hw_log,
                          .rst_gpio_num = CONFIG_LORA_RS,
                          .cs_gpio_num = CONFIG_LORA_CS,
                          .d0_gpio_num = CONFIG_LORA_D0,
                          .implicit_header = 0,
                          .frequency = 0};
    lora_api_config_t cfg = {
        .lora = &lora,
        .process_rx_packet_fnc = lora_process,
        .get_tx_packet_fnc = lora_packet,
        .frequency_khz = frequency_khz,
        .transmiting_period = transmiting_period,
    };
    RETURN_ON_FALSE(lora_task_init(&cfg));
    return true;
}