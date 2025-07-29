#include "data_protocol.h"
#include <stddef.h>

/**
 * @brief 
 */
uint16_t data_protocol_calculate_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF; 

    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021; // wielomian 0x1021
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}