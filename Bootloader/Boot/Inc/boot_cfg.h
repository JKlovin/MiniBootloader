#ifndef BOOT_CFG_H
#define BOOT_CFG_H

#include <stdint.h>

/* Flash总容量 */
#define BOOT_FLASH_TOTAL      0x00010000UL
/* Flash基地址 */
#define BOOT_FLASH_BASE       0x08000000UL
/* Flash尾地址 */
#define BOOT_FLASH_END        (BOOT_FLASH_BASE + BOOT_FLASH_TOTAL)
/* Flash页大小 */
#define BOOT_FLASH_PAGE       0x00000400UL

/* Bootloader基地址 */
#define BOOT_LOADER_BASE      0x08000000UL
/* Bootloader容量大小 */
#define BOOT_LOADER_SIZE      0x00004000UL

/* 用户程序APP基地址 */
#define BOOT_APP_BASE         0x08004000UL
/* 用户程序APP容量大小 */
#define BOOT_APP_SIZE         0x00006000UL
/* 用户程序APP尾地址 */
#define BOOT_APP_END          (BOOT_APP_BASE + BOOT_APP_SIZE)

/* 固件暂存区基地址 */
#define BOOT_STAGE_BASE       0x0800A000UL
/* 固件暂存区容量大小 */
#define BOOT_STAGE_SIZE       0x00005C00UL
/* 固件暂存区尾地址 */
#define BOOT_STAGE_END        (BOOT_STAGE_BASE + BOOT_STAGE_SIZE)

/* 固件信息记录区基地址 */
#define BOOT_META_BASE        0x0800FC00UL
/* 固件信息记录区容量大小 */
#define BOOT_META_SIZE        0x00000400UL
/* 固件信息记录区尾地址 */
#define BOOT_META_END         (BOOT_META_BASE + BOOT_META_SIZE)

/* 最大允许升级的固件大小 */
#define BOOT_MAX_IMAGE        BOOT_STAGE_SIZE

/* SRAM基地址 */
#define BOOT_RAM_BASE         0x20000000UL
/* SRAM容量大小 */
#define BOOT_RAM_SIZE         0x00005000UL
/* SRAM尾地址 */
#define BOOT_RAM_END          (BOOT_RAM_BASE + BOOT_RAM_SIZE)

/* ============================================================
 * BUP固件包格式
 * ============================================================ */
 /* BUP固件包MAGIC，用于识别合法格式的固件包
 * 0x31505542在小端模式下对应ASCII字符串"BUP1"
 */
#define BOOT_PACKAGE_MAGIC    0x31505542UL
/* BUP固件包头部长度：24字节
 * 包含MAGIC、APP目标地址、固件大小、固件CRC32、
 * 固件版本号、包头CRC32，共6个uint32_t字段
 */
#define BOOT_PACKAGE_HEADER   24U
/* Bootloader自动跳转APP的等待时间：30000ms */
#define BOOT_AUTOJUMP_MS      30000U

#if (BOOT_APP_END != BOOT_STAGE_BASE) || \
    (BOOT_STAGE_END != BOOT_META_BASE) || \
    (BOOT_META_END != BOOT_FLASH_END) || \
    (BOOT_LOADER_BASE + BOOT_LOADER_SIZE != BOOT_APP_BASE)
#error "Boot partition layout is not contiguous"
#endif

#endif
