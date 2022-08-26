#ifndef __WAV_H_
#define __WAV_H_

#include "hal_common.h"

#define WAV_BUFFER_SIZE   (5 * 1024)

/* Exported types : RIFF Chunk -----------------------------------------------*/
typedef struct
{
    uint32_t ChunkID;       /* ����̶�Ϊ"RIFF",��0x46464952 */

    uint32_t ChunkSize ;    /* ���ϴ�С;�ļ��ܴ�С-8 */

    uint32_t Format;        /* ��ʽ;WAVE,��0x45564157 */

} ChunkRIFF_TypeDef;


/* Exported types : FMT Chunk ------------------------------------------------*/
typedef struct
{
    uint32_t ChunkID;       /* ����̶�Ϊ"fmt ",��0x20746D66 */

    uint32_t ChunkSize ;    /* �Ӽ��ϴ�С(������ID��Size);����Ϊ:20 */

    uint16_t AudioFormat;   /* ��Ƶ��ʽ;0x01,��ʾ����PCM;0x11��ʾIMA ADPCM */

    uint16_t NumOfChannels; /* ͨ������;1,��ʾ������;2,��ʾ˫���� */

    uint32_t SampleRate;    /* ������;0x1F40,��ʾ8Khz */

    uint32_t ByteRate;      /* �ֽ����� */ 

    uint16_t BlockAlign;    /* �����(�ֽ�) */

    uint16_t BitsPerSample; /* �����������ݴ�С;4λADPCM,����Ϊ4 */

//  uint16_t ByteExtraData; /* ���ӵ������ֽ�;2��; ����PCM,û��������� */

}ChunkFMT_TypeDef;


/* Exported types : FACT Chunk -----------------------------------------------*/
typedef struct 
{
    uint32_t ChunkID;       /* ����̶�Ϊ"fact",��0x74636166 */

    uint32_t ChunkSize;     /* �Ӽ��ϴ�С(������ID��Size);����Ϊ:4 */

    uint32_t NumOfSamples;  /* ����������; */

}ChunkFACT_TypeDef;


/* Exported types : LIST Chunk -----------------------------------------------*/
typedef struct 
{
    uint32_t ChunkID;       /* ����̶�Ϊ"LIST",��0x74636166 */

    uint32_t ChunkSize;     /* �Ӽ��ϴ�С(������ID��Size);����Ϊ:4 */

}ChunkLIST_TypeDef;



/* Exported types : DATA Chunk -----------------------------------------------*/
typedef struct 
{
    uint32_t ChunkID;       /* ����̶�Ϊ"data",��0x5453494C */

    uint32_t ChunkSize;     /* �Ӽ��ϴ�С(������ID��Size) */

}ChunkDATA_TypeDef;


/* Exported types : WAVE Header ----------------------------------------------*/
typedef struct
{ 
    ChunkRIFF_TypeDef ChunkRIFF;    /* RIFF�� */

    ChunkFMT_TypeDef  ChunkFMT;     /* FMT�� */

//  ChunkFACT_TypeDef ChunkFACT;    /* FACT�� ����PCM,û������ṹ�� */

    ChunkDATA_TypeDef ChunkDATA;    /* DATA�� */
}WaveHeader_TypeDef;


/* Exported types : WAVE Play Control ----------------------------------------*/
typedef struct
{ 
    uint16_t AudioFormat;       /* ��Ƶ��ʽ;0X01,��ʾ����PCM;0X11��ʾIMA ADPCM */
    uint16_t nChannels;         /* ͨ������;1,��ʾ������;2,��ʾ˫���� */
    uint16_t BlockAlign;        /* �����(�ֽ�) */
    uint32_t DataSize;          /* WAV���ݴ�С */

    uint32_t Totalsecond;       /* ���׸�ʱ��,��λ:�� */
    uint32_t CurrentSecond;     /* ��ǰ����ʱ�� */

    uint32_t BitRate;           /* ������(λ��) */
    uint32_t SampleRate;        /* ������ */
    uint16_t BitsPerSample;     /* λ��,bsp,����16bit,24bit,32bit */

    uint32_t DataStart;         /* ����֡��ʼ��λ��(���ļ������ƫ��) */
} WAV_TypeDef; 

typedef enum {
    I2S_AudioFreq_192k               = (192000),
    I2S_AudioFreq_96k                = (96000),
    I2S_AudioFreq_48k                = (48000),
    I2S_AudioFreq_44k                = (44100),
    I2S_AudioFreq_32k                = (32000),
    I2S_AudioFreq_24k                = (24000),
    I2S_AudioFreq_22k                = (22050),
    I2S_AudioFreq_16k                = (16000),
    I2S_AudioFreq_11k                = (11025),
    I2S_AudioFreq_12k                = (12000),
    I2S_AudioFreq_8k                 = (8000),
    I2S_AudioFreq_4k                 = (4000),
    I2S_AudioFreq_Default            = (2),
} SPI_I2S_AUDIO_FREQ_TypeDef;

extern void WAV_PrepareData(void);
extern void WAV_PlayHandler(void);
extern void WAV_PlaySong(char *Path, char *Name);


#endif
