/*
 * Copyright 2021 MindMotion Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board_init.h"
#include "hal_common.h"
#include "ffconf.h"
#include "ff.h"

/*
 * Macros.
 */

/*
 * Variables.
 */
 
FATFS fs;

/*
 * Declerations.
 */
void fatFs_init(void);
void delay_init(uint32_t TICK_RATE_HZ);
void delay_ms(uint16_t nms);
/*
 * Functions.
 */
int main(void)
{
		BOARD_Init();
		printf("**********MM32F5270************\r\n");
		printf(">BOARD_Init() Done!\r\n");
		printf(">fatFs_init() Start...\r\n");
		fatFs_init();
		printf(">fatFs_init() Done!\r\n");
		printf(">Scheduler_Setup() Start...\r\n");
		Scheduler_Setup();
		printf(">Scheduler_Setup() Done!\r\n");
		printf("**********Init END************\r\n");
    while (1)
    {
			Scheduler_Run();
    }
}

/*Test FatFs Init*/
void fatFs_init(void)
{
		static FIL File;
		static UINT br=0;
		static uint8_t Buffer[14];
		FRESULT  res;	
		
		printf(">f_mount() Start...\r\n");
		res = f_mount(&fs,"1:/",1);
		if(res == FR_OK)
		{
				printf(">f_mount() Done!\r\n");
				printf(">f_open() Start...\r\n");
				res = f_open(&File,"1:/HELLO.txt",FA_READ|FA_WRITE);
				if(res == FR_OK)
				{
					printf(">f_open() Done!\r\n");
					do{
							res = f_read(&File,Buffer,0XFF,&br);
							if(br!=0)
							{
								printf(">f_read() Done! HELLO.txt:%s\r\n",Buffer);
							}
					}while(br!=0);
					f_close(&File);
				}else{
						printf(">f_open() Fail!,res=%d\r\n",res);
				}
		}else{
				printf(">f_mount() Fail!,res=%d\r\n",res);
				while(1);
		}
}

/*-----------------------------------------------------------*/


/* EOF. */

