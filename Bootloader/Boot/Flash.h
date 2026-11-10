#ifndef __BOOT_FLASH_H
#define __BOOT_FLASH_H

#include "stm32f1xx_hal.h"
#include "bootloader.h"
#include <stdint.h>

/* =========================================================
 * STM32F103C8T6 Flash configuration
 * ========================================================= */

/* Flash 最小擦除单位：1KB */
#define BOOT_FLASH_PAGE_SIZE       0x400UL

/* 预留的 Flash 测试区域 */
#define BOOT_TEST_FLASH_ADDR       0x0800FC00UL
#define BOOT_TEST_FLASH_SIZE       0x400UL
#define BOOT_TEST_FLASH_END        \
    (BOOT_TEST_FLASH_ADDR + BOOT_TEST_FLASH_SIZE)

/*
 * 写保护配置
 *
 * 0：禁止擦写 APP，避免测试时误删除用户程序
 * 1：允许擦写 APP，后续实现正式 IAP 时启用
 */
#define BOOT_ALLOW_APP_WRITE       0U

/*
 * 1：允许访问最后 1KB 测试区域
 * 0：禁止访问测试区域
 */
#define BOOT_ALLOW_TEST_WRITE      1U

/* =========================================================
 * Flash API
 * ========================================================= */

/* 擦除 Flash */
HAL_StatusTypeDef Boot_FlashErase(
    uint32_t Address,
    uint32_t Size
);

/* 写入 Flash */
HAL_StatusTypeDef Boot_FlashWrite(
    uint32_t Address,
    const uint8_t *Buffer,
    uint32_t Size
);

/* 读回校验 Flash 数据 */
HAL_StatusTypeDef Boot_FlashVerify(
    uint32_t Address,
    const uint8_t *Buffer,
    uint32_t Size
);

#endif
