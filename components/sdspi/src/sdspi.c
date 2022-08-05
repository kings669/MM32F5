/* sdspi.c */

#include <assert.h>
#include <string.h>
#include "sdspi.h"

/*******************************************************************************
 * Definitons
 ******************************************************************************/
/*! @brief Card cmd maximum retry times value */
#define SDSPI_TRANSFER_RETRY_TIMES (20000u)

/*! @brief define SDSPI cmd code length */
#define SDSPI_CMD_CODE_BYTE_LEN (6u)
#define SDSPI_CMD_FORMAT_GET_INDEX(cmd) ((cmd >> 8U) & 0xff)
#define SDSPI_CMD_FORMAT_GET_RESPONSE_TYPE(cmd) (cmd & 0xff)
#define SDSPI_CMD_IDLE_CRC (0x95)
#define SDSPI_CMD_SEND_INTERFACE_CRC (0x87)

/*! @brief Reverse byte sequence in uint32_t */
//#define SWAP_WORD_BYTE_SEQUENCE(x) (__REV(x))

#define SWAP_WORD_BYTE_SEQUENCE(x) (   (0xff000000 & ((x)<<24u) ) \
                                     | (0x00ff0000 & ((x)<< 8u) ) \
                                     | (0x0000ff00 & ((x)>> 8u) ) \
                                     | (0x000000ff & ((x)>>24u) ) )

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Wait card to be ready state.
 *
 * @param interface interface state.
 * @retval SDSPI_ApiRetStatus_SDSPI_XferFail Exchange data over SPI Fail.
 * @retval SDSPI_ApiRetStatus_SDSPI_ResponseError Response is error.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_WaitReady(SDSPI_Interface_Type *interface);

#if SDSPI_CARD_CRC_PROTECTION_ENABLE
/*!
 * @brief Calculate CRC7
 *
 * @param buffer Data buffer.
 * @param length Data length.
 * @param crc The orginal crc value.
 * @return Generated CRC7.
 */
static uint32_t SDSPI_GenerateCRC7(uint8_t *buffer, uint32_t length, uint32_t crc);
#endif

/*!
 * @brief Send cmd.
 *
 * @param interface interface state.
 * @param cmd The cmd to be wrote.
 * @param arg cmd argument
 * @param resp resp buffer
 * @retval SDSPI_ApiRetStatus_SDSPI_WaitReadyFail Wait ready Fail.
 * @retval SDSPI_ApiRetStatus_SDSPI_XferFail Exchange data over SPI Fail.
 * @retval SDSPI_ApiRetStatus_SDSPI_ResponseError Response is error.
 * @retval SDSPI_ApiRetStatus_Fail Send cmd Fail.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_SendCmd(SDSPI_Interface_Type *interface, uint32_t cmd, uint32_t arg, uint8_t *resp);

/*!
 * @brief Send GO_IDLE cmd.
 *
 * @param card Card descriptor.
 * @retval SDSPI_ApiRetStatus_SDSPI_XferFail Send timing byte Fail.
 * @retval SDSPI_ApiRetStatus_SDSPI_SendCmdFail Send cmd Fail.
 * @retval SDSPI_ApiRetStatus_SDSPI_ResponseError Response is error.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_GoIdle(SDSPI_CardHandler_Type *card);

/*!
 * @brief Send GET_INTERFACE_CONDITION cmd.
 *
 * This function checks card interface condition, which includes interface supply voltage information and asks the card
 * whether it supports voltage.
 *
 * @param card Card descriptor.
 * @param flags The interface Capacity Support flag
 * @retval SDSPI_ApiRetStatus_SDSPI_SendCmdFail Send cmd Fail.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_SendIfCond(SDSPI_CardHandler_Type *card, uint32_t *flags);

/*!
 * @brief Send SEND_APPLICATION_COMMAND cmd.
 *
 * @param card Card descriptor.
 * @retval SDSPI_ApiRetStatus_SDSPI_SendCmdFail Send cmd Fail.
 * @retval SDSPI_ApiRetStatus_SDSPI_ResponseError Response is error.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_SendAppCmd(SDSPI_CardHandler_Type *card);

/*!
 * @brief Send GET_OPERATION_CONDITION cmd.
 *
 * @param card Card descriptor.
 * @param argument Operation condition.
 * @retval kuint32_timeout Timeout.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_AppSendOpCond(SDSPI_CardHandler_Type *card, uint32_t arg);

/*!
 * @brief Send READ_OCR cmd to get OCR register content.
 *
 * @param card Card descriptor.
 * @retval SDSPI_ApiRetStatus_SDSPI_SendCmdFail Send cmd Fail.
 * @retval SDSPI_ApiRetStatus_SDSPI_ResponseError Response is error.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_ReadOcr(SDSPI_CardHandler_Type *card);

/*!
 * @brief Send SET_BLOCK_SIZE cmd.
 *
 * This function sets the block length in bytes for SDSC cards. For SDHC cards, it does not affect memory
 * read or write cmds, always 512 bytes fixed block length is used.
 * @param card Card descriptor.
 * @param blockSize Block size.
 * @retval SDSPI_ApiRetStatus_SDSPI_SendCmdFail Send cmd Fail.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_SetBlockSize(SDSPI_CardHandler_Type *card, uint32_t blockSize);

/*!
 * @brief Read data from card
 *
 * @param interface interface state.
 * @param buffer Buffer to save data.
 * @param size The data size to read.
 * @retval SDSPI_ApiRetStatus_SDSPI_ResponseError Response is error.
 * @retval SDSPI_ApiRetStatus_SDSPI_XferFail Exchange data over SPI Fail.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_Read(SDSPI_Interface_Type *interface, uint8_t *buffer, uint32_t size);

/*!
 * @brief Decode CSD register
 *
 * @param card Card descriptor.
 * @param rawCsd Raw CSD register content.
 */
static void SDSPI_DecodeCsd(SDSPI_CardHandler_Type *card, uint8_t *rawCsd);

/*!
 * @brief Send GET-CSD cmd.
 *
 * @param card Card descriptor.
 * @retval SDSPI_ApiRetStatus_SDSPI_SendCmdFail Send cmd Fail.
 * @retval SDSPI_ApiRetStatus_SDSPI_ReadFail Read data blocks Fail.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_SendCsd(SDSPI_CardHandler_Type *card);

/*!
 * @brief Decode raw CID register.
 * In our sdspi init function, this function is removed for better code size, if id information
 * is needed, you can call it after the init function directly.
 * @param card Card descriptor.
 * @param rawCid Raw CID register content.
 */
static void SDSPI_DecodeCid(SDSPI_CardHandler_Type *card, uint8_t *rawCid);

/*!
 * @brief Decode SCR register.
 *
 * @param card Card descriptor.
 * @param rawScr Raw SCR register content.
 */
static void SDSPI_DecodeScr(SDSPI_CardHandler_Type *card, uint8_t *rawScr);

/*!
 * @brief Send SEND_SCR cmd.
 *
 * @param card Card descriptor.
 * @retval SDSPI_ApiRetStatus_SDSPI_SendCmdFail Send cmd Fail.
 * @retval SDSPI_ApiRetStatus_SDSPI_ReadFail Read data blocks Fail.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_SendScr(SDSPI_CardHandler_Type *card);

/*!
 * @brief Send STOP_TRANSMISSION cmd to card to stop ongoing data transferring.
 *
 * @param card Card descriptor.
 * @retval SDSPI_ApiRetStatus_SDSPI_SendCmdFail Send cmd Fail.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_StopTrans(SDSPI_CardHandler_Type *card);

/*!
 * @brief Write data to card
 *
 * @param interface interface state.
 * @param buffer Data to send.
 * @param size Data size.
 * @param token The data token.
 * @retval SDSPI_ApiRetStatus_SDSPI_WaitReadyFail Card is busy error.
 * @retval SDSPI_ApiRetStatus_SDSPI_XferFail Exchange data over SPI Fail.
 * @retval SDSPI_ApiRetStatus_InvalidArgument Invalid argument.
 * @retval SDSPI_ApiRetStatus_SDSPI_ResponseError Response is error.
 * @retval SDSPI_ApiRetStatus_Success Operate successfully.
 */
static SDSPI_ApiRetStatus_Type SDSPI_Write(SDSPI_Interface_Type *interface, uint8_t *buffer, uint32_t size, uint8_t token);

/*!
 * @brief select function.
 *
 * @param card card descriptor.
 * @param group function group.
 * @param function function name.
 */
static SDSPI_ApiRetStatus_Type SDSPI_SelectFunction(SDSPI_CardHandler_Type *card, uint32_t group, uint32_t function);

/*!
 * @brief Send SWITCH_FUNCTION cmd to switch the card function group.
 *
 * @param card card descriptor.
 * @param mode 0 to check function group, 1 to switch function group.
 * @param group function group.
 * @param number function name.
 * @status buffer to recieve function status.
 */
static SDSPI_ApiRetStatus_Type SDSPI_SwitchFunction(SDSPI_CardHandler_Type *card, uint32_t mode, uint32_t group, uint32_t number, uint32_t *status);

/*!
 * @brief Erase data for the given block range..
 *
 * @param card card descriptor.
 * @param startBlock start block address.
 * @param blockCount the number of block to be erase.
 */
static SDSPI_ApiRetStatus_Type SDSPI_Erase(SDSPI_CardHandler_Type *card, uint32_t startBlock, uint32_t blockCount);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
static SDSPI_ApiRetStatus_Type SDSPI_WaitReady(SDSPI_Interface_Type *interface)
{
    uint8_t resp;
    uint8_t timingByte = 0xFFU; /* The byte need to be sent as read/write data block timing requirement */
    uint32_t retryCount = SDSPI_TRANSFER_RETRY_TIMES;

    do
    {
        if (SDSPI_ApiRetStatus_Success != interface->spi_xfer(&timingByte, &resp, 1U))
        {
            return SDSPI_ApiRetStatus_SDSPI_XferFail;
        }

    } while ((resp != 0xFFU) && (--retryCount));

    /* Response 0xFF means card is still busy. */
    if (resp != 0xFFU)
    {
        return SDSPI_ApiRetStatus_SDSPI_ResponseError;
    }

    return SDSPI_ApiRetStatus_Success;
}

#if SDSPI_CARD_CRC_PROTECTION_ENABLE
static uint32_t SDSPI_GenerateCRC7(uint8_t *buffer, uint32_t length, uint32_t crc)
{
    uint32_t index;

    static const uint8_t crcTable[] = {0x00U, 0x09U, 0x12U, 0x1BU, 0x24U, 0x2DU, 0x36U, 0x3FU,
                                       0x48U, 0x41U, 0x5AU, 0x53U, 0x6CU, 0x65U, 0x7EU, 0x77U};

    while (length)
    {
        index = (((crc >> 3U) & 0x0FU) ^ ((*buffer) >> 4U));
        crc = ((crc << 4U) ^ crcTable[index]);

        index = (((crc >> 3U) & 0x0FU) ^ ((*buffer) & 0x0FU));
        crc = ((crc << 4U) ^ crcTable[index]);

        buffer++;
        length--;
    }

    return (crc & 0x7FU);
}

static uint16_t SDSPI_GenerateCRC16(uint8_t *buffer, uint32_t length, uint16_t crc)
{
    while (length)
    {
        crc = (uint8_t)(crc >> 8U) | (crc << 8U);
        crc ^= *buffer++;
        crc ^= (uint8_t)(crc & 0xffU) >> 4U;
        crc ^= (crc << 8U) << 4U;
        crc ^= ((crc & 0xffU) << 4U) << 1U;
        length--;
    }

    return (crc >> 8U) | (crc << 8U);
}
#endif

static SDSPI_ApiRetStatus_Type SDSPI_SendCmd(SDSPI_Interface_Type *interface, uint32_t cmd, uint32_t arg, uint8_t *resp)
{
    assert(interface);
    assert(resp);

    uint32_t i;
    uint8_t timingByte = 0xFFU; /* The byte need to be sent as read/write data block timing requirement */
    uint8_t buffer[SDSPI_CMD_CODE_BYTE_LEN] = {0U};
    uint32_t responseType = SDSPI_CMD_FORMAT_GET_RESPONSE_TYPE(cmd);
    uint8_t index = SDSPI_CMD_FORMAT_GET_INDEX(cmd);

    if ((SDSPI_ApiRetStatus_Success != SDSPI_WaitReady(interface)) && (index != SDMMC_GoIdleState))
    {
        return SDSPI_ApiRetStatus_SDSPI_WaitReadyFail;
    }

    /* Send cmd. */
    buffer[0U] = (index | 0x40U);
    buffer[1U] = ((arg >> 24U) & 0xFFU);
    buffer[2U] = ((arg >> 16U) & 0xFFU);
    buffer[3U] = ((arg >> 8U) & 0xFFU);
    buffer[4U] = (arg & 0xFFU);
#if SDSPI_CARD_CRC_PROTECTION_ENABLE
    buffer[5U] = ((SDSPI_GenerateCRC7(buffer, 5U, 0U) << 1U) | 1U);
#else
    if (index == SDMMC_GoIdleState)
    {
        buffer[5U] = SDSPI_CMD_IDLE_CRC;
    }
    else if (index == SD_SendInterfaceCondition)
    {
        buffer[5U] = SDSPI_CMD_SEND_INTERFACE_CRC;
    }
#endif

    if (interface->spi_xfer(buffer, NULL, SDSPI_CMD_CODE_BYTE_LEN))
    {
        return SDSPI_ApiRetStatus_SDSPI_XferFail;
    }

    /* Wait for the resp coming, the left most bit which is transfered first in first resp byte is 0 */
    for (i = 0U; i < 9U; i++)
    {
        if (SDSPI_ApiRetStatus_Success != interface->spi_xfer(&timingByte, &resp[0U], 1U))
        {
            return SDSPI_ApiRetStatus_SDSPI_XferFail;
        }

        /* Check if resp 0 coming. */
        if (!(resp[0U] & 0x80U))
        {
            break;
        }
    }

    if (resp[0U] & 0x80U) /* Max index byte is high means resp comming. */
    {
        return SDSPI_ApiRetStatus_SDSPI_ResponseError;
    }

    if (responseType != SDSPI_RespType_R1)
    {
        if (responseType == SDSPI_RespType_R1b)
        {
            if (SDSPI_ApiRetStatus_Success != SDSPI_WaitReady(interface))
            {
                return SDSPI_ApiRetStatus_SDSPI_WaitReadyFail;
            }
        }
        else if (responseType == SDSPI_RespType_R2)
        {
            if (SDSPI_ApiRetStatus_Success != interface->spi_xfer(&timingByte, &(resp[1U]), 1U))
            {
                return SDSPI_ApiRetStatus_SDSPI_XferFail;
            }
        }
        else if ((responseType == SDSPI_RespType_R3) || (responseType == SDSPI_RespType_R7))
        {
            /* Left 4 bytes in resp type R3 and R7(total 5 bytes in SPI mode) */
            if (SDSPI_ApiRetStatus_Success != interface->spi_xfer(&timingByte, &(resp[1U]), 4U))
            {
                return SDSPI_ApiRetStatus_SDSPI_XferFail;
            }
        }
    }

    return SDSPI_ApiRetStatus_Success;
}

#if SDSPI_CARD_CRC_PROTECTION_ENABLE
uint32_t SDSPI_EnableCmdCrc(SDSPI_CardHandler_Type *card, bool enable)
{
    uint8_t resp = 0U;

    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_Crc, enable, &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }

    return SDSPI_ApiRetStatus_Success;
}
#endif

static SDSPI_ApiRetStatus_Type SDSPI_GoIdle(SDSPI_CardHandler_Type *card)
{
    assert(card);
    assert(card->interface);

    uint8_t resp = 0U;
    uint32_t retryCount = SDSPI_TRANSFER_RETRY_TIMES;

    /* SD card will enter SPI mode if the CS is asserted (negative) during the reception of the reset cmd (CMD0)
    and the card will be IDLE state. */
    do
    {
        if ((SDSPI_ApiRetStatus_Success == SDSPI_SendCmd(card->interface, SDSPI_Cmd_GoIdle, 0U, &resp)) &&
            (resp == SDSPI_R1InIdleStateFlag))
        {
            break;
        }
    } while (--retryCount);

    return SDSPI_ApiRetStatus_Success;
}

static SDSPI_ApiRetStatus_Type SDSPI_SendIfCond(SDSPI_CardHandler_Type *card, uint32_t *flags)
{
    assert(card);
    assert(card->interface);

    uint8_t resp[5U] = {0U};
    uint32_t i = SDSPI_TRANSFER_RETRY_TIMES;

    do
    {
        /* CMD8 is used to check if the card accept the current supply voltage and check if
        the card support CMD8 */
        if (SDSPI_ApiRetStatus_Success == SDSPI_SendCmd(card->interface, SDSPI_Cmd_SendIfCond, 0x1AAU, resp))
        {
            /* not support CMD8, clear hcs flag */
            if (resp[0U] & SDSPI_R1IllegalCommandFlag)
            {
                return SDSPI_ApiRetStatus_Success;
            }
            /* if VCA is set and pattern is match, then break */
            if ((resp[3U] == 0x1U) && (resp[4U] == 0xAAU))
            {
                *flags |= SDMMC_MASK(SD_OcrHostCapacitySupportFlag);
                return SDSPI_ApiRetStatus_Success;
            }
            /* if VCA in the reponse not set, then the card not support current voltage, return fail */
            else if (resp[3U] == 0U)
            {
                return SDSPI_ApiRetStatus_SDSPI_InvalidVoltage;
            }
        }
        else
        {
            return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
        }
    } while (--i);

    return SDSPI_ApiRetStatus_Fail;
}

static SDSPI_ApiRetStatus_Type SDSPI_SendAppCmd(SDSPI_CardHandler_Type *card)
{
    assert(card);
    assert(card->interface);

    uint8_t resp = 0U;

    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_AppCmd, 0U, &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }

    if (resp && (!(resp & SDSPI_R1InIdleStateFlag)))
    {
        return SDSPI_ApiRetStatus_SDSPI_ResponseError;
    }

    return SDSPI_ApiRetStatus_Success;
}

static SDSPI_ApiRetStatus_Type SDSPI_AppSendOpCond(SDSPI_CardHandler_Type *card, uint32_t argument)
{
    assert(card);
    assert(card->interface);

    uint8_t resp = 0U;
    uint32_t i = SDSPI_TRANSFER_RETRY_TIMES;

    do
    {
        if (SDSPI_ApiRetStatus_Success == SDSPI_SendAppCmd(card))
        {
            if (SDSPI_ApiRetStatus_Success ==
                SDSPI_SendCmd(card->interface, SDSPI_Cmd_AppSendOpCond, argument, &resp))
            {
                if ((resp & SDSPI_R1InIdleStateFlag) == 0U)
                {
                    return SDSPI_ApiRetStatus_Success;
                }
            }
        }

    } while (--i);

    return SDSPI_ApiRetStatus_Fail;
}

static SDSPI_ApiRetStatus_Type SDSPI_ReadOcr(SDSPI_CardHandler_Type *card)
{
    assert(card);
    assert(card->interface);

    uint32_t i = 0U;
    uint8_t resp[5U] = {0U};

    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_ReadOcr, 0U, resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }

    if (resp[0U])
    {
        return SDSPI_ApiRetStatus_SDSPI_ResponseError;
    }

    /* Switch the bytes sequence. All register's content is transferred from highest byte to lowest byte. */
    card->ocr = 0U;
    for (i = 4U; i > 0U; i--)
    {
        card->ocr |= (uint32_t)resp[i] << ((4U - i) * 8U);
    }

    if (card->ocr & SDMMC_MASK(SD_OcrCardCapacitySupportFlag))
    {
        card->cardType |= SDSPI_CardType_HighCapacity;
    }

    return SDSPI_ApiRetStatus_Success;
}

static SDSPI_ApiRetStatus_Type SDSPI_SetBlockSize(SDSPI_CardHandler_Type *card, uint32_t blockSize)
{
    assert(card);
    assert(card->interface);

    uint8_t resp = 0U;
    uint32_t i = SDSPI_TRANSFER_RETRY_TIMES;

    do
    {
        if (SDSPI_ApiRetStatus_Success == SDSPI_SendCmd(card->interface, SDSPI_Cmd_SetBlockLength, blockSize, &resp))
        {
            return SDSPI_ApiRetStatus_Success;
        }
    } while (--i);

    return SDSPI_ApiRetStatus_Fail;
}

static void SDSPI_DecodeCsd(SDSPI_CardHandler_Type *card, uint8_t *rawCsd)
{
    assert(rawCsd);
    assert(card);

    sd_csd_t *csd = &(card->csd);

    csd->csdStructure = (rawCsd[0U] >> 6U);
    csd->dataReadAccessTime1 = rawCsd[1U];
    csd->dataReadAccessTime2 = rawCsd[2U];
    csd->transferSpeed = rawCsd[3U];
    csd->cardCommandClass = (((uint32_t)rawCsd[4U] << 4U) | ((uint32_t)rawCsd[5U] >> 4U));
    csd->readBlockLength = ((rawCsd)[5U] & 0xFU);
    if (rawCsd[6U] & 0x80U)
    {
        csd->flags |= SD_CsdReadBlockPartialFlag;
    }
    if (rawCsd[6U] & 0x40U)
    {
        csd->flags |= SD_CsdWriteBlockMisalignFlag;
    }
    if (rawCsd[6U] & 0x20U)
    {
        csd->flags |= SD_CsdReadBlockMisalignFlag;
    }
    if (rawCsd[6U] & 0x10U)
    {
        csd->flags |= SD_CsdDsrImplementedFlag;
    }

    /* Some fileds is different when csdStructure is different. */
    if (csd->csdStructure == 0U) /* Decode the bits when CSD structure is version 1.0 */
    {
        csd->deviceSize = ((((uint32_t)rawCsd[6] & 0x3U) << 10U) | ((uint32_t)rawCsd[7U] << 2U) | ((uint32_t)rawCsd[8U] >> 6U));
        csd->readCurrentVddMin = ((rawCsd[8U] >> 3U) & 7U);
        csd->readCurrentVddMax = (rawCsd[8U] >> 7U);
        csd->writeCurrentVddMin = ((rawCsd[9U] >> 5U) & 7U);
        csd->writeCurrentVddMax = (rawCsd[9U] >> 2U);
        csd->deviceSizeMultiplier = (((rawCsd[9U] & 3U) << 1U) | (rawCsd[10U] >> 7U));

        card->blockCount = (csd->deviceSize + 1U) << (csd->deviceSizeMultiplier + 2U);
        card->blockSize = (1U << (csd->readBlockLength));

        if (card->blockSize != SDSPI_DEFAULT_BLOCK_SIZE)
        {
            card->blockCount = (card->blockCount * card->blockSize);
            card->blockSize = SDSPI_DEFAULT_BLOCK_SIZE;
            card->blockCount = (card->blockCount / card->blockSize);
        }
        /* CSD V1.0 support SDSC card only */
        card->cardType |= SDSPI_CardType_Sdsc;
    }
    else if (csd->csdStructure == 1U) /* Decode the bits when CSD structure is version 2.0 */
    {
        card->blockSize = SDSPI_DEFAULT_BLOCK_SIZE;
        csd->deviceSize = ((((uint32_t)rawCsd[7U] & 0x3FU) << 16U) | ((uint32_t)rawCsd[8U] << 8U) | ((uint32_t)rawCsd[9U]));
        if (csd->deviceSize >= 0xFFFFU)
        {
            card->cardType |= SDSPI_CardType_Sdxc;
        }
        else
        {
            card->cardType |= SDSPI_CardType_Sdhc;
        }
        card->blockCount = ((csd->deviceSize + 1U) * 1024U);
    }
    else
    {
    }

    if ((rawCsd[10U] >> 6U) & 1U)
    {
        csd->flags |= SD_CsdEraseBlockEnabledFlag;
    }
    csd->eraseSectorSize = (((rawCsd[10U] & 0x3FU) << 1U) | (rawCsd[11U] >> 7U));
    csd->writeProtectGroupSize = (rawCsd[11U] & 0x7FU);
    if (rawCsd[12U] >> 7U)
    {
        csd->flags |= SD_CsdWriteProtectGroupEnabledFlag;
    }
    csd->writeSpeedFactor = ((rawCsd[12U] >> 2U) & 7U);
    csd->writeBlockLength = (((rawCsd[12U] & 3U) << 2U) | (rawCsd[13U] >> 6U));
    if ((rawCsd[13U] >> 5U) & 1U)
    {
        csd->flags |= SD_CsdWriteBlockPartialFlag;
    }
    if (rawCsd[14U] >> 7U)
    {
        csd->flags |= SD_CsdFileFormatGroupFlag;
    }
    if ((rawCsd[14U] >> 6U) & 1U)
    {
        csd->flags |= SD_CsdCopyFlag;
    }
    if ((rawCsd[14U] >> 5U) & 1U)
    {
        csd->flags |= SD_CsdPermanentWriteProtectFlag;
    }
    if ((rawCsd[14U] >> 4U) & 1U)
    {
        csd->flags |= SD_CsdTemporaryWriteProtectFlag;
    }
    csd->fileFormat = ((rawCsd[14U] >> 2U) & 3U);
}

static SDSPI_ApiRetStatus_Type SDSPI_SendCsd(SDSPI_CardHandler_Type *card)
{
    assert(card);
    assert(card->interface);

    uint8_t resp = 0U;

    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_SendCsd, 0U, &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }

    if (SDSPI_ApiRetStatus_Success != SDSPI_Read(card->interface, card->rawCsd, sizeof(card->rawCsd)))
    {
        return SDSPI_ApiRetStatus_SDSPI_ReadFail;
    }

    SDSPI_DecodeCsd(card, card->rawCsd);

    return SDSPI_ApiRetStatus_Success;
}

static void SDSPI_DecodeCid(SDSPI_CardHandler_Type *card, uint8_t *rawCid)
{
    assert(card);
    assert(rawCid);

    sd_cid_t *cid = &(card->cid);
    cid->manufacturerID = rawCid[0U];
    cid->applicationID = (((uint32_t)rawCid[1U] << 8U) | (uint32_t)(rawCid[2U]));
    memcpy(cid->productName, &rawCid[3U], SD_PRODUCT_NAME_BYTES);
    cid->productVersion = rawCid[8U];
    cid->productSerialNumber = (((uint32_t)rawCid[9U] << 24U) | ((uint32_t)rawCid[10U] << 16U) |
                                ((uint32_t)rawCid[11U] << 8U) | ((uint32_t)rawCid[12U]));
    cid->manufacturerData = ((((uint32_t)rawCid[13U] & 0x0FU) << 8U) | ((uint32_t)rawCid[14U]));
}

SDSPI_ApiRetStatus_Type SDSPI_SendCid(SDSPI_CardHandler_Type *card)
{
    assert(card);
    assert(card->interface);

    uint8_t resp = 0U;

    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_SendCid, 0U, &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }

    if (SDSPI_ApiRetStatus_Success != (SDSPI_Read(card->interface, card->rawCid, sizeof(card->rawCid))))
    {
        return SDSPI_ApiRetStatus_SDSPI_ReadFail;
    }

    SDSPI_DecodeCid(card, card->rawCid);

    return SDSPI_ApiRetStatus_Success;
}

static void SDSPI_DecodeScr(SDSPI_CardHandler_Type *card, uint8_t *rawScr)
{
    assert(card);
    assert(rawScr);

    sd_scr_t *scr = &(card->scr);
    scr->scrStructure = ((rawScr[0U] & 0xF0U) >> 4U);
    scr->sdSpecification = (rawScr[0U] & 0x0FU);
    if (rawScr[1U] & 0x80U)
    {
        scr->flags |= SD_ScrDataStatusAfterErase;
    }
    scr->sdSecurity = ((rawScr[1U] & 0x70U) >> 4U);
    scr->sdBusWidths = (rawScr[1U] & 0x0FU);
    if (rawScr[2U] & 0x80U)
    {
        scr->flags |= SD_ScrSdSpecification3;
    }
    scr->extendedSecurity = ((rawScr[2U] & 0x78U) >> 3U);
    //scr->cmdSupport = (rawScr[3U] & 0x03U);
}

static SDSPI_ApiRetStatus_Type SDSPI_SendScr(SDSPI_CardHandler_Type *card)
{
    assert(card);
    assert(card->interface);

    uint8_t resp = 0U;

    if (SDSPI_ApiRetStatus_Success != SDSPI_SendAppCmd(card))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendAppCmdFail;
    }

    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_SendScr, 0U, &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }

    if (SDSPI_ApiRetStatus_Success != (SDSPI_Read(card->interface, card->rawScr, sizeof(card->rawScr))))
    {
        return SDSPI_ApiRetStatus_SDSPI_ReadFail;
    }

    SDSPI_DecodeScr(card, card->rawScr);

    return SDSPI_ApiRetStatus_Success;
}

static SDSPI_ApiRetStatus_Type SDSPI_StopTrans(SDSPI_CardHandler_Type *card)
{
    uint8_t resp = 0U;

    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_StopTrans, 0U, &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }

    return SDSPI_ApiRetStatus_Success;
}

static SDSPI_ApiRetStatus_Type SDSPI_Write(SDSPI_Interface_Type *interface, uint8_t *buffer, uint32_t size, uint8_t token)
{
    assert(interface);
    assert(interface->spi_xfer);

    uint8_t resp;
    uint16_t timingByte = 0xFFFFU; /* The byte need to be sent as read/write data block timing requirement */

    if (SDSPI_ApiRetStatus_Success != SDSPI_WaitReady(interface))
    {
        return SDSPI_ApiRetStatus_SDSPI_WaitReadyFail;
    }

    /* Write data token. */
    if (interface->spi_xfer(&token, NULL, 1U))
    {
        return SDSPI_ApiRetStatus_SDSPI_XferFail;
    }
    if (token == SDSPI_DataTokenStopTransfer)
    {
        return SDSPI_ApiRetStatus_Success;
    }

    if ((!size) || (!buffer))
    {
        return SDSPI_ApiRetStatus_InvalidArgument;
    }

    /* Write data. */
    if (SDSPI_ApiRetStatus_Success != interface->spi_xfer(buffer, NULL, size))
    {
        return SDSPI_ApiRetStatus_SDSPI_XferFail;
    }

#if SDSPI_CARD_CRC_PROTECTION_ENABLE
    timingByte = SDSPI_GenerateCRC16(buffer, size, 0U);
#endif

    /* Get the last two bytes CRC */
    if (interface->spi_xfer((uint8_t *)&timingByte, NULL, 2U))
    {
        return SDSPI_ApiRetStatus_SDSPI_XferFail;
    }
    /* Get the resp token. */
    if (interface->spi_xfer((uint8_t *)&timingByte, &resp, 1U))
    {
        return SDSPI_ApiRetStatus_SDSPI_XferFail;
    }
    if ((resp & SDSPI_DATA_RESPONSE_TOKEN_MASK) != SDSPI_DataResponseTokenAccepted)
    {
        return SDSPI_ApiRetStatus_SDSPI_ResponseError;
    }

    return SDSPI_ApiRetStatus_Success;
}

static SDSPI_ApiRetStatus_Type SDSPI_Read(SDSPI_Interface_Type *interface, uint8_t *buffer, uint32_t size)
{
    assert(interface);
    assert(interface->spi_xfer);
    assert(buffer);
    assert(size);

    uint8_t resp;
    uint32_t i = SDSPI_TRANSFER_RETRY_TIMES;
    uint16_t timingByte = 0xFFFFU; /* The byte need to be sent as read/write data block timing requirement */
    uint16_t crc = 0U;

    memset(buffer, 0xFFU, size);

    /* Wait data token comming */
    do
    {
        if (SDSPI_ApiRetStatus_Success != interface->spi_xfer((uint8_t *)&timingByte, &resp, 1U))
        {
            return SDSPI_ApiRetStatus_SDSPI_XferFail;
        }

    } while ((resp == 0xFFU) && (--i));

    /* Check data token and spi_xfer data. */
    if (resp != SDSPI_DataTokenBlockRead)
    {
        return SDSPI_ApiRetStatus_SDSPI_ResponseError;
    }
    if (interface->spi_xfer(buffer, buffer, size))
    {
        return SDSPI_ApiRetStatus_SDSPI_XferFail;
    }

    /* Get 16 bit CRC */
    if (SDSPI_ApiRetStatus_Success != interface->spi_xfer((uint8_t *)&timingByte, (uint8_t *)&crc, 2U))
    {
        return SDSPI_ApiRetStatus_SDSPI_XferFail;
    }

    return SDSPI_ApiRetStatus_Success;
}

static SDSPI_ApiRetStatus_Type SDSPI_Erase(SDSPI_CardHandler_Type *card, uint32_t startBlock, uint32_t blockCount)
{
    assert(card);
    assert(blockCount);

    uint8_t resp = 0U;
    uint32_t eraseBlockStart;
    uint32_t eraseBlockEnd;

    if (SDSPI_ApiRetStatus_Success != SDSPI_WaitReady(card->interface))
    {
        return SDSPI_ApiRetStatus_SDSPI_WaitReadyFail;
    }

    eraseBlockStart = (card->cardType & SDSPI_CardType_HighCapacity) == 0U ? (startBlock * card->blockSize) : startBlock;
    eraseBlockEnd = eraseBlockStart + ((card->cardType & SDSPI_CardType_HighCapacity) == 0U ? card->blockSize : 1U) * (blockCount - 1U);

    /* set erase start address */
    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_WriteBlockEraseStart, eraseBlockStart, &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }
    /* set erase end address */
    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_WriteBlockEraseEnd, eraseBlockEnd, &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }
    /* start erase */
    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_WriteBlockErase, 0U, &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }

    return SDSPI_ApiRetStatus_Success;
}

static SDSPI_ApiRetStatus_Type SDSPI_SwitchFunction(
    SDSPI_CardHandler_Type *card, uint32_t mode, uint32_t group, uint32_t number, uint32_t *status)
{
    assert(card);
    assert(status);

    uint8_t resp = 0u;
    uint32_t argument = 0U;

    argument = (mode << 31U | 0x00FFFFFFU);
    argument &= ~((uint32_t)(0xFU) << (group * 4U));
    argument |= (number << (group * 4U));

    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_Switch, argument, &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }

    if (SDSPI_ApiRetStatus_Success != (SDSPI_Read(card->interface, (uint8_t *)status, 64U)))
    {
        return SDSPI_ApiRetStatus_SDSPI_ReadFail;
    }

    return SDSPI_ApiRetStatus_Success;
}

static SDSPI_ApiRetStatus_Type SDSPI_SelectFunction(SDSPI_CardHandler_Type *card, uint32_t group, uint32_t function)
{
    assert(card);

    uint32_t functionStatus[16U] = {0U};
    uint16_t functionGroupInfo[6U] = {0};
    uint32_t currentFunctionStatus = 0U;

    /* check if card support CMD6 */
    if (!(card->csd.cardCommandClass & SDMMC_CommandClassSwitch))
    {
        return SDSPI_ApiRetStatus_SDSPI_NotSupportYet;
    }

    /* Check if card support high speed mode. */
    if (SDSPI_ApiRetStatus_Success != SDSPI_SwitchFunction(card, SD_SwitchCheck, group, function, functionStatus))
    {
        return SDSPI_ApiRetStatus_SDSPI_XferFail;
    }
    /* In little endian mode, SD bus byte transferred first is the byte stored in lowest byte position in
    a word which will cause 4 byte's sequence in a word is not consistent with their original sequence from
    card. So the sequence of 4 bytes received in a word should be converted. */
    functionStatus[0U] = SWAP_WORD_BYTE_SEQUENCE(functionStatus[0U]);
    functionStatus[1U] = SWAP_WORD_BYTE_SEQUENCE(functionStatus[1U]);
    functionStatus[2U] = SWAP_WORD_BYTE_SEQUENCE(functionStatus[2U]);
    functionStatus[3U] = SWAP_WORD_BYTE_SEQUENCE(functionStatus[3U]);
    functionStatus[4U] = SWAP_WORD_BYTE_SEQUENCE(functionStatus[4U]);

    /* -functionStatus[0U]---bit511~bit480;
       -functionStatus[1U]---bit479~bit448;
       -functionStatus[2U]---bit447~bit416;
       -functionStatus[3U]---bit415~bit384;
       -functionStatus[4U]---bit383~bit352;
       According to the "switch function status[bits 511~0]" return by switch cmd in mode "check function":
       -Check if function 1(high speed) in function group 1 is supported by checking if bit 401 is set;
       -check if function 1 is ready and can be switched by checking if bits 379~376 equal value 1;
     */
    functionGroupInfo[5U] = (uint16_t)functionStatus[0U];
    functionGroupInfo[4U] = (uint16_t)(functionStatus[1U] >> 16U);
    functionGroupInfo[3U] = (uint16_t)(functionStatus[1U]);
    functionGroupInfo[2U] = (uint16_t)(functionStatus[2U] >> 16U);
    functionGroupInfo[1U] = (uint16_t)(functionStatus[2U]);
    functionGroupInfo[0U] = (uint16_t)(functionStatus[3U] >> 16U);
    currentFunctionStatus = ((functionStatus[3U] & 0xFFU) << 8U) | (functionStatus[4U] >> 24U);

    /* check if function is support */
    if (((functionGroupInfo[group] & (1 << function)) == 0U) ||
        ((currentFunctionStatus >> (group * 4U)) & 0xFU) != function)
    {
        return SDSPI_ApiRetStatus_SDSPI_NotSupportYet;
    }

    /* Switch to high speed mode. */
    if (SDSPI_ApiRetStatus_Success != SDSPI_SwitchFunction(card, SD_SwitchSet, group, function, functionStatus))
    {
        return SDSPI_ApiRetStatus_SDSPI_XferFail;
    }
    /* In little endian mode is little endian, SD bus byte transferred first is the byte stored in lowest byte
    position in a word which will cause 4 byte's sequence in a word is not consistent with their original
    sequence from card. So the sequence of 4 bytes received in a word should be converted. */
    functionStatus[3U] = SWAP_WORD_BYTE_SEQUENCE(functionStatus[3U]);
    functionStatus[4U] = SWAP_WORD_BYTE_SEQUENCE(functionStatus[4U]);

    /* According to the "switch function status[bits 511~0]" return by switch cmd in mode "set function":
       -check if group 1 is successfully changed to function 1 by checking if bits 379~376 equal value 1;
     */
    currentFunctionStatus = ((functionStatus[3U] & 0xFFU) << 8U) | (functionStatus[4U] >> 24U);

    if (((currentFunctionStatus >> (group * 4U)) & 0xFU) != function)
    {
        return SDSPI_ApiRetStatus_SDSPI_SwitchCmdFail;
    }

    return SDSPI_ApiRetStatus_Success;
}

SDSPI_ApiRetStatus_Type SDSPI_Init(SDSPI_CardHandler_Type *card, const SDSPI_Interface_Type *interface)
{
    assert(card);
    assert(interface);
    assert(interface);
    assert(interface->spi_init);
    assert(interface->spi_freq);
    assert(interface->spi_xfer);

    card->interface = (SDSPI_Interface_Type *)interface;

    if (interface->spi_init())
    {
        return SDSPI_ApiRetStatus_SDSPI_SpiInitFail;
    }

    uint32_t applicationCommand41Argument = 0U;

    /* Card must be initialized in 400KHZ. */
    if (card->interface->spi_freq(SDMMC_CLOCK_400KHZ))
    {
        return SDSPI_ApiRetStatus_SDSPI_SetFreqFail;
    }

    uint32_t i = 0U;
    uint8_t test = 0xFFU;
    for (i = 0U; i < 10; i++)
    {
      if (card->interface->spi_xfer(NULL, &test, 1))
      {
          return SDSPI_ApiRetStatus_SDSPI_XferFail;
      }
    }

    /* Reset the card by CMD0. */
    if (SDSPI_ApiRetStatus_Success != SDSPI_GoIdle(card))
    {
        return SDSPI_ApiRetStatus_SDSPI_GoIdleFail;
    }

    /* Check the card's supported interface condition. */
    if (SDSPI_ApiRetStatus_Success != SDSPI_SendIfCond(card, &applicationCommand41Argument))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendIfCondFail;
    }

#if SDSPI_CARD_CRC_PROTECTION_ENABLE
    /* enable cmd crc protection. */
    if (SDSPI_ApiRetStatus_Success != SDSPI_EnableCmdCrc(card, true))
    {
        return SDSPI_ApiRetStatus_SDSPI_SwitchCmdFail;
    }
#endif

    /* Set card's interface condition according to interface's capability and card's supported interface condition */
    if (SDSPI_ApiRetStatus_Success != SDSPI_AppSendOpCond(card, applicationCommand41Argument))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendOpCondFail;
    }

    if (SDSPI_ApiRetStatus_Success != SDSPI_ReadOcr(card))
    {
        return SDSPI_ApiRetStatus_SDSPI_ReadOcrFail;
    }

    /* Force to use 512-byte length block, no matter which version.  */
    if (SDSPI_ApiRetStatus_Success != SDSPI_SetBlockSize(card, SDSPI_DEFAULT_BLOCK_SIZE)) /* 512u. */
    {
        return SDSPI_ApiRetStatus_SDSPI_SetBlockSizeFail;
    }
    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCsd(card))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCsdFail;
    }

    /* Set to max frequency according to the max frequency information in CSD register. */
    if (SDSPI_ApiRetStatus_Success !=
        card->interface->spi_freq(SD_CLOCK_25MHZ > card->interface->baudrate ? card->interface->baudrate : SD_CLOCK_25MHZ))
    {
        return SDSPI_ApiRetStatus_SDSPI_SetFreqFail;
    }

    if (SDSPI_ApiRetStatus_Success != SDSPI_SendScr(card))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCsdFail;
    }

    return SDSPI_ApiRetStatus_Success;
}

void SDSPI_Deinit(SDSPI_CardHandler_Type *card)
{
    assert(card);

    memset(card, 0, sizeof(SDSPI_CardHandler_Type));
}

bool SDSPI_CheckReadOnly(SDSPI_CardHandler_Type *card)
{
    assert(card);

    if ((card->csd.flags & SD_CsdPermanentWriteProtectFlag) || (card->csd.flags & SD_CsdTemporaryWriteProtectFlag))
    {
        return true;
    }

    return false;
}

SDSPI_ApiRetStatus_Type SDSPI_ReadBlocks(SDSPI_CardHandler_Type *card, uint8_t *buffer, uint32_t startBlock, uint32_t blockCount)
{
    assert(card);
    assert(card->interface);
    assert(buffer);
    assert(blockCount);

    uint32_t i;
    uint8_t resp = 0U;

    /* send cmd */
    if (SDSPI_ApiRetStatus_Success !=
        SDSPI_SendCmd(
            card->interface, blockCount == 1U ? SDSPI_Cmd_ReadSigleBlock : SDSPI_Cmd_ReadMultiBlock,
            ((card->cardType & SDSPI_CardType_HighCapacity) == 0U ? (startBlock * card->blockSize) : startBlock),
            &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }
    /* read data */
    for (i = 0U; i < blockCount; i++)
    {
        if (SDSPI_ApiRetStatus_Success != SDSPI_Read(card->interface, buffer, card->blockSize))
        {
            return SDSPI_ApiRetStatus_SDSPI_ReadFail;
        }
        buffer += card->blockSize;
    }

    /* Write stop transmission cmd after the last data block. */
    if (blockCount > 1U)
    {
        if (SDSPI_ApiRetStatus_Success != SDSPI_StopTrans(card))
        {
            return SDSPI_ApiRetStatus_SDSPI_StopTransFail;
        }
    }

    return SDSPI_ApiRetStatus_Success;
}

SDSPI_ApiRetStatus_Type SDSPI_WriteBlocks(SDSPI_CardHandler_Type *card, uint8_t *buffer, uint32_t startBlock, uint32_t blockCount)
{
    assert(card);
    assert(card->interface);
    assert(buffer);
    assert(blockCount);

    uint32_t i;
    uint8_t resp = 0U;

    if (SDSPI_CheckReadOnly(card))
    {
        return SDSPI_ApiRetStatus_SDSPI_WriteProtected;
    }

    /* send cmd */
    if (SDSPI_ApiRetStatus_Success !=
        SDSPI_SendCmd(
            card->interface, blockCount == 1U ? SDSPI_Cmd_WriteSigleBlock : SDSPI_Cmd_WriteMultiBlock,
            ((card->cardType & SDSPI_CardType_HighCapacity) == 0U ? (startBlock * card->blockSize) : startBlock),
            &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }
    /* check resp */
    if (resp)
    {
        return SDSPI_ApiRetStatus_SDSPI_ResponseError;
    }
    /* write data */
    for (i = 0U; i < blockCount; i++)
    {
        if (SDSPI_ApiRetStatus_Success !=
            SDSPI_Write(card->interface, buffer, card->blockSize,
                        blockCount == 1U ? SDSPI_DataTokenSingleBlockWrite : SDSPI_DataTokenMultipleBlockWrite))
        {
            return SDSPI_ApiRetStatus_SDSPI_WriteFail;
        }
        buffer += card->blockSize;
    }
    /* stop transfer */
    if (blockCount > 1U)
    {
        if (SDSPI_ApiRetStatus_Success != SDSPI_Write(card->interface, 0U, 0U, SDSPI_DataTokenStopTransfer))
        {
            return SDSPI_ApiRetStatus_SDSPI_WriteFail;
        }

        /* Wait the card programming end. */
        if (SDSPI_ApiRetStatus_Success != SDSPI_WaitReady(card->interface))
        {
            return SDSPI_ApiRetStatus_SDSPI_WaitReadyFail;
        }
    }

    return SDSPI_ApiRetStatus_Success;
}

SDSPI_ApiRetStatus_Type SDSPI_EraseBlocksPre(SDSPI_CardHandler_Type *card, uint32_t blockCount)
{
    assert(blockCount > 1U);

    uint8_t resp = 0U;

    /* Pre-erase before writing data */
    if (SDSPI_ApiRetStatus_Success != SDSPI_SendAppCmd(card))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendAppCmdFail;
    }

    if (SDSPI_ApiRetStatus_Success != SDSPI_SendCmd(card->interface, SDSPI_Cmd_WriteBlockEraseCount, blockCount, &resp))
    {
        return SDSPI_ApiRetStatus_SDSPI_SendCmdFail;
    }

    if (resp)
    {
        return SDSPI_ApiRetStatus_SDSPI_ResponseError;
    }

    return SDSPI_ApiRetStatus_Success;
}

SDSPI_ApiRetStatus_Type SDSPI_EraseBlocks(SDSPI_CardHandler_Type *card, uint32_t startBlock, uint32_t blockCount)
{
    assert(card);
    assert(blockCount);

    uint32_t blockCountOneTime; /* The block count can be erased in one time sending ERASE_BLOCKS cmd. */
    uint32_t blockDone = 0U;    /* The block count has been erased. */
    uint32_t blockLeft;         /* Left block count to be erase. */

    blockLeft = blockCount;
    while (blockLeft)
    {
        if (blockLeft > (card->csd.eraseSectorSize + 1U))
        {
            blockCountOneTime = card->csd.eraseSectorSize + 1U;
            blockLeft = blockLeft - blockCountOneTime;
        }
        else
        {
            blockCountOneTime = blockLeft;
            blockLeft = 0U;
        }

        if (SDSPI_Erase(card, (startBlock + blockDone), blockCountOneTime) != SDSPI_ApiRetStatus_Success)
        {
            return SDSPI_ApiRetStatus_Fail;
        }

        blockDone += blockCountOneTime;
    }

    return SDSPI_ApiRetStatus_Success;
}

SDSPI_ApiRetStatus_Type SDSPI_SwitchToHighSpeed(SDSPI_CardHandler_Type *card)
{
    assert(card);

    if (SDSPI_SelectFunction(card, SD_GroupTimingMode, SD_FunctionSDR25HighSpeed) == SDSPI_ApiRetStatus_Success)
    {
        card->interface->spi_freq(SD_CLOCK_50MHZ > card->interface->baudrate ? card->interface->baudrate : SD_CLOCK_50MHZ);

        return SDSPI_ApiRetStatus_Success;
    }

    return SDSPI_ApiRetStatus_Fail;
}

/* EOF. */

