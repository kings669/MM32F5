#ifndef __I2S_H__
#define __I2S_H__
#include "hal_common.h"
#include "hal_spi.h"
#include "hal_dma.h"
#include "hal_dma_request.h"
#include "board_init.h"

void I2S_Configure(SPI_I2S_STANDARD_TypeDef   Standard,
                         SPI_I2S_DATAFORMAT_TypeDef DataFormat,
                         SPI_I2S_AUDIO_FREQ_TypeDef AudioFreq,
                         SPI_I2S_TRANS_MODE_TypeDef Mode);
void I2S_DMA_Transfer(uint16_t *Buffer, uint32_t BufferSize);
void i2s_init(void);
void I2S_PowerON(uint8_t Enable);
#endif

