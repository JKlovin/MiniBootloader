#include "boot_crc.h"

/* 计算CRC校验值 */
uint32_t Boot_CRC32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL, i;
    uint8_t j;
    for (i = 0U; i < len; ++i)
    {
        crc ^= data[i];
        for (j = 0U; j < 8U; ++j)
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320UL : 0U);
    }
    return crc ^ 0xFFFFFFFFUL;
}

uint16_t Boot_CRC16_Xmodem(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0U;
    uint32_t i;
    uint8_t j;
    for (i = 0U; i < len; ++i)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (j = 0U; j < 8U; ++j)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000U) ? 0x1021U : 0U));
    }
    return crc;
}
