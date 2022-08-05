/* sdspi.h */

#ifndef __SDSPI_H__
#define __SDSPI_H__

#include <stdint.h>
#include "sdmmc_spec.h"

/******************************************************************************
 * Definitions
 *****************************************************************************/

#define SDSPI_DEFAULT_BLOCK_SIZE (512U)

#define SDSPI_DUMMY_DATA   (0xff)

#ifndef SDSPI_CARD_CRC_PROTECTION_ENABLE
#define SDSPI_CARD_CRC_PROTECTION_ENABLE 0U
#endif
/*!
 * @addtogroup SDSPI
 * @{
 */

/*! @brief SDSPI API status */
typedef enum
{
    SDSPI_ApiRetStatus_Success = 0,
    SDSPI_ApiRetStatus_InvalidArgument,
    SDSPI_ApiRetStatus_Fail,
    SDSPI_ApiRetStatus_SDSPI_SetFreqFail,      /*!< Set frequency failed */
    SDSPI_ApiRetStatus_SDSPI_XferFail,         /*!< Exchange data on SPI bus failed */
    SDSPI_ApiRetStatus_SDSPI_WaitReadyFail,    /*!< Wait card ready failed */
    SDSPI_ApiRetStatus_SDSPI_ResponseError,    /*!< Response is error */
    SDSPI_ApiRetStatus_SDSPI_WriteProtected,   /*!< Write protected */
    SDSPI_ApiRetStatus_SDSPI_GoIdleFail,       /*!< Go idle failed */
    SDSPI_ApiRetStatus_SDSPI_SendCmdFail,      /*!< Send command failed */
    SDSPI_ApiRetStatus_SDSPI_ReadFail,         /*!< Read data failed */
    SDSPI_ApiRetStatus_SDSPI_WriteFail,        /*!< Write data failed */
    SDSPI_ApiRetStatus_SDSPI_SendIfCondFail,   /*!< Send interface condition failed */
    SDSPI_ApiRetStatus_SDSPI_SendOpCondFail,   /*!< Send operation condition failed */
    SDSPI_ApiRetStatus_SDSPI_ReadOcrFail,      /*!< Read OCR failed */
    SDSPI_ApiRetStatus_SDSPI_SetBlockSizeFail, /*!< Set block size failed */
    SDSPI_ApiRetStatus_SDSPI_SendCsdFail,      /*!< Send CSD failed */
    SDSPI_ApiRetStatus_SDSPI_SendCidFail,      /*!< Send CID failed */
    SDSPI_ApiRetStatus_SDSPI_StopTransFail,    /*!< Stop transmission failed */
    SDSPI_ApiRetStatus_SDSPI_SendAppCmdFail,   /*!< Send application command failed */
    SDSPI_ApiRetStatus_SDSPI_InvalidVoltage,   /*!< invaild supply voltage */
    SDSPI_ApiRetStatus_SDSPI_SwitchCmdFail,    /*!< switch command crc protection on/off */
    SDSPI_ApiRetStatus_SDSPI_NotSupportYet,    /*!< not support */
    SDSPI_ApiRetStatus_SDSPI_SpiInitFail,      /*!< not support */
} SDSPI_ApiRetStatus_Type;

/*! @brief SDSPI card flag */
enum _sdspi_card_type
{
    SDSPI_CardType_HighCapacity = (1U << 0U), /*!< Card is high capacity */
    SDSPI_CardType_Sdhc = (1U << 1U),         /*!< Card is SDHC */
    SDSPI_CardType_Sdxc = (1U << 2U),         /*!< Card is SDXC */
    SDSPI_CardType_Sdsc = (1U << 3U),         /*!< Card is SDSC */
};

/*! @brief SDSPI response type */
enum _sdspi_resp_type
{
    SDSPI_RespType_R1  = 0U,  /*!< Response 1 */
    SDSPI_RespType_R1b = 1U,  /*!< Response 1 with busy */
    SDSPI_RespType_R2  = 2U,  /*!< Response 2 */
    SDSPI_RespType_R3  = 3U,  /*!< Response 3 */
    SDSPI_RespType_R7  = 4U,  /*!< Response 7 */
};

#define SDSPI_MAKE_CMD(resp, cmd)  ( (resp) | ((cmd) << 8u) )
enum _sdspi_cmd
{
    SDSPI_Cmd_GoIdle               = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SDMMC_GoIdleState),
    SDSPI_Cmd_Crc                  = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SDSPI_CmdCrc),
    SDSPI_Cmd_SendIfCond           = SDSPI_MAKE_CMD(SDSPI_RespType_R7 , SD_SendInterfaceCondition),
    SDSPI_Cmd_AppCmd               = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SDMMC_ApplicationCommand),
    SDSPI_Cmd_AppSendOpCond        = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SD_ApplicationSendOperationCondition),
    SDSPI_Cmd_ReadOcr              = SDSPI_MAKE_CMD(SDSPI_RespType_R3 , SDMMC_ReadOcr),
    SDSPI_Cmd_SetBlockLength       = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SDMMC_SetBlockLength),
    SDSPI_Cmd_SendCsd              = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SDMMC_SendCsd),
    SDSPI_Cmd_SendCid              = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SDMMC_SendCid),
    SDSPI_Cmd_SendScr              = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SD_ApplicationSendScr),
    SDSPI_Cmd_StopTrans            = SDSPI_MAKE_CMD(SDSPI_RespType_R1b, SDMMC_StopTransmission),
    SDSPI_Cmd_WriteSigleBlock      = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SDMMC_WriteSingleBlock),
    SDSPI_Cmd_WriteMultiBlock      = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SDMMC_WriteMultipleBlock),
    SDSPI_Cmd_ReadSigleBlock       = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SDMMC_ReadSingleBlock),
    SDSPI_Cmd_ReadMultiBlock       = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SDMMC_ReadMultipleBlock),
    SDSPI_Cmd_WriteBlockEraseCount = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SD_ApplicationSetWriteBlockEraseCount),
    SDSPI_Cmd_WriteBlockEraseStart = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SD_EraseWriteBlockStart),
    SDSPI_Cmd_WriteBlockEraseEnd   = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SD_EraseWriteBlockEnd),
    SDSPI_Cmd_WriteBlockErase      = SDSPI_MAKE_CMD(SDSPI_RespType_R1b, SDMMC_Erase),
    SDSPI_Cmd_Switch               = SDSPI_MAKE_CMD(SDSPI_RespType_R1 , SD_Switch),
};

typedef struct _sdspi_interface
{
    uint32_t baudrate; /*!< Bus baud rate */
    SDSPI_ApiRetStatus_Type (*spi_init)(void);        /*!< init spi hardware. */
    SDSPI_ApiRetStatus_Type (*spi_freq)(uint32_t hz); /*!< Set frequency of SPI */
    SDSPI_ApiRetStatus_Type (*spi_xfer)(uint8_t *in, uint8_t *out, uint32_t size); /*!< Exchange data over SPI */
} SDSPI_Interface_Type;

typedef struct _sdspi_card
{
    SDSPI_Interface_Type *interface;       /*!< interface state information */
    uint32_t relativeAddress; /*!< Relative address of the card */
    uint32_t cardType;        /*!< Flags defined in _sdspi_card_type. */
    uint8_t rawCid[16U];      /*!< Raw CID content */
    uint8_t rawCsd[16U];      /*!< Raw CSD content */
    uint8_t rawScr[8U];       /*!< Raw SCR content */
    uint32_t ocr;             /*!< Raw OCR content */
    sd_cid_t cid;             /*!< CID */
    sd_csd_t csd;             /*!< CSD */
    sd_scr_t scr;             /*!< SCR */
    uint32_t blockCount;      /*!< Card total block number */
    uint32_t blockSize;       /*!< Card block size */
} SDSPI_CardHandler_Type;

/*************************************************************************************************
 * API
 ************************************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

SDSPI_ApiRetStatus_Type SDSPI_Init(SDSPI_CardHandler_Type *card, const SDSPI_Interface_Type *interface);
SDSPI_ApiRetStatus_Type SDSPI_ReadBlocks(SDSPI_CardHandler_Type *card, uint8_t *buffer, uint32_t startBlock, uint32_t blockCount);
SDSPI_ApiRetStatus_Type SDSPI_WriteBlocks(SDSPI_CardHandler_Type *card, uint8_t *buffer, uint32_t startBlock, uint32_t blockCount);
SDSPI_ApiRetStatus_Type SDSPI_SendCid(SDSPI_CardHandler_Type *card);
SDSPI_ApiRetStatus_Type SDSPI_EraseBlocksPre(SDSPI_CardHandler_Type *card, uint32_t blockCount);
SDSPI_ApiRetStatus_Type SDSPI_EraseBlocks(SDSPI_CardHandler_Type *card, uint32_t startBlock, uint32_t blockCount);
SDSPI_ApiRetStatus_Type SDSPI_SwitchToHighSpeed(SDSPI_CardHandler_Type *card);

void SDSPI_Deinit(SDSPI_CardHandler_Type *card);
bool SDSPI_CheckReadOnly(SDSPI_CardHandler_Type *card);

#if SDSPI_CARD_CMD_CRC_PROTECTION_ENABLE
SDSPI_ApiRetStatus_Type SDSPI_EnableCmdCrc(SDSPI_CardHandler_Type *card, bool enable);
#endif

extern const char * sdspi_card_type_str[];

#if defined(__cplusplus)
}
#endif

#endif /* __SDSPI_H__ */
