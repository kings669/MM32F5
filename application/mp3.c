/* Includes ------------------------------------------------------------------*/
#include "mp3.h"
#include "mad.h"
#include "i2s_port.h"
#include "scheduler.h"

#define MP3_LIBMAD_I_BUFFER_SIZE    (10 * 1024)
#define MP3_LIBMAD_O_BUFFER_SIZE    (5  * 1024)

FIL     MP3_libmad_File;
UINT    MP3_libmad_BR;
FRESULT MP3_libmad_RES;

DTCM_RAM unsigned short MP3_libmad_oBuffer[2][MP3_LIBMAD_O_BUFFER_SIZE];
unsigned char  MP3_libmad_iBuffer[MP3_LIBMAD_I_BUFFER_SIZE+MAD_BUFFER_GUARD];

uint8_t  MP3_libmad_NextIndex  = 0;
uint8_t  MP3_libmad_PlayEnded  = 0;
uint32_t MP3_libmad_SampleRate = 0;
uint32_t MP3_libmad_BufferSize = 0;

struct mad_stream MP3_libmad_Stream;
struct mad_frame  MP3_libmad_Frame;
struct mad_synth  MP3_libmad_Synth;
mad_timer_t       MP3_libmad_Timer;

extern uint8_t I2S_DMA_Finish;


/*******************************************************************************
 * @brief       
 * @param       
 * @retval      
 * @attention   
*******************************************************************************/
const char *MP3_libmad_MadErrorString(const struct mad_stream *Stream)
{
    switch(Stream->error)
    {
        /* Generic unrecoverable errors */
        case MAD_ERROR_BUFLEN:          return("input buffer too small (or EOF)");
        case MAD_ERROR_BUFPTR:          return("invalid (null) buffer pointer");
        case MAD_ERROR_NOMEM:           return("not enough memory");

        /* Frame header related unrecoverable errors */
        case MAD_ERROR_LOSTSYNC:        return("lost synchronization");
        case MAD_ERROR_BADLAYER:        return("reserved header layer value");
        case MAD_ERROR_BADBITRATE:      return("forbidden bitrate value");
        case MAD_ERROR_BADSAMPLERATE:   return("reserved sample frequency value");
        case MAD_ERROR_BADEMPHASIS:     return("reserved emphasis value");

        /* Recoverable errors */
        case MAD_ERROR_BADCRC:          return("CRC check failed");
        case MAD_ERROR_BADBITALLOC:     return("forbidden bit allocation value");
        case MAD_ERROR_BADSCALEFACTOR:  return("bad scalefactor index");
        case MAD_ERROR_BADFRAMELEN:     return("bad frame length");
        case MAD_ERROR_BADBIGVALUES:    return("bad big_values count");
        case MAD_ERROR_BADBLOCKTYPE:    return("reserved block_type");
        case MAD_ERROR_BADSCFSI:        return("bad scalefactor selection info");
        case MAD_ERROR_BADDATAPTR:      return("bad main_data_begin pointer");
        case MAD_ERROR_BADPART3LEN:     return("bad audio data length");
        case MAD_ERROR_BADHUFFTABLE:    return("bad Huffman table select");
        case MAD_ERROR_BADHUFFDATA:     return("Huffman data overrun");
        case MAD_ERROR_BADSTEREO:       return("incompatible block_type for JS");

        /* Unknown error. This switch may be out of sync with libmad's
         * defined error codes
         */
        default:                        return("Unknown error code");
    }
}


/*******************************************************************************
 * @brief       
 * @param       
 * @retval      
 * @attention   
*******************************************************************************/
signed short MP3_libmad_MadFixedToSshort(mad_fixed_t Sample)
{
    /* round */
    Sample += (1L << (MAD_F_FRACBITS - 16));

    /* clip */
    if(Sample >= MAD_F_ONE)
    {
        Sample = MAD_F_ONE - 1;
    }
    else if(Sample < -MAD_F_ONE)
    {
        Sample = -MAD_F_ONE;
    }

    /* quantize */
    return (Sample >> (MAD_F_FRACBITS + 1 - 16));
}


/*******************************************************************************
 * @brief       
 * @param       
 * @retval      
 * @attention   
*******************************************************************************/
void MP3_libmad_PrintFrameInfo(struct mad_header *Header)
{
    const char *Layer, *Mode, *Emphasis;

    switch(Header->layer)
    {
        case MAD_LAYER_I  : Layer = "I";   break;
        case MAD_LAYER_II : Layer = "II";  break;
        case MAD_LAYER_III: Layer = "III"; break;
        default:
            Layer = "(unexpected layer value)";
            break;
    }

    /* Convert the audio mode to it's printed representation. */
    switch(Header->mode)
    {
        case MAD_MODE_SINGLE_CHANNEL: Mode = "single channel";              break;
        case MAD_MODE_DUAL_CHANNEL  : Mode = "dual channel";                break;
        case MAD_MODE_JOINT_STEREO  : Mode = "joint (MS/intensity) stereo"; break;
        case MAD_MODE_STEREO        : Mode = "normal LR stereo";            break;
        default:
            Mode = "(unexpected mode value)";
            break;
    }

    switch(Header->emphasis)
    {
        case MAD_EMPHASIS_NONE      : Emphasis = "no";         break;
        case MAD_EMPHASIS_50_15_US  : Emphasis = "50/15 us";   break;
        case MAD_EMPHASIS_CCITT_J_17: Emphasis = "CCITT J.17"; break;
        default:
            Emphasis = "(unexpected emphasis value)";
            break;
    }

    printf("\r\n");
    printf("%lu bp/s audio MPEG layer %s stream %s CRC, "
           "%s with %s emphasis at %d Hz sample rate\r\n",
            Header->bitrate, Layer,
            Header->flags & MAD_FLAG_PROTECTION ? "with" : "without",
            Mode,Emphasis, Header->samplerate);
    printf("\r\n");
}


/*******************************************************************************
 * @brief       
 * @param       
 * @retval      
 * @attention   
*******************************************************************************/
void MP3_libmad_PlayHandler(uint32_t SampleRate)
{
    if(MP3_libmad_BufferSize == MP3_LIBMAD_O_BUFFER_SIZE)
    {
        if(SampleRate != MP3_libmad_SampleRate)  
        {
            MP3_libmad_SampleRate = SampleRate;

            I2S_Configure(I2S_Protocol_PHILIPS, I2S_DataWidth_16b,I2S_AudioFreq_44k, I2S_XferMode_TxOnly);
        }

       // I2S_DMA_WaitEOT();
				while(I2S_DMA_Finish!=1)
				{
					Scheduler_Run();
					printf("-----Wait-----\r\n");
				}

        I2S_DMA_Transfer(MP3_libmad_oBuffer[MP3_libmad_NextIndex], MP3_LIBMAD_O_BUFFER_SIZE);

        if(MP3_libmad_NextIndex == 0) MP3_libmad_NextIndex = 1;
        else                          MP3_libmad_NextIndex = 0;

        MP3_libmad_BufferSize = 0;
    }
}


/*******************************************************************************
 * @brief       
 * @param       
 * @retval      
 * @attention   
*******************************************************************************/
void MP3_libmad_PlaySong(char *Path, char *Name)
{
    static char     FilePath[100];
    static int      TagSize   = 0;
    static uint32_t FrameCount  = 0;

    /* First the structures used by libmad must be initialized. */
    mad_stream_init(&MP3_libmad_Stream);
    mad_frame_init( &MP3_libmad_Frame );
    mad_synth_init( &MP3_libmad_Synth );
    mad_timer_reset(&MP3_libmad_Timer );

    memset( FilePath, 0x00, sizeof(FilePath));
    sprintf(FilePath, "%s%s",   Path,   Name);

    if(f_open(&MP3_libmad_File, FilePath, FA_READ) == FR_OK)
    {
        I2S_PowerON(1);

        MP3_libmad_PlayEnded = 0;
        I2S_DMA_Finish       = 1;


        MP3_libmad_RES = f_read(&MP3_libmad_File, MP3_libmad_iBuffer, MP3_LIBMAD_I_BUFFER_SIZE, &MP3_libmad_BR);

        if(strncmp("ID3", (char *)MP3_libmad_iBuffer, 3) == 0)
        {
            /*计算标签信息总大小  不包括标签头的10个字节*/
            TagSize =  ((unsigned int)MP3_libmad_iBuffer[6] << 21) |
                       ((unsigned int)MP3_libmad_iBuffer[7] << 14) |
                       ((unsigned int)MP3_libmad_iBuffer[8] << 7)  |
                       ((unsigned int)MP3_libmad_iBuffer[9] << 0);

            TagSize += 10;

            printf("\r\nMP3 TAG Size : %d\r\n", TagSize);
        }

        f_lseek(&MP3_libmad_File, TagSize);   /* 跳过TAG信息 */


        while(1)
        {
            if((MP3_libmad_Stream.buffer == NULL) || (MP3_libmad_Stream.error == MAD_ERROR_BUFLEN))
            {
                size_t         ReadSize, Remaining;		
                unsigned char *ReadStart = NULL;

                if(MP3_libmad_Stream.next_frame != NULL)
                {
                    Remaining = MP3_libmad_Stream.bufend - MP3_libmad_Stream.next_frame;
                    memmove(MP3_libmad_iBuffer, MP3_libmad_Stream.next_frame, Remaining);

                    ReadStart = MP3_libmad_iBuffer       + Remaining;
                    ReadSize  = MP3_LIBMAD_I_BUFFER_SIZE - Remaining;
                }
                else
                {
                    ReadSize  = MP3_LIBMAD_I_BUFFER_SIZE,
                    ReadStart = MP3_libmad_iBuffer,
                    Remaining = 0;
                }

                MP3_libmad_RES = f_read(&MP3_libmad_File, (char *)ReadStart, ReadSize, &MP3_libmad_BR);

                if((MP3_libmad_BR <= 0) || (MP3_libmad_BR < ReadSize))
                {
                    printf("\r\nEnd Of File\r\n");  break;
                }

                mad_stream_buffer(&MP3_libmad_Stream, MP3_libmad_iBuffer, MP3_libmad_BR + Remaining);
                MP3_libmad_Stream.error = MAD_ERROR_NONE;
            }

            if(mad_frame_decode(&MP3_libmad_Frame, &MP3_libmad_Stream))
            {
                if(MAD_RECOVERABLE(MP3_libmad_Stream.error))
                {
                    if((MP3_libmad_Stream.error != MAD_ERROR_LOSTSYNC) || (MP3_libmad_Stream.this_frame != NULL))
                    {
                        printf("\r\nRecoverable   Frame Level Error (%s)\r\n", MP3_libmad_MadErrorString(&MP3_libmad_Stream));
                    }

                    continue;
                }
                else
                {
                    if(MP3_libmad_Stream.error == MAD_ERROR_BUFLEN)
                    {
                        continue;
                    }
                    else
                    {
                        printf("\r\nUnrecoverable Frame Level Error (%s)\r\n", MP3_libmad_MadErrorString(&MP3_libmad_Stream));
                        break;
                    }
                }
             }

            if(FrameCount == 0)
            {
                MP3_libmad_PrintFrameInfo(&MP3_libmad_Frame.header);
            }

            FrameCount++;
            mad_timer_add(&MP3_libmad_Timer, MP3_libmad_Frame.header.duration);

            mad_synth_frame(&MP3_libmad_Synth, &MP3_libmad_Frame);

            for(uint32_t i = 0; i < MP3_libmad_Synth.pcm.length; i++)
            {
                short Sample = MP3_libmad_MadFixedToSshort(MP3_libmad_Synth.pcm.samples[0][i]);

                MP3_libmad_oBuffer[MP3_libmad_NextIndex][MP3_libmad_BufferSize++] = Sample;

                if(MAD_NCHANNELS(&MP3_libmad_Frame.header) == 2)	
                {
                    Sample = MP3_libmad_MadFixedToSshort(MP3_libmad_Synth.pcm.samples[1][i]);
                }

                MP3_libmad_oBuffer[MP3_libmad_NextIndex][MP3_libmad_BufferSize++] = Sample;

                MP3_libmad_PlayHandler(MP3_libmad_Synth.pcm.samplerate);
            }

            if(MP3_libmad_PlayEnded == 1)
            {
                break;
            }
        }

        DMA_EnableChannel(DMA1,DMA_REQ_DMA1_SPI2_TX,false);

        f_close(&MP3_libmad_File);

        I2S_PowerON(0);
    }

    mad_synth_finish( &MP3_libmad_Synth );
    mad_frame_finish( &MP3_libmad_Frame );
    mad_stream_finish(&MP3_libmad_Stream);

    static char Buffer[80];
    mad_timer_string(MP3_libmad_Timer, Buffer, "%lu:%02lu.%03u", MAD_UNITS_MINUTES, MAD_UNITS_MILLISECONDS, 0);
    printf("\r\n%d Frames Decoded (%s).\r\n", FrameCount, Buffer);
}

