#ifndef BOOTLOADER_H
#define BOOTLOADER_H
#include "boot_cfg.h"
#include <stdint.h>

/* Backward-compatible aliases for the old project. */
#define APP_FLASH_ADDR       BOOT_APP_BASE
#define APP_FLASH_SIZE       BOOT_APP_SIZE
#define APP_FLASH_END_ADDR   BOOT_APP_END
#define SRAM_START_ADDR      BOOT_RAM_BASE
#define SRAM_END_ADDR        BOOT_RAM_END

uint8_t Boot_ImageVectorsValid(uint32_t image_base, uint32_t size);
uint8_t Boot_IsAppValid(void);
void JumpToApp(void);
#endif
