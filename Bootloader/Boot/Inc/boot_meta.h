#ifndef BOOT_META_H
#define BOOT_META_H
#include "boot_flash.h"

#define BOOT_META_MAGIC  0x3141544DUL   /* 'BUP1' */
#define BOOT_META_MARK   0xA55AU

/* 固件升级信息记录表，状态记录表 */
typedef struct
{
	uint32_t magic;				//用于识别这是不是一条有效格式的升级记录
	uint32_t size;				//新固件的真实字节数
	uint32_t crc32;				//新固件数据的 CRC32 校验值
	uint32_t version;			//固件版本号
	uint32_t header_crc;	//前四个字段的 CRC32，用于检测元数据头是否损坏

	/* 状态标志位 */
	uint16_t ready;				//是否已提交固件
	uint16_t started;			//是否已开始安装
	uint16_t done;				//是否已完成安装

	/* 预留 */
	uint16_t reserved16;
	uint32_t reserved32;
} BootMeta_t;  /* 32 bytes */

/* 检测固件信息并返回标志位 */
typedef enum
{
	BOOT_META_EMPTY 		= 0,	//META区域为空，可能是第一次运行
	BOOT_META_VALID 		= 1,	//找到有效元数据，可以进一步检查固件CRC和升级状态
	BOOT_META_INVALID 	= 2		//存在数据，但没有有效元数据，需要进行异常处理
} BootMetaState_t;

BootMetaState_t Boot_MetaLatest(BootMeta_t *out, uint32_t *record_addr);
uint8_t Boot_MetaHasRoom(void);
HAL_StatusTypeDef Boot_MetaAppend(uint32_t size, uint32_t crc32, uint32_t version);
HAL_StatusTypeDef Boot_MetaSetStarted(uint32_t record_addr);
HAL_StatusTypeDef Boot_MetaSetDone(uint32_t record_addr);
#endif
