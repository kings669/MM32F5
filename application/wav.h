#ifndef __WAV_H_
#define __WAV_H_

#include "hal_common.h"

#define WAV_BUFFER_SIZE   (5 * 1024)

/* Exported types : RIFF Chunk -----------------------------------------------*/
typedef struct
{
    uint32_t ChunkID;       /* 这里固定为"RIFF",即0x46464952 */

    uint32_t ChunkSize ;    /* 集合大小;文件总大小-8 */

    uint32_t Format;        /* 格式;WAVE,即0x45564157 */

} ChunkRIFF_TypeDef;


/* Exported types : FMT Chunk ------------------------------------------------*/
typedef struct
{
    uint32_t ChunkID;       /* 这里固定为"fmt ",即0x20746D66 */

    uint32_t ChunkSize ;    /* 子集合大小(不包括ID和Size);这里为:20 */

    uint16_t AudioFormat;   /* 音频格式;0x01,表示线性PCM;0x11表示IMA ADPCM */

    uint16_t NumOfChannels; /* 通道数量;1,表示单声道;2,表示双声道 */

    uint32_t SampleRate;    /* 采样率;0x1F40,表示8Khz */

    uint32_t ByteRate;      /* 字节速率 */ 

    uint16_t BlockAlign;    /* 块对齐(字节) */

    uint16_t BitsPerSample; /* 单个采样数据大小;4位ADPCM,设置为4 */

//  uint16_t ByteExtraData; /* 附加的数据字节;2个; 线性PCM,没有这个参数 */

}ChunkFMT_TypeDef;


/* Exported types : FACT Chunk -----------------------------------------------*/
typedef struct 
{
    uint32_t ChunkID;       /* 这里固定为"fact",即0x74636166 */

    uint32_t ChunkSize;     /* 子集合大小(不包括ID和Size);这里为:4 */

    uint32_t NumOfSamples;  /* 采样的数量; */

}ChunkFACT_TypeDef;


/* Exported types : LIST Chunk -----------------------------------------------*/
typedef struct 
{
    uint32_t ChunkID;       /* 这里固定为"LIST",即0x74636166 */

    uint32_t ChunkSize;     /* 子集合大小(不包括ID和Size);这里为:4 */

}ChunkLIST_TypeDef;



/* Exported types : DATA Chunk -----------------------------------------------*/
typedef struct 
{
    uint32_t ChunkID;       /* 这里固定为"data",即0x5453494C */

    uint32_t ChunkSize;     /* 子集合大小(不包括ID和Size) */

}ChunkDATA_TypeDef;


/* Exported types : WAVE Header ----------------------------------------------*/
typedef struct
{ 
    ChunkRIFF_TypeDef ChunkRIFF;    /* RIFF块 */

    ChunkFMT_TypeDef  ChunkFMT;     /* FMT块 */

//  ChunkFACT_TypeDef ChunkFACT;    /* FACT块 线性PCM,没有这个结构体 */

    ChunkDATA_TypeDef ChunkDATA;    /* DATA块 */
}WaveHeader_TypeDef;


/* Exported types : WAVE Play Control ----------------------------------------*/
typedef struct
{ 
    uint16_t AudioFormat;       /* 音频格式;0X01,表示线性PCM;0X11表示IMA ADPCM */
    uint16_t nChannels;         /* 通道数量;1,表示单声道;2,表示双声道 */
    uint16_t BlockAlign;        /* 块对齐(字节) */
    uint32_t DataSize;          /* WAV数据大小 */

    uint32_t Totalsecond;       /* 整首歌时长,单位:秒 */
    uint32_t CurrentSecond;     /* 当前播放时长 */

    uint32_t BitRate;           /* 比特率(位速) */
    uint32_t SampleRate;        /* 采样率 */
    uint16_t BitsPerSample;     /* 位数,bsp,比如16bit,24bit,32bit */

    uint32_t DataStart;         /* 数据帧开始的位置(在文件里面的偏移) */
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
