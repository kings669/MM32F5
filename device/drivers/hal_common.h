/*
 * Copyright 2021 MindMotion Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __HAL_COMMON_H__
#define __HAL_COMMON_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "hal_device_registers.h"

//I2S
#include "hal_i2s.h"
#include "hal_spi.h"
#include "board_init.h"
#include "hal_dma.h"
#include "hal_dma_request.h"

#include "ffconf.h"
#include "ff.h"

#include "wav.h"
#include "audio.h"

#define DTCM_RAM __attribute__((section(".RAM_D1")))


#endif /* __HAL_COMMON_H__ */

