#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "Hz_string.h"

#include "ff.h"
#include "init.h"
#include "keyboard.h"
#include "rtc.h"
#include "startup_info.h"

#include "ServiceFatFs.h"
#include "ServiceFlashMap.h"
#include "ServiceGraphic.h"
#include "ServiceRawFlash.h"
#include "ServiceSTMPPartition.h"
#include "ServiceSwap.h"
#include "ServiceUSBDevice.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "pageman.h"

unsigned char *vbuf;

GraphicMessage GM;
GraphicTextOutArgs text1;
BYTE work[FF_MAX_SS];

static void Hz16chOut(const unsigned char **HZlist, unsigned int len, unsigned int x, unsigned int y) {
    unsigned char c, i;
    const unsigned char *p;
    unsigned char mode = 0;
    p = (const unsigned char *)HZlist;
    while (len) {
        c = 0;
        i = 0;
        c = *p;
        p++;
        if (c == 'F') {
            if (x + 16 > 256) {
                x = 0;
                y += 16;
            }
            for (int ly = 0; ly < 32; ly++) {
                c = *p;
                for (int lx = 0; lx < 8; lx++) {

                    if (c & 1) {
                        vbuf[(x++) + 258 * (y + 8)] = 0xFF;
                    } else {
                        vbuf[(x++) + 258 * (y + 8)] = 0;
                    }
                    c = c >> 1;
                }
                i++;
                p++;
                if ((int)i % 2 == 0) {
                    y++;
                    x -= 16;
                }
            }
            x += 16;
            y -= 16;
        } else {
            if (x + 8 > 256) {
                x = 0;
                y += 16;
            }
            for (int ly = 0; ly < 16; ly++) {
                c = *p;
                for (int lx = 0; lx < 8; lx++) {

                    if (c & 1) {
                        vbuf[(x++) + 258 * (y + 8)] = 0xFF;
                    } else {
                        vbuf[(x++) + 258 * (y + 8)] = 0;
                    }
                    c = c >> 1;
                }
                p++;
                y++;
                x -= 8;
            }
            x += 8;
            y -= 16;
            p += 16;
        }
        len--;
    }

    GM.type = GRAPHIC_MSG_TYPE_FLUSH;
    xQueueSend(GraphicQueue, &(GM.selfAddr), (TickType_t)0);
}

void displayRecovery() {
    GM.type = GRAPHIC_MSG_TYPE_CLEAR;
    xQueueSend(GraphicQueue, &(GM.selfAddr), (TickType_t)0);

    Hz16chOut((const unsigned char **)&HzString1, 6, 1, 1);
    Hz16chOut((const unsigned char **)&HzString2, 44, 1 + 32, 1 + 16 * 1);
    Hz16chOut((const unsigned char **)&HzString3, 3, 1 + 32, 1 + 16 * 4);
    Hz16chOut((const unsigned char **)&HzString4, 9, 1 + 32, 1 + 16 * 5);
}

extern unsigned int FSOK;

void runInRecoverMode() {

    //for (;;) {
        //if (is_key_down(KEY_1)) {
            GM.type = GRAPHIC_MSG_TYPE_CLEAR;
            xQueueSend(GraphicQueue, &(GM.selfAddr), (TickType_t)0);
            vTaskDelay(100);

            if (isRawFlash()) {

                //Hz16chOut((const unsigned char**)&HzString6,9,1+32,1 + 16 * 3);
                //vTaskDelay(1000);
                //resetFlashRegionInfo();
                //flashMapReset();

                displayRecovery();
                goto next_loop;
            }

            FSOK = 0;

            lockFmap(true);
            flashMapClear();

            printf("start earsing\n");
            for (int i = getDataRegonStartBlock(); i < getDataRegonStartBlock() + getDataRegonTotalBlocks(); i++) {

                xEraseFlashBlocks(i, 1, 5000);
                vTaskDelay(1);
            }

            flashMapReset();

            MKFS_PARM opt;
            opt.fmt = FM_FAT;
            opt.au_size = 2048;
            opt.align = 2048;
            opt.n_fat = 2;


            FATFS fs;

            int fr = f_mkfs("", &opt, work, sizeof work);
            //int fr = f_mkfs("", 0, work, sizeof work);

            printf("format :%d\n", fr);

            f_mount(&fs, "/", 1);

            lockFmap(false);
            printf("format done.\n");
            f_setlabel("HP 39GII");
            FSOK = 1;

            Hz16chOut((const unsigned char **)&HzString5, 2, 1 + 16 * 10, 1 + 16 * 4);

            vTaskDelay(2000);

            flashSyncNow();
            displayRecovery();
        

    for (;;) {
    next_loop:

        vTaskDelay(100);
    }
}

void vInit() {
    vTaskDelay(200);
    /*
	GM.selfAddr = &GM;
	GM.type = GRAPHIC_MSG_TYPE_CLEAR;
	xQueueSend(GraphicQueue, &(GM.selfAddr), ( TickType_t ) 0 );
	
	GM.type = GRAPHIC_MSG_TYPE_TEXTOUT;
	GM.argsList = &text1;
	text1.x = 1;
	text1.y = 1;
	text1.area_width = 100;
	text1.area_height = 16;
	text1.font_size = 12;
	text1.font_color = 255;
	text1.text = "Test a";
	xQueueSend(GraphicQueue, &(GM.selfAddr) , ( TickType_t ) 0 );
	*/
    

    GM.selfAddr = &GM;
    GM.type = GRAPHIC_MSG_TYPE_CLEAR;
    xQueueSend(GraphicQueue, &(GM.selfAddr), (TickType_t)0);
    vbuf = (char *)getVramAddress();

    /*
	
	taskENTER_CRITICAL();
	
	
	
	
	taskEXIT_CRITICAL();
	*/
    //runInRecoverMode();
    if (!isfatFsInited()) {
        displayRecovery();
        runInRecoverMode();
    }

    if (is_key_down(KEY_PLUS)) {
        displayRecovery();
        runInRecoverMode();
    }
    
    GM.selfAddr = &GM;
    GM.type = GRAPHIC_MSG_TYPE_CLEAR;
    xQueueSend(GraphicQueue, &(GM.selfAddr), (TickType_t)0);
    //modifyRegion(0,22,49);
    //saveRegionTable();

    GM.type = GRAPHIC_MSG_TYPE_TEXTOUT;
    GM.argsList = &text1;
    text1.x = 1;
    text1.y = 1;
    text1.area_width = 100;
    text1.area_height = 16;
    text1.font_size = 12;
    text1.font_color = 255;
    text1.text = "Booting stage 2...";
    xQueueSend(GraphicQueue, &(GM.selfAddr), (TickType_t)0);

    //int x = 0;
    //unsigned int args[3] = {11 ,32, 43};
    //x = testCall(args,3);

    //printf("retx:%d\n",x);
    //mallopt(M_MXFAST,4);
    
    //vPortFree(pvPortMalloc(200*1024));
    malloc_stats();

    /*
	for(int i=0;i<6;i++){
		printf("RTC:%d : %08x\n",i,rtc_persistent_get(i));	
	}
	
	rtc_persistent_set(3,0x55aa5a5a);

	for(int i=0;i<6;i++){
		printf("RTC:%d : %08x\n",i,rtc_persistent_get(i));	
	}
	*/

    //asm volatile ("swi #1001");
/*
    FIL config_file;
    FRESULT fr;
    unsigned char *Config_line_buf = pvPortMalloc(1024);
    unsigned int arg = 0;

    fr = f_open(&config_file, "config", FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
    printf("Read config:%d\n", fr);

    while (f_gets(Config_line_buf, 1024, &config_file)) {
        unsigned int equ_pos = 0;
        unsigned int p = 0;
        while (Config_line_buf[p] != 0) {
            if (Config_line_buf[p++] == '=') {
                equ_pos = p - 1;
                break;
            }
        }
        if (equ_pos > 0) {
            if (memcmp("swap_mb", Config_line_buf, equ_pos) == 0) {
                p = equ_pos + 1;
                arg = atoi(&Config_line_buf[p]);
                swapSizeMB = arg;
                printf("swap size=%d MB\n", arg);
            }
        }
        memset(Config_line_buf, 0, 1024);
    }
    f_close(&config_file);

    vPortFree(Config_line_buf);
*/

    if(swapSizeMB == 0){
        swapSizeMB = 16;
    }

    //xTaskCreate(vServiceSwap, "Swap Svc", configMINIMAL_STACK_SIZE, NULL, 4, NULL);
    int res;
    printf("init vm...\n");
    res = pageman_init(16,swapSizeMB);
    printf("init vm:%d\n",res);
    
    xTaskCreate(vServiceUSBDevice, "USB Device Service", configMINIMAL_STACK_SIZE, NULL, 4, NULL);

    void vm_test();

    vm_test();

    //xTaskCreate( vFaultTask, "Fault Task", configMINIMAL_STACK_SIZE, NULL, 3, NULL );

    /*
	printf("Page fault test.\n");
	int a = *((unsigned int *)0x140000);
	int b = *((unsigned int *)0x40000);
	printf("%08x %08x\n",a,b);	
	*/

    vTaskDelete(NULL);
    for (;;) {
    }
}


/*
int testCall(unsigned int *args, unsigned int argsLen){
	unsigned int ret = 0;
	unsigned int *retAddr = ret;
	asm volatile ("ldr r0,%0" :: "m"(args));
	asm volatile ("ldr r1,%0" :: "m"(argsLen));
	asm volatile ("ldr r2,%0" :: "m"(retAddr));
	asm volatile ("swi #1000");
	return ret;
}
*/
