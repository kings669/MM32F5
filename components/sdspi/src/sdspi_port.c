/* sdcard_spi_port.c */
#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_gpio.h"

#include "sdspi.h"

status_t sdspi_spi_init(void);
status_t sdspi_spi_freq(uint32_t hz);
status_t sdspi_spi_xfer(uint8_t *in, uint8_t *out, uint32_t len);

const sdspi_interface_t sdspi_if =
{
    .baudrate = 1000000u, /* 1mhz. */
    .spi_init = sdspi_spi_init,
    .spi_freq = sdspi_spi_freq,
    .spi_xfer = sdspi_spi_xfer
};

/* pins:
 * tx : PTC6/SPI0_MOSI
 * rx : PTC7/SPI0_MISO
 * clk: PTC5/SPI0_SCK
 * cs : PTC4/SPI0_PCS0
 */

status_t sdspi_spi_init(void)
{
    /* pin mux. */
    CLOCK_EnableClock(kCLOCK_PortC);
    PORTC->PCR[4] = PORT_PCR_MUX(1); /* PTC4: SPI0_PCS0, GPIO模式. */
    PORTC->PCR[5] = PORT_PCR_MUX(2); /* PTC5: SPI0_CLK. */
    PORTC->PCR[6] = PORT_PCR_MUX(2); /* PTC6: SPI0_MOSI */
    PORTC->PCR[7] = PORT_PCR_MUX(2) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; /* PTC7: SPI0_MISO. */

    /* gpio for cs pin. */
    gpio_pin_config_t gpio_pin_config;
    gpio_pin_config.pinDirection = kGPIO_DigitalOutput;
    gpio_pin_config.outputLogic = 1u;
    GPIO_PinInit(GPIOC, 4u, &gpio_pin_config);

    /* spi0. */
    /* spi0使用bus clock. */
    CLOCK_EnableClock(kCLOCK_Spi0);
    SPI0->C1 = SPI_C1_SPE_MASK /* enable spi. */
             | SPI_C1_MSTR_MASK /* master mode. */
             | SPI_C1_CPOL(0) /* clock polarity. */
             | SPI_C1_CPHA(0) /* phase polarity. */
             ;
    SPI0->C2 = 0u;
    SPI0->BR = SPI_BR_SPPR(2) | SPI_BR_SPR(2); /* div = 3x8. busclk = 24mhz, baudrate = 1mhz. */

    return kStatus_Success;
}

status_t sdspi_spi_freq(uint32_t hz)
{
    switch (hz)
    {
    case SDMMC_CLOCK_400KHZ:
        SPI0->BR = SPI_BR_SPPR(2) | SPI_BR_SPR(4); /* div = 3x32. busclk = 24mhz, baudrate = 250khz. */
        break;
    default:
        SPI0->BR = SPI_BR_SPPR(2) | SPI_BR_SPR(2); /* div = 3x32. busclk = 24mhz, baudrate = 1mhz. */
        break;
    }
    return kStatus_Success;
}

uint8_t spi_xfer(uint8_t tx_dat)
{
    uint8_t rx_dat;
    while (! (SPI0->S & SPI_S_SPTEF_MASK) )
    {}
    SPI0->DL = tx_dat;
    while (! (SPI0->S & SPI_S_SPRF_MASK) )
    {}
    rx_dat = SPI0->DL;

    return rx_dat;
}

void spi_assert_cs(bool enable)
{
    GPIO_WritePinOutput(GPIOC, 4u, (enable ? 0u: 1u) );
}

status_t sdspi_spi_xfer(uint8_t *in, uint8_t *out, uint32_t len)
{
    uint8_t inbuf, outbuf;
    spi_assert_cs(true);

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

    spi_assert_cs(false);

    return kStatus_Success;
}

/* EOF. */

