#ifndef BOOT_CRC_H
#define BOOT_CRC_H
#include <stdint.h>
uint32_t Boot_CRC32(const uint8_t *data, uint32_t len);
uint16_t Boot_CRC16_Xmodem(const uint8_t *data, uint32_t len);
#endif
