#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include <stdint.h>
#include "stm32f1xx.h"

/* 用户程序基地址 */
#define APP_FLASH_ADDR				(0x08004000UL)
/* 用户程序大小 */
#define APP_FLASH_SIZE				(0x4000)
/* 用户程序尾地址 */
#define APP_FLASH_END_ADDR		(APP_FLASH_ADDR + APP_FLASH_SIZE)

/* 用户堆栈基地址 */
#define SRAM_START_ADDR				(0x20000000UL)
/* 用户堆栈大小 */
#define SRAM_SIZE							(0x5000)
/* 用户堆栈尾地址 */
#define SRAM_END_ADDR					(SRAM_START_ADDR + SRAM_SIZE)




uint8_t Boot_IsAppValid(void);
void JumpToApp(void);


#endif
