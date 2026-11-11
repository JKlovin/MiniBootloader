#include "boot_flash.h"
#include <stddef.h>

static uint8_t Flash_InRegion(uint32_t addr, uint32_t len,
                              uint32_t start, uint32_t end)
{
    return (uint8_t)((len != 0U) && (addr >= start) &&
                     (addr < end) && (len <= end - addr));
}

static uint8_t Flash_Allowed(uint32_t addr, uint32_t len)
{
    return (uint8_t)(Flash_InRegion(addr, len, BOOT_APP_BASE, BOOT_APP_END) ||
                     Flash_InRegion(addr, len, BOOT_STAGE_BASE, BOOT_STAGE_END) ||
                     Flash_InRegion(addr, len, BOOT_META_BASE, BOOT_META_END));
}

HAL_StatusTypeDef Boot_FlashErase(uint32_t addr, uint32_t size)
{
    FLASH_EraseInitTypeDef cfg = {0};
    HAL_StatusTypeDef st, lock_st;
    uint32_t count, actual, bad_page = 0xFFFFFFFFUL, i;

    if (!Flash_Allowed(addr, size) || (addr % BOOT_FLASH_PAGE) != 0U)
        return HAL_ERROR;

    count = (size - 1U) / BOOT_FLASH_PAGE + 1U;
    actual = count * BOOT_FLASH_PAGE;
    if (!Flash_Allowed(addr, actual))
        return HAL_ERROR;

    st = HAL_FLASH_Unlock();
    if (st != HAL_OK)
        return st;
		
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
    cfg.TypeErase = FLASH_TYPEERASE_PAGES;
    cfg.PageAddress = addr;
    cfg.NbPages = count;
    st = HAL_FLASHEx_Erase(&cfg, &bad_page);
		
    if (st == HAL_OK)
    {
        for (i = 0U; i < actual; i += 2U)
        {
            if (*(volatile const uint16_t *)(addr + i) != 0xFFFFU)
            {
                st = HAL_ERROR;
                break;
            }
        }
    }
    lock_st = HAL_FLASH_Lock();
    return (st == HAL_OK) ? lock_st : st;
}

HAL_StatusTypeDef Boot_FlashWrite(uint32_t addr, const uint8_t *data, uint32_t size)
{
    HAL_StatusTypeDef st, lock_st;
    uint32_t actual, i;
    uint16_t half;

    if (data == NULL || (addr & 1U) != 0U || !Flash_Allowed(addr, size))
        return HAL_ERROR;

    actual = size + (size & 1U);
    if (!Flash_Allowed(addr, actual))
        return HAL_ERROR;

    /* Precheck: no write to a programmed halfword. */
    for (i = 0U; i < actual; i += 2U)
        if (*(volatile const uint16_t *)(addr + i) != 0xFFFFU)
            return HAL_ERROR;

    st = HAL_FLASH_Unlock();
    if (st != HAL_OK)
        return st;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
    for (i = 0U; i < actual; i += 2U)
    {
        half = (uint16_t)data[i];
        half |= (uint16_t)((i + 1U < size ? data[i + 1U] : 0xFFU) << 8);
        if (half != 0xFFFFU)
        {
            st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, half);
            if (st != HAL_OK)
                break;
        }
        if (*(volatile const uint16_t *)(addr + i) != half)
        {
            st = HAL_ERROR;
            break;
        }
    }
    lock_st = HAL_FLASH_Lock();
    return (st == HAL_OK) ? lock_st : st;
}

HAL_StatusTypeDef Boot_FlashVerify(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t i;
    if (data == NULL || !Flash_Allowed(addr, size))
        return HAL_ERROR;
    for (i = 0U; i < size; i++)
        if (*(volatile const uint8_t *)(addr + i) != data[i])
            return HAL_ERROR;
    return HAL_OK;
}
