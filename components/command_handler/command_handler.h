#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief 
 */
//dodac komendy jakie maja byc
typedef enum {
    CMD_PING = 0x01,        
    CMD_SET_MODE = 0x10,    
    CMD_REBOOT = 0xFF,      
} command_id_t;


/**
 * @brief przetwarza surowy pakiet danych odebrany od stacji 
 * @param data 
 * @param len 
 */
void command_handler_process_packet(const uint8_t *data, size_t len);


/**
 * @brief 
 * @param data 
 * @param length 
 * @return 
 */
uint16_t command_handler_calculate_crc16(const uint8_t *data, size_t length);

#endif