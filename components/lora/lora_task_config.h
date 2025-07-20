// Copyright 2022 PWrInSpace, Kuba
#ifndef LORA_TASK_CONFIG_H
#define LORA_TASK_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include "lora_task.h"

#define PACKET_PREFIX "SP3MIK"

/**
 * @brief Initialize lora api and run lora task
 *q
 * @param frequency_khz lora frequency kzh
 * @param transmiting_period lora transmiting period
 * @return true :D
 * @return false :C
 */
bool initialize_lora(uint32_t frequency_khz, uint32_t transmiting_period);


void lora_send_settings_frame(void);

#endif
