/*
 * Copyright 2021 MindMotion Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pin_init.h"
#include "hal_rcc.h"
#include "hal_gpio.h"

void BOARD_InitPins(void)
{
    /* PB6 - UART1_TX. */
    GPIO_Init_Type gpio_init;
    gpio_init.Pins  = GPIO_PIN_6;
    gpio_init.PinMode  = GPIO_PinMode_AF_PushPull;
    gpio_init.Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio_init);
    GPIO_PinAFConf(GPIOB, gpio_init.Pins, GPIO_AF_7);

    /* PB7 - UART1_RX. */
    gpio_init.Pins  = GPIO_PIN_7;
    gpio_init.PinMode  = GPIO_PinMode_In_Floating;
    gpio_init.Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio_init);
    GPIO_PinAFConf(GPIOB, gpio_init.Pins, GPIO_AF_7);
    
    /* PC12 - GPIO output: SPI3_MOSI. */
    gpio_init.Pins  = GPIO_PIN_12;
    gpio_init.PinMode  = GPIO_PinMode_AF_PushPull;
    gpio_init.Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio_init);
    GPIO_PinAFConf(GPIOC, gpio_init.Pins, GPIO_AF_6); 

    /* PC11 - GPIO input: SPI3_MISO. */
    gpio_init.Pins  = GPIO_PIN_11;
    gpio_init.PinMode  = GPIO_PinMode_In_Floating;
    gpio_init.Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio_init);
    GPIO_PinAFConf(GPIOC, gpio_init.Pins, GPIO_AF_6); 

    /* PC10 - GPIO output: SPI3_SCK. */
    gpio_init.Pins  = GPIO_PIN_10;
    gpio_init.PinMode  = GPIO_PinMode_AF_PushPull;
    gpio_init.Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio_init);
    GPIO_PinAFConf(GPIOC, gpio_init.Pins, GPIO_AF_6); 

    /* PA15 - GPIO output: SPI3_CS. */
    gpio_init.Pins  = GPIO_PIN_15;
    gpio_init.PinMode  = GPIO_PinMode_AF_PushPull;
    gpio_init.Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);
    GPIO_PinAFConf(GPIOA, gpio_init.Pins, GPIO_AF_6); 
		
    /* PD3 - I2S_CK. */
    gpio_init.Pins  = GPIO_PIN_3;
    gpio_init.PinMode  = GPIO_PinMode_AF_PushPull; //GPIO_PinMode_In_PushPull
    gpio_init.Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &gpio_init);
    GPIO_PinAFConf(GPIOD, gpio_init.Pins, GPIO_AF_5);

    /* PE6 - I2S_SD. */
    gpio_init.Pins  = GPIO_PIN_6;
    gpio_init.PinMode  = GPIO_PinMode_AF_PushPull; //GPIO_PinMode_In_PushPull
    gpio_init.Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOE, &gpio_init);
    GPIO_PinAFConf(GPIOE, gpio_init.Pins, GPIO_AF_5);

    /* PE4 - I2S_WS. */
    gpio_init.Pins  = GPIO_PIN_4;
    gpio_init.PinMode  = GPIO_PinMode_AF_PushPull; //GPIO_PinMode_In_PushPull
    gpio_init.Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOE, &gpio_init);
    GPIO_PinAFConf(GPIOE, gpio_init.Pins, GPIO_AF_5);

    /* PE5 - I2S_MCK. */
    gpio_init.Pins  = GPIO_PIN_5;
    gpio_init.PinMode  = GPIO_PinMode_AF_PushPull; //GPIO_PinMode_In_PushPull
    gpio_init.Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOE, &gpio_init);
    GPIO_PinAFConf(GPIOE, gpio_init.Pins, GPIO_AF_5);
}


/* EOF. */
