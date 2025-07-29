#ifndef DATA_PROTOCOL_H
#define DATA_PROTOCOL_H

#include <stdint.h>

// dodac typy pakietow!
typedef enum {
    PACKET_TYPE_TELEMETRY = 0x01,
    PACKET_TYPE_COMMAND = 0x02,
    PACKET_TYPE_ACK = 0x03,
} packet_type_t;


typedef struct {
    uint8_t packet_type;
    uint32_t timestamp_ms;
    
    float latitude;
    float longitude;
    float altitude;
    uint8_t satellites;
    // dane z mcb - dodac reszte
    float pressure;
    float temperature;
    float acceleration_x;
    float gps_fix;
    float mcb_data;

    uint16_t status_flags;
    uint16_t crc16;
} telemetry_frame_t;


typedef struct {
    uint8_t packet_type;
    uint8_t command_id;
    uint32_t command_arg;
    uint16_t crc16;
} command_frame_t;

#endif 