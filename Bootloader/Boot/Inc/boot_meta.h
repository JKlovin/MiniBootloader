#ifndef BOOT_META_H
#define BOOT_META_H
#include "boot_flash.h"

#define BOOT_META_MAGIC  0x3141544DUL   /* 'MTA1' */
#define BOOT_META_MARK   0xA55AU

typedef struct
{
    uint32_t magic;
    uint32_t size;
    uint32_t crc32;
    uint32_t version;
    uint32_t header_crc;
    uint16_t ready;
    uint16_t started;
    uint16_t done;
    uint16_t reserved16;
    uint32_t reserved32;
} BootMeta_t;  /* 32 bytes */

typedef enum
{
    BOOT_META_EMPTY = 0,
    BOOT_META_VALID = 1,
    BOOT_META_INVALID = 2
} BootMetaState_t;

BootMetaState_t Boot_MetaLatest(BootMeta_t *out, uint32_t *record_addr);
uint8_t Boot_MetaHasRoom(void);
HAL_StatusTypeDef Boot_MetaAppend(uint32_t size, uint32_t crc32, uint32_t version);
HAL_StatusTypeDef Boot_MetaSetStarted(uint32_t record_addr);
HAL_StatusTypeDef Boot_MetaSetDone(uint32_t record_addr);
#endif
