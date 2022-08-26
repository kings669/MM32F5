/*
 * Copyright 2022 MindMotion Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "board_init.h"
#include "sdspi.h"
#include "hal_spi.h"

/* pins:
 * tx : PC12/SPI_MOSI
 * rx : PC8/SPI0_MISO
 * clk: PC12/SPI0_SCK
 * cs : PC11/SPI0_PCS0
 */

#define BOARD_SDSPI_TX_GPIO_PORT  GPIOC
#define BOARD_SDSPI_TX_GPIO_PIN   GPIO_PIN_12

#define BOARD_SDSPI_RX_GPIO_PORT  GPIOC
#define BOARD_SDSPI_RX_GPIO_PIN   GPIO_PIN_11

#define BOARD_SDSPI_CLK_GPIO_PORT GPIOC
#define BOARD_SDSPI_CLK_GPIO_PIN  GPIO_PIN_10

#define BOARD_SDSPI_CS_GPIO_PORT  GPIOA
#define BOARD_SDSPI_CS_GPIO_PIN   GPIO_PIN_15

SDSPI_ApiRetStatus_Type sdspi_spi_init(void);
SDSPI_ApiRetStatus_Type sdspi_spi_freq(uint32_t hz);
SDSPI_ApiRetStatus_Type sdspi_spi_xfer(uint8_t *in, uint8_t *out, uint32_t len);

const SDSPI_Interface_Type board_sdspi_if =
{
    .baudrate = 1000000u, /* 1mhz. */
    .spi_init = sdspi_spi_init,
    .spi_freq = sdspi_spi_freq,
    .spi_xfer = sdspi_spi_xfer
};

uint32_t board_sdspi_delay_count;

static void board_sdspi_delay(uint32_t count)
{
    for (uint32_t i = count; i > 0u; i--)
    {
        __NOP();
    }
}

SDSPI_ApiRetStatus_Type sdspi_spi_init(void)
{
    GPIO_WriteBit(BOARD_SDSPI_CS_GPIO_PORT , BOARD_SDSPI_CS_GPIO_PIN , 1u);
//    GPIO_WriteBit(BOARD_SDSPI_TX_GPIO_PORT , BOARD_SDSPI_TX_GPIO_PIN , 0u);
//    GPIO_WriteBit(BOARD_SDSPI_CLK_GPIO_PORT, BOARD_SDSPI_CLK_GPIO_PIN, 0u);
	
    /* Setup SPI module. */
    SPI_Master_Init_Type spi_init;
    spi_init.ClockFreqHz = CLOCK_APB1_FREQ;
    spi_init.BaudRate = SDMMC_CLOCK_400KHZ;
    spi_init.XferMode = SPI_XferMode_TxRx;
    spi_init.PolPha = SPI_PolPha_Alt2;
    spi_init.DataWidth = SPI_DataWidth_8b;
    spi_init.LSB = false;
    spi_init.AutoCS = true;
    SPI_InitMaster(SPI3, &spi_init);
	
    /* Enable SPI. */
    SPI_Enable(SPI3, true);
	

//    board_sdspi_delay_count = 100u;
    return SDSPI_ApiRetStatus_Success;
}

void SPI_SetBaudRate(SPI_Type * SPIx, uint32_t src_clk, uint32_t baudrate)
{
    uint32_t div = src_clk / baudrate;
    if (div < 2u)
    {
        /* div = 0, 1 is not allowed. */
        div = 2u;
    }
    SPIx->SPBRG = div;
    if (div <= 4)
    {
        /* to support high speed mode. */
        SPIx->CCTL |= (SPI_I2S_CCTL_TXEDGE_MASK | SPI_I2S_CCTL_RXEDGE_MASK);
    }
    else
    {
        SPIx->CCTL &= ~(SPI_I2S_CCTL_TXEDGE_MASK | SPI_I2S_CCTL_RXEDGE_MASK);
    }
}

SDSPI_ApiRetStatus_Type sdspi_spi_freq(uint32_t hz)
{
    switch (hz)
    {
    case SDMMC_CLOCK_400KHZ:
				SPI_Enable(SPI3, false);
				SPI_SetBaudRate(SPI3,CLOCK_APB1_FREQ,SDMMC_CLOCK_400KHZ);	
				SPI_Enable(SPI3, true);
        break;
    default:
				SPI_Enable(SPI3, false);
				SPI_SetBaudRate(SPI3,CLOCK_APB1_FREQ,SD_CLOCK_25MHZ);	
				SPI_Enable(SPI3, true);
        break;
    }
    return SDSPI_ApiRetStatus_Success;
}

//SDSPI_ApiRetStatus_Type sdspi_spi_freq(uint32_t hz)
//{
//    switch (hz)
//    {
//    case SDMMC_CLOCK_400KHZ:
//        board_sdspi_delay_count = 100u;
//        break;
//    default:
//        board_sdspi_delay_count = 0u;
//        break;
//    }
//    return SDSPI_ApiRetStatus_Success;
//}
/* SPI tx. */
void app_spi_putbyte(uint8_t c)
{
    /* Polling for tx empty. */
    while ( SPI_STATUS_TX_FULL & SPI_GetStatus(SPI3) )
    {}
    SPI_PutData(SPI3, c);
}

/* SPI rx. */
uint8_t app_spi_getbyte(void)
{
    /* Polling for rx done. */
    while (0u == (SPI_STATUS_RX_DONE & SPI_GetStatus(SPI3)) )
    {}
    return SPI_GetData(SPI3);
}

uint8_t spi_xfer(uint8_t tx_dat)
{
    uint8_t rx_dat = 0u;
		app_spi_putbyte(tx_dat);
		rx_dat=app_spi_getbyte();
    return rx_dat;
}
//uint8_t spi_xfer(uint8_t tx_dat)
//{
//    uint8_t rx_dat = 0u;

//    for (uint8_t i = 0u; i < 8u; i++)
//    {
//        GPIO_WriteBit(BOARD_SDSPI_CLK_GPIO_PORT, BOARD_SDSPI_CLK_GPIO_PIN, 0u );
//        GPIO_WriteBit(BOARD_SDSPI_TX_GPIO_PORT, BOARD_SDSPI_TX_GPIO_PIN, (0u != (tx_dat & (1u << (7u-i)))) ? 1u : 0u );
//        board_sdspi_delay(board_sdspi_delay_count);
//        GPIO_WriteBit(BOARD_SDSPI_CLK_GPIO_PORT, BOARD_SDSPI_CLK_GPIO_PIN, 1u );
//        board_sdspi_delay(board_sdspi_delay_count);
//        rx_dat = (rx_dat << 1u) | GPIO_ReadInDataBit(BOARD_SDSPI_RX_GPIO_PORT, BOARD_SDSPI_RX_GPIO_PIN);
//    }

//    return rx_dat;
//}

void spi_assert_cs(bool enable)
{
    GPIO_WriteBit(BOARD_SDSPI_CS_GPIO_PORT, BOARD_SDSPI_CS_GPIO_PIN, (enable ? 0u: 1u) );
}

SDSPI_ApiRetStatus_Type sdspi_spi_xfer(uint8_t *in, uint8_t *out, uint32_t len)
{
    uint8_t inbuf, outbuf;
    //spi_assert_cs(true);

    for (uint32_t i = 0u; i < len; i++)
    {
        inbuf = (in == NULL) ? SDSPI_DUMMY_DATA: *in++;
        outbuf = spi_xfer(inbuf);
        if (out)
        {
            *out = outbuf;
            out++;
        }
    }

    //spi_assert_cs(false);

    return SDSPI_ApiRetStatus_Success;
}

/* EOF. */

