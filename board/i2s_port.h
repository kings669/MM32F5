#ifndef __I2S_PORT_H_
#define __I2S_PORT_H_

#include "hal_common.h"
extern void I2S_Configure(I2S_Protocol_Type   Standard,I2S_DataWidth_Type DataFormat, uint32_t AudioFreq,I2S_XferMode_Type Mode);
extern void I2S_DMA_Transfer(uint16_t *Buffer, uint32_t BufferSize);
extern void I2S_PowerON(uint8_t Enable);
#endif
