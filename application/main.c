/*
 * Copyright 2021 MindMotion Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board_init.h"
#include "ffconf.h"
#include "ff.h"


/*
 * Macros.
 */

/*
 * Variables.
 */

FATFS fs;
const TCHAR fs_drv[] = "1:/";
TCHAR fs_path[256] = "\0";


/*
 * Declerations.
 */
 FRESULT app_fatfs_listfiles(const char * dir_path);


/*
 * Functions.
 */
int main(void)
{
    uint8_t ch;

    BOARD_Init();

    printf("sdspi_basic example.\r\n");
	
		/* f_mount().\r\n */
    printf("f_mount(). ");
    if( !f_mount(&fs, fs_drv ,1) )
    {
        printf("succ.\r\n");
    }
    else
    {
        printf("fail.\r\n");
        while (1)
        {}
    }
    
    /* root dir. */
    app_fatfs_listfiles(fs_drv);
    
    /* dir0. */
    fs_path[0] = '\0';
    strcat(fs_path, fs_drv);
    strcat(fs_path, "dir0/");
    app_fatfs_listfiles(fs_path);

    /* dir1. */
    fs_path[0] = '\0';
    strcat(fs_path, fs_drv);
    strcat(fs_path, "dir1/");
    app_fatfs_listfiles(fs_path);
    
    printf("app_fatfs_listfiles() done.\r\n");

    while (1)
    {
        ch = getchar();
        putchar(ch);
    }
}

/* list the file items under the indecated path. */
FRESULT app_fatfs_listfiles(const char * dir_path)
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    char *fn;

    printf("* %s ->\r\n", dir_path);
    
    res = f_opendir(&dir, dir_path);
    if (res != FR_OK)
    {
        return res;
    }

    for (;;)
    {
        /* read iterator. */
        res = f_readdir(&dir, &fno);
        if ( (res != FR_OK) || (fno.fname[0] == 0) )
        {
            break;
        }

        /* skip the "self" and "father" dir item. */
        if (fno.fname[0] == '.') 
        {
            continue;
        }
        
        /* collect the dir or file name. */
        fn = fno.fname;
        if (fno.fattrib & AM_DIR) /* dir name. */
        {
            printf("\t%s/\r\n", fn);
        } 
        else /* file name */
        {
            printf("\t%s: %u B\r\n", fn, (unsigned)fno.fsize);
        }			
    }

    /* close the opened dir to reest the iterator. */
    res = f_closedir(&dir);

    return res;
}

/* EOF. */

