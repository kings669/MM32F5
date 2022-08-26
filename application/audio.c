#include "audio.h"
#include "mp3.h"

uint8_t SongNumber = 0;
char    SongName[20][25];

uint8_t AUDIO_StartPlay = 1;
uint8_t AUDIO_PlayState = 0;
uint8_t AUDIO_PlayIndex = 0;
uint8_t AUDIO_Extension = 0;

extern uint32_t WAV_PlaybackTotal;
extern uint32_t WAV_PlaybackProgress;

FRESULT Audio_ScanFiles(char *path);


void AUDIO_Init(void)
{
    Audio_ScanFiles("1:/Music");

    printf("\r\nSong Number : %d\r\n", SongNumber);

}

void Audio_Task(void)
{
	  static uint8_t  AUDIO_Switching = 0;
    static uint32_t AUDIO_PlayTime  = 0;
		
		static char Buffer[100];
	
    if(AUDIO_StartPlay == 1)//Only one
    {
        AUDIO_StartPlay = 0;

        if((AUDIO_PlayState == 0) && (SongNumber != 0))
        {
            printf("\r\n\r\n%s\r\n", __FUNCTION__);
						
            if((strstr(SongName[AUDIO_PlayIndex], "wav") != NULL) || (strstr(SongName[AUDIO_PlayIndex], "WAV") != NULL))
            {
                AUDIO_Extension = 0;//WAV
                AUDIO_Switching = 1;
							
								printf("Start Play : %s  FileType: WAV\r\n",SongName[AUDIO_PlayIndex]);

                WAV_PlaySong("1:/Music/", SongName[AUDIO_PlayIndex]);
            }
            else if((strstr(SongName[AUDIO_PlayIndex], "mp3") != NULL) || (strstr(SongName[AUDIO_PlayIndex], "MP3") != NULL))
            {
                AUDIO_Extension = 1;//MP3
                AUDIO_Switching = 1;
                AUDIO_PlayTime  = 0;

								printf("Start Play : %s  FileType: MP3\r\n",SongName[AUDIO_PlayIndex]);
							
								MP3_libmad_PlaySong("1:/Music/", SongName[AUDIO_PlayIndex]);
            }
            else
            {
                printf("\r\nUnknow Audio File!!!\r\n");
            }
        }
    }
		
    if(AUDIO_PlayState == 1)
    {
        memset(Buffer, 0, sizeof(Buffer));

        if(AUDIO_Extension == 0)
        {
						float res_Back=0;
						//res_Back = ((WAV_PlaybackProgress) / (WAV_PlaybackTotal)) * 100.0f;
						//res_Back = ((float)(WAV_PlaybackProgress) / (float)(WAV_PlaybackTotal));
						//sprintf(Buffer, "%.1f", res_Back);
					printf(">WAV_PlaybackProgress:%d,WAV_PlaybackTotal:%d\r\n",WAV_PlaybackProgress,WAV_PlaybackTotal);
        }
        else
        {
            AUDIO_PlayTime++;
            sprintf(Buffer, "%02d:%02d", ((AUDIO_PlayTime/2)/60), ((AUDIO_PlayTime/2)%60));
						printf(">AUDIO_PlayTime:%s\r\n",Buffer);
        }
    }
    else
    {
        if(AUDIO_Switching == 1)
        {
            AUDIO_Switching = 0;

            for(uint8_t i = 0; i < SongNumber; i++)//printf Song Name
            {
                memset(Buffer, 0, sizeof(Buffer));
                sprintf(Buffer, "%02d.%s", i, SongName[i]);

								printf(">SongName:%s\r\n",Buffer);
            }
        }
    }

    //memset( Buffer,   0x00, sizeof(Buffer));
    //sprintf(Buffer, "%02d", AUDIO_PlayIndex);
    //LCD_ShowLOG(200, 0, Buffer);
		//printf(">AUDIO_PlayIndex:%s\r\n",Buffer);
}


FRESULT Audio_ScanFiles(char *path)
{
	  static FRESULT res;
    static DIR     dir;
    static UINT    i;
		static FILINFO fno;
		
    char   Buffer[30];
		
		printf(">f_opendir() Start...\r\n");
    res = f_opendir(&dir, path);                        /* Open the directory */
		
    if(res == FR_OK)
    {
				printf(">Path :%s f_opendir() Done!\r\n",path);
        while(1)
        {
						printf(">--Read a directory--item f_readdir() Start...\r\n");
            res = f_readdir(&dir, &fno);                /* Read a directory item */

            if((res != FR_OK) || (fno.fname[0] == 0))   /* Break on error or end of dir */
            {
								printf(">Break on error or end of dir res=%d\r\n",res);
                break;
            }
						printf(">--Read a directory--item f_readdir() Done!\r\n");
						
            if(fno.fattrib & AM_DIR)                    /* It is a directory */
            {
                i = strlen(path);

                sprintf(&path[i], "/%s", fno.fname);
								printf(">It is a directory!\r\n");
								printf(">Enter the directory...\r\n");
                res = Audio_ScanFiles(path);            /* Enter the directory */
                if (res != FR_OK)break;
                path[i] = 0;
            }
            else
            {
							/* It is a file. */
								printf(">It is a file.\r\n");
                if(SongNumber < 20)
                {
                    memset(Buffer, 0, sizeof(Buffer));
                    sprintf(Buffer, "%02d.%s", SongNumber, fno.fname);
										
										printf("%s\r\n",Buffer);
										printf("Name:%s\r\n",fno.fname);
                    memset(SongName[SongNumber], 0, sizeof(SongName[SongNumber]));
                    strcpy(SongName[SongNumber], fno.fname);
									
                    SongNumber++;
                }
            }
        }

        f_closedir(&dir);
    }else{
				printf(">f_opendir() Fail! res =%d\r\n",res);
		}

    return res;
}