#ifndef BOOT_FLASH_H
#define BOOT_FLASH_H
#include "stm32f1xx_hal.h"
#include "boot_cfg.h"

HAL_StatusTypeDef Boot_FlashErase(uint32_t addr, uint32_t size);
HAL_StatusTypeDef Boot_FlashWrite(uint32_t addr, const uint8_t *data, uint32_t size);
HAL_StatusTypeDef Boot_FlashVerify(uint32_t addr, const uint8_t *data, uint32_t size);
#endif
