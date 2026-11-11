#include "boot_meta.h"
#include "boot_crc.h"
#include <string.h>
#include <stddef.h>

#define META_SLOT_COUNT (BOOT_META_SIZE / 32U)

typedef char CheckMetaSize[(sizeof(BootMeta_t) == 32U) ? 1 : -1];

/* 检查Meta区是否非空 */
/* 返回 1 ：该地址Meta槽为空白，没有数据 */
/* 返回 0 ：该地址Meta槽为非空，已有数据 */
static uint8_t Meta_Blank(uint32_t addr)
{
    uint32_t i;
    for (i = 0U; i < sizeof(BootMeta_t); i += 2U)
        if (*(volatile const uint16_t *)(addr + i) != 0xFFFFU)
            return 0U;

    return 1U;
}

/* Meta区空闲槽位查找 */
static uint32_t Meta_NextSlot(void)
{
    uint32_t i, last = BOOT_META_BASE;
    uint8_t seen = 0U;
	
	/* 找到最后一个非空槽 */
    for (i = 0U; i < META_SLOT_COUNT; i++)
    {
        uint32_t addr = BOOT_META_BASE + i * sizeof(BootMeta_t);
        if (!Meta_Blank(addr))
        {
            last = addr;
            seen = 1U;
        }
    }
		
		/* 一片空白，即Meta还没被使用 */
    if (!seen)
        return BOOT_META_BASE;
		
		/* 槽位已被占满 */
    if (last >= BOOT_META_END - sizeof(BootMeta_t))
        return 0U;
		
    return last + sizeof(BootMeta_t);
}

uint8_t Boot_MetaHasRoom(void)
{
    return (uint8_t)(Meta_NextSlot() != 0U);
}

/* 从 META Flash 分区中查找最新有效升级记录的函数 */
BootMetaState_t Boot_MetaLatest(BootMeta_t *out, uint32_t *record_addr)
{
    uint32_t i, found_addr = 0U;
    uint8_t nonblank = 0U;
    BootMeta_t item, latest;

    /* 从Meta分区读取记录 */
    for (i = 0U; i < META_SLOT_COUNT; i++)
    {
        uint32_t addr = BOOT_META_BASE + i * sizeof(BootMeta_t);

        /* 筛选有效记录 */

        /* 检查当前槽位是否全部为 0xFFFF 半字，即是否为空白 */
        if (Meta_Blank(addr))
            continue;
        /* 非空 */
        nonblank = 1U;
        /* 检查Meta分区固件升级信息 */
        memcpy(&item, (const void *)addr, sizeof(item));
        if (item.magic != BOOT_META_MAGIC ||
            item.size < 8U || item.size > BOOT_MAX_IMAGE ||
            item.ready != BOOT_META_MARK ||
            item.header_crc != Boot_CRC32((const uint8_t *)&item, 16U))
            continue;
        latest = item;
        found_addr = addr;
    }

    if (found_addr != 0U)
    {
        if (out != NULL)
            *out = latest;
        if (record_addr != NULL)
            *record_addr = found_addr;
        return BOOT_META_VALID;
    }
    return nonblank ? BOOT_META_INVALID : BOOT_META_EMPTY;
}

/* 添加Meta记录信息 */
HAL_StatusTypeDef Boot_MetaAppend(uint32_t size, uint32_t crc32, uint32_t version)
{
    BootMeta_t rec;
    uint32_t addr;
    const uint8_t marker[2] = {0x5AU, 0xA5U};

    if (size < 8U || size > BOOT_MAX_IMAGE)
        return HAL_ERROR;
    addr = Meta_NextSlot();
    if (addr == 0U || !Meta_Blank(addr))
        return HAL_ERROR;

    memset(&rec, 0xFF, sizeof(rec));
    rec.magic = BOOT_META_MAGIC;
    rec.size = size;
    rec.crc32 = crc32;
    rec.version = version;
    rec.header_crc = Boot_CRC32((const uint8_t *)&rec, 16U);

    /* Commit header first, then READY marker last. */
    if (Boot_FlashWrite(addr, (const uint8_t *)&rec, 20U) != HAL_OK)
        return HAL_ERROR;
    return Boot_FlashWrite(addr + (uint32_t)offsetof(BootMeta_t, ready), marker, 2U);
}

static HAL_StatusTypeDef Meta_SetFlag(uint32_t addr, uint32_t field_offset)
{
    const uint8_t marker[2] = {0x5AU, 0xA5U};
    uint32_t field_addr = addr + field_offset;
    uint16_t current;
    BootMeta_t newest;
    uint32_t newest_addr;

    if (Boot_MetaLatest(&newest, &newest_addr) != BOOT_META_VALID ||
        newest_addr != addr)
        return HAL_ERROR;

    current = *(volatile const uint16_t *)field_addr;
    /* A torn non-blank marker means the transition was attempted.
       It is safe to treat STARTED / DONE as set; callers verify image CRC. */
    if (current != 0xFFFFU)
        return HAL_OK;
    return Boot_FlashWrite(field_addr, marker, 2U);
}

HAL_StatusTypeDef Boot_MetaSetStarted(uint32_t addr)
{
    return Meta_SetFlag(addr, (uint32_t)offsetof(BootMeta_t, started));
}

HAL_StatusTypeDef Boot_MetaSetDone(uint32_t addr)
{
    return Meta_SetFlag(addr, (uint32_t)offsetof(BootMeta_t, done));
}
