#ifndef __BOARD_IT_H_
#define __BOARD_IT_H_
#include "hal_common.h"
#ifdef __cplusplus
 extern "C" {
#endif 
void SysTick_Init(void);
void SysTick_Handler(void);
uint32_t GetSysRunTimeMs(void);
void delay_us(uint16_t nus);
void delay_ms(uint16_t nms);
#ifdef __cplusplus
}
#endif

#endif
