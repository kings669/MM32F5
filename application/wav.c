#include "wav.h"
#include "i2s_port.h"

FIL     WAV_File;
UINT    WAV_BR[2];
FRESULT WAV_RES;

DTCM_RAM uint8_t WAV_DataBuffer[2][WAV_BUFFER_SIZE];
uint8_t WAV_NextIndex = 0;
uint8_t WAV_PlayEnded = 0;

uint32_t WAV_PlaybackTotal     = 0;
uint32_t WAV_PlaybackProgress  = 0;

static uint8_t WAV_DecodeFile(WAV_TypeDef *pWav, char *Path, char *Name)
{
    static ChunkRIFF_TypeDef *WAV_RIFF;
    static ChunkFMT_TypeDef  *WAV_FMT ;
    static ChunkFACT_TypeDef *WAV_FACT;
    static ChunkDATA_TypeDef *WAV_DATA;

		static uint8_t WAV_HeadBuffer[512];

    static uint8_t Result = 0;
    static char FilePath[ 100];

    memset( FilePath, 0x00, sizeof(FilePath));
    sprintf(FilePath, "%s%s",   Path,   Name);
		printf("%s\r\n",FilePath);
    WAV_RES = f_open(&WAV_File, FilePath, FA_READ);

    if(WAV_RES == FR_OK)
    {
        /* ��ȡ512�ֽ������� */
        WAV_RES = f_read(&WAV_File, WAV_HeadBuffer, 512, &WAV_BR[0]);

        if((WAV_RES == FR_OK) && (WAV_BR[0] != 0))
        {
            /* ��ȡRIFF�� */
            WAV_RIFF = (ChunkRIFF_TypeDef *)WAV_HeadBuffer;

            /* ��WAV��ʽ�ļ� */
            if(WAV_RIFF->Format == 0x45564157)
            {
                /* ��ȡFMT�� */
                WAV_FMT  = (ChunkFMT_TypeDef *)(WAV_HeadBuffer+12);

                /* ��ȡFACT�� */
                WAV_FACT = (ChunkFACT_TypeDef *)(WAV_HeadBuffer+12+8+WAV_FMT->ChunkSize);

                if((WAV_FACT->ChunkID == 0x74636166) || (WAV_FACT->ChunkID == 0x5453494C))
                {
                    /* ����FACT/LIST���ʱ��(δ����) */
                    pWav->DataStart=12+8+WAV_FMT->ChunkSize+8+WAV_FACT->ChunkSize;
                }
                else
                {
                    pWav->DataStart=12+8+WAV_FMT->ChunkSize;
                }

                /* ��ȡDATA�� */
                WAV_DATA = (ChunkDATA_TypeDef *)(WAV_HeadBuffer+pWav->DataStart);

                /* �����ɹ� */
                if(WAV_DATA->ChunkID == 0x61746164)
                {
                    pWav->AudioFormat   = WAV_FMT->AudioFormat;     /* ��Ƶ��ʽ */
                    pWav->nChannels     = WAV_FMT->NumOfChannels;   /* ͨ���� */
                    pWav->SampleRate    = WAV_FMT->SampleRate;      /* ������ */
                    pWav->BitRate       = WAV_FMT->ByteRate*8;      /* �õ�λ�� */
                    pWav->BlockAlign    = WAV_FMT->BlockAlign;      /* ����� */
                    pWav->BitsPerSample = WAV_FMT->BitsPerSample;   /* λ��,16/24/32λ */

                    pWav->DataSize      = WAV_DATA->ChunkSize;      /* ���ݿ��С */
                    pWav->DataStart     = pWav->DataStart+8;        /* ��������ʼ�ĵط� */

                    printf("\r\npWav->AudioFormat   : %d", pWav->AudioFormat);
                    printf("\r\npWav->nChannels     : %d", pWav->nChannels);
                    printf("\r\npWav->SampleRate    : %d", pWav->SampleRate);
                    printf("\r\npWav->BitRate       : %d", pWav->BitRate);
                    printf("\r\npWav->BlockAlign    : %d", pWav->BlockAlign);
                    printf("\r\npWav->BitsPerSample : %d", pWav->BitsPerSample);
                    printf("\r\npWav->DataSize      : %d", pWav->DataSize);
                    printf("\r\npWav->DataStart     : %d", pWav->DataStart);

                    WAV_PlaybackTotal = pWav->DataSize;
                }
                else
                {
                    Result = 3; /* DATA����δ�ҵ� */
                }
            }
            else
            {
                Result = 2;     /* ��WAV��ʽ�ļ� */
            }
        }

       f_close(&WAV_File);
    }
    else
    {
        Result = 1;             /* ���ļ����� */
    }

    printf("\r\n\r\nWAV Decode File Result : %d\r\n\r\n", Result);

    return Result;
}

void WAV_PrepareData(void)
{
    if(WAV_NextIndex == 0)
    {
        WAV_RES = f_read(&WAV_File, WAV_DataBuffer[WAV_NextIndex], WAV_BUFFER_SIZE, &WAV_BR[WAV_NextIndex]);
        if(WAV_BR[0] == 0) WAV_PlayEnded = 1;
    }
    else
    {
        WAV_RES = f_read(&WAV_File, WAV_DataBuffer[WAV_NextIndex], WAV_BUFFER_SIZE, &WAV_BR[WAV_NextIndex]);
        if(WAV_BR[1] == 0) WAV_PlayEnded = 1;
    }
}

void WAV_PlayHandler(void)
{
    if(WAV_PlayEnded == 0)
    {
        WAV_PlaybackProgress += WAV_BR[WAV_NextIndex];

        I2S_DMA_Transfer((uint16_t *)&WAV_DataBuffer[WAV_NextIndex][0], (WAV_BR[WAV_NextIndex] /2));
			
    }
    else
    {
        DMA_EnableChannel(DMA1,DMA_REQ_DMA1_SPI2_TX,false);
        f_close(&WAV_File); 
				I2S_PowerON(0);
        printf("\r\nWAV Play Finish!\r\n");
    }
//WAV_NextIndex = 1;
    if(WAV_NextIndex == 0) WAV_NextIndex = 1;
    else                   WAV_NextIndex = 0;
}

void WAV_PlaySong(char *Path, char *Name)
{
    WAV_TypeDef WaveFile;
    char FilePath[100];

    /* ��ȡWAV�ļ�����Ϣ */
    if(WAV_DecodeFile(&WaveFile, Path, Name) == 0)
    {
        if((WaveFile.BitsPerSample == 16) && (WaveFile.nChannels == 2) &&
           (WaveFile.SampleRate  > 44000) && (WaveFile.SampleRate < 48100))
        {
					I2S_PowerON(1);
					I2S_Configure(I2S_Protocol_PHILIPS, I2S_DataWidth_16b,I2S_AudioFreq_44k, I2S_XferMode_TxOnly);
        }
        else
        {
            printf("\r\nWAV File Error!\r\n");  return;
        }
    }
    else
    {
        printf("\r\nNot WAV File!\r\n");    return;
    }

    memset( FilePath, 0x00, sizeof(FilePath));
    sprintf(FilePath, "%s%s",   Path,   Name);
		printf(">f_open WAV Start...\r\n");
    WAV_RES = f_open(&WAV_File, FilePath, FA_READ);

    if(WAV_RES == FR_OK)
    {
				printf(">f_open WAV Done!\r\n");
        WAV_NextIndex = 0;
        WAV_PlayEnded = 0;

        WAV_PlaybackProgress = 0;

        WAV_PrepareData();
        WAV_PlayHandler();
    }
    else
    {
        printf("\r\nWAV File Open Error : %d", WAV_RES);
    }
}

