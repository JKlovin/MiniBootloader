#include "Flash.h"
#include <stddef.h>

/* =========================================================
 * 1. Flash 地址范围检查
 * ========================================================= */

/*
 * 判断指定地址范围是否允许擦写
 *
 * 返回：
 * 1：允许操作
 * 0：不允许操作
 */
static uint8_t Boot_FlashRangeCheck(
    uint32_t Address,
    uint32_t Size
)
{
    if (Size == 0U)
    {
        return 0U;
    }

#if (BOOT_ALLOW_APP_WRITE == 1U)

    /* APP 分区 */
    if ((Address >= APP_FLASH_ADDR) &&
        (Address < APP_FLASH_END_ADDR))
    {
        if (Size <= (APP_FLASH_END_ADDR - Address))
        {
            return 1U;
        }
    }

#endif

#if (BOOT_ALLOW_TEST_WRITE == 1U)

    /* 测试分区 */
    if ((Address >= BOOT_TEST_FLASH_ADDR) &&
        (Address < BOOT_TEST_FLASH_END))
    {
        if (Size <= (BOOT_TEST_FLASH_END - Address))
        {
            return 1U;
        }
    }

#endif

    return 0U;
}


/* =========================================================
 * 2. Flash 擦除
 * ========================================================= */

HAL_StatusTypeDef Boot_FlashErase(
    uint32_t Address,
    uint32_t Size
)
{
    FLASH_EraseInitTypeDef EraseInit = {0};

    HAL_StatusTypeDef Status;
    HAL_StatusTypeDef LockStatus;

    uint32_t PageError = 0xFFFFFFFFUL;
    uint32_t PageCount;
    uint32_t EraseSize;
    uint32_t Offset;
    uint16_t ReadData;

    /* 1. 检查请求的地址和大小 */
    if (Boot_FlashRangeCheck(Address, Size) == 0U)
    {
        return HAL_ERROR;
    }

    /* 2. 检查页对齐 */
    if ((Address % BOOT_FLASH_PAGE_SIZE) != 0U)
    {
        return HAL_ERROR;
    }

    /* 3. 计算需要擦除的页面数量 */
    PageCount =
        ((Size - 1U) / BOOT_FLASH_PAGE_SIZE) + 1U;

    /* 4. 计算实际擦除大小 */
    EraseSize = PageCount * BOOT_FLASH_PAGE_SIZE;

    /* 5. 检查向上取整后的实际擦除范围 */
    if (Boot_FlashRangeCheck(Address, EraseSize) == 0U)
    {
        return HAL_ERROR;
    }

    /* 6. 解锁 Flash */
    Status = HAL_FLASH_Unlock();

    if (Status != HAL_OK)
    {
        return Status;
    }

    /* 7. 清除错误及完成标志 */
    __HAL_FLASH_CLEAR_FLAG(
        FLASH_FLAG_EOP |
        FLASH_FLAG_PGERR |
        FLASH_FLAG_WRPERR
    );

    /* 8. 配置 Flash 擦除参数 */
    EraseInit.TypeErase =
        FLASH_TYPEERASE_PAGES;

    EraseInit.PageAddress = Address;
    EraseInit.NbPages = PageCount;

    /* 9. 开始擦除 */
    Status = HAL_FLASHEx_Erase(
        &EraseInit,
        &PageError
    );

    /* 10. 验证擦除结果 */
    if (Status == HAL_OK)
    {
        for (Offset = 0U;
             Offset < EraseSize;
             Offset += 2U)
        {
            ReadData = *(volatile uint16_t *)
                       (Address + Offset);

            if (ReadData != 0xFFFFU)
            {
                Status = HAL_ERROR;
                break;
            }
        }
    }

    /* 11. 重新锁定 Flash */
    LockStatus = HAL_FLASH_Lock();

    if (Status == HAL_OK)
    {
        Status = LockStatus;
    }

    return Status;
}


/* =========================================================
 * 3. Flash 数据写入
 * ========================================================= */

HAL_StatusTypeDef Boot_FlashWrite(
    uint32_t Address,
    const uint8_t *Buffer,
    uint32_t Size
)
{
    HAL_StatusTypeDef Status;
    HAL_StatusTypeDef LockStatus;

    uint32_t ProgramSize;
    uint32_t Offset;
    uint16_t Data;
    uint16_t ReadData;

    /* 1. 检查数据缓冲区 */
    if (Buffer == NULL)
    {
        return HAL_ERROR;
    }

    /* 2. 检查实际数据地址范围 */
    if (Boot_FlashRangeCheck(Address, Size) == 0U)
    {
        return HAL_ERROR;
    }

    /* 3. 半字编程要求地址 2 字节对齐 */
    if ((Address & 0x01U) != 0U)
    {
        return HAL_ERROR;
    }

    /* 4. 奇数长度补齐为偶数 */
    ProgramSize = Size + (Size & 0x01U);

    /* 5. 检查补齐后的实际写入范围 */
    if (Boot_FlashRangeCheck(Address, ProgramSize) == 0U)
    {
        return HAL_ERROR;
    }

    /*
     * 6. 编程前检查：
     * 确保整个目标区域已经处于擦除状态
     */
    for (Offset = 0U;
         Offset < ProgramSize;
         Offset += 2U)
    {
        ReadData = *(volatile uint16_t *)
                   (Address + Offset);

        if (ReadData != 0xFFFFU)
        {
            return HAL_ERROR;
        }
    }

    /* 7. 解锁 Flash */
    Status = HAL_FLASH_Unlock();

    if (Status != HAL_OK)
    {
        return Status;
    }

    /* 8. 清除相关标志 */
    __HAL_FLASH_CLEAR_FLAG(
        FLASH_FLAG_EOP |
        FLASH_FLAG_PGERR |
        FLASH_FLAG_WRPERR
    );

    /* 9. 按半字连续编程 */
    for (Offset = 0U;
         Offset < ProgramSize;
         Offset += 2U)
    {
        /* 低字节 */
        Data = (uint16_t)Buffer[Offset];

        /* 高字节 */
        if ((Offset + 1U) < Size)
        {
            Data |=
                ((uint16_t)Buffer[Offset + 1U] << 8);
        }
        else
        {
            /* 奇数长度时用 FF 补齐 */
            Data |= 0xFF00U;
        }

        /*
         * 跳过全 FF 半字：
         * 已擦除的数据无须重新编程
         */
        if (Data != 0xFFFFU)
        {
            Status = HAL_FLASH_Program(
                FLASH_TYPEPROGRAM_HALFWORD,
                Address + Offset,
                Data
            );

            if (Status != HAL_OK)
            {
                break;
            }
        }

        /* 写入后立即校验 */
        ReadData = *(volatile uint16_t *)
                   (Address + Offset);

        if (ReadData != Data)
        {
            Status = HAL_ERROR;
            break;
        }
    }

    /* 10. 锁定 Flash */
    LockStatus = HAL_FLASH_Lock();

    if (Status == HAL_OK)
    {
        Status = LockStatus;
    }

    return Status;
}


/* =========================================================
 * 4. Flash 数据校验
 * ========================================================= */

HAL_StatusTypeDef Boot_FlashVerify(
    uint32_t Address,
    const uint8_t *Buffer,
    uint32_t Size
)
{
    uint32_t Offset;
    uint8_t ReadData;

    /* 1. 参数检查 */
    if (Buffer == NULL)
    {
        return HAL_ERROR;
    }

    if (Boot_FlashRangeCheck(Address, Size) == 0U)
    {
        return HAL_ERROR;
    }

    /* 2. 逐字节比较 */
    for (Offset = 0U; Offset < Size; Offset++)
    {
        ReadData = *(volatile uint8_t *)
                   (Address + Offset);

        if (ReadData != Buffer[Offset])
        {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}
