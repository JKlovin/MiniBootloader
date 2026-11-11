#include "boot_upgrade.h"
#include "bootloader.h"
#include "boot_flash.h"
#include "boot_crc.h"
#include "boot_meta.h"
#include <string.h>

#define XM_SOH 0x01U
#define XM_EOT 0x04U
#define XM_ACK 0x06U
#define XM_NAK 0x15U
#define XM_CAN 0x18U
#define XM_CRC_REQUEST 0x43U
#define XM_FRAME_SIZE 132U /* after SOH: seq + inv + 128 + CRC16 */
#define XM_MAX_ERRORS 10U

/* 用于可靠性实验 */
#define BOOT_TEST_RESET_AFTER_ERASE 0U
#define BOOT_TEST_RESET_AFTER_FIRST_BLOCK 0U

/* 定义使用串口 */
static UART_HandleTypeDef *s_uart;

static void Boot_MetaDump(void);

static void Boot_SendByte(uint8_t b)
{
    (void)HAL_UART_Transmit(s_uart, &b, 1U, 1000U);
}

/* 结束传输 */
static void Boot_CancelTransfer(void)
{
    Boot_SendByte(XM_CAN);
    Boot_SendByte(XM_CAN);
}

static void Boot_Print(const char *s)
{
    (void)HAL_UART_Transmit(s_uart, (uint8_t *)s,
                            (uint16_t)strlen(s), 2000U);
}

static uint32_t Boot_ReadLE32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void Boot_PrintHex32(uint32_t v)
{
    static const char digits[] = "0123456789ABCDEF";
    char out[11] = "0x00000000";
    uint32_t i;
    for (i = 0U; i < 8U; ++i)
        out[i + 2U] = digits[(v >> (28U - 4U * i)) & 15U];
    Boot_Print(out);
}

/* A .bup file starts with 24 bytes: magic, app_addr, length, crc, version, header_crc. */
typedef struct
{
    uint32_t size;
    uint32_t crc;
    uint32_t version;
} BootPackage_t;

/* BUP格式检查 */
static uint8_t Boot_ParsePackage(const uint8_t *p, BootPackage_t *out)
{
    if (Boot_ReadLE32(p) != BOOT_PACKAGE_MAGIC)
        return 0U;
    if (Boot_ReadLE32(p + 4U) != BOOT_APP_BASE)
        return 0U;
    out->size = Boot_ReadLE32(p + 8U);
    out->crc = Boot_ReadLE32(p + 12U);
    out->version = Boot_ReadLE32(p + 16U);
    if (Boot_ReadLE32(p + 20U) != Boot_CRC32(p, 20U))
        return 0U;
    if (out->size < 8U || out->size > BOOT_MAX_IMAGE)
        return 0U;
    return 1U;
}

/* 暂存区代码检查 */
static HAL_StatusTypeDef Boot_CheckStage(const BootMeta_t *m)
{
    if (m->size < 8U || m->size > BOOT_MAX_IMAGE)
        return HAL_ERROR;
		
    if (!Boot_ImageVectorsValid(BOOT_STAGE_BASE, m->size))
        return HAL_ERROR;
		
    return (Boot_CRC32((const uint8_t *)BOOT_STAGE_BASE, m->size) == m->crc32) ? HAL_OK : HAL_ERROR;
}

/* 用户程序CRC校验 */
static HAL_StatusTypeDef Boot_CheckApp(const BootMeta_t *m)
{
    /* 检查固件大小 */
    if (m->size < 8U || m->size > BOOT_MAX_IMAGE)
        return HAL_ERROR;

    /* 检查向量表，检查MSP和ReseHandler */
    if (!Boot_ImageVectorsValid(BOOT_APP_BASE, m->size))
        return HAL_ERROR;

    /* 检查实际固件 */
    return (Boot_CRC32((const uint8_t *)BOOT_APP_BASE, m->size) == m->crc32) ? HAL_OK : HAL_ERROR;
}

/* 用户程序烧写 */
static HAL_StatusTypeDef Boot_Install(const BootMeta_t *m, uint32_t meta_addr)
{
    uint8_t block[128];
    uint32_t offset, n;

	/* 检查暂存区代码有没有问题 */
    if (Boot_CheckStage(m) != HAL_OK)
        return HAL_ERROR;
	
	/* 设置Meta区 Start 标志位，启动用户代码烧写 */
    if (Boot_MetaSetStarted(meta_addr) != HAL_OK)
        return HAL_ERROR;

    /* 检查用户代码有没有问题 */
    /* 当前代码与需要烧写的新代码是不是同一份 */
    if (Boot_CheckApp(m) == HAL_OK)
        return Boot_MetaSetDone(meta_addr);

    /* 擦除用户代码区 */
    if (Boot_FlashErase(BOOT_APP_BASE, BOOT_APP_SIZE) != HAL_OK)
        return HAL_ERROR;

		/* =====================================================
		 * 测试：模拟 APP 擦除之后发生断电复位
		 * ===================================================== */
		#if (BOOT_TEST_RESET_AFTER_ERASE == 1U)

				if (m->started == 0xFFFFU)
				{
						Boot_Print(
								"\r\n[TEST] APP erased, reset now!\r\n");

						/* 模拟突然复位 */
						NVIC_SystemReset();
				}

		#endif
		
		/* 烧写用户程序 */
    for (offset = 0U; offset < m->size; offset += n)
    {
        n = m->size - offset;
        if (n > sizeof(block))
            n = sizeof(block);
        memcpy(block, (const void *)(BOOT_STAGE_BASE + offset), n);
        if (Boot_FlashWrite(BOOT_APP_BASE + offset, block, n) != HAL_OK)
            return HAL_ERROR;

				/* 测试：第一次安装写入首个数据块后复位 */
				#if (BOOT_TEST_RESET_AFTER_FIRST_BLOCK == 1U)

								if ((offset == 0U) && (m->started == 0xFFFFU))
								{
										Boot_Print(
												"\r\n[TEST] Partial APP written, reset!\r\n");

										NVIC_SystemReset();

										while (1)
										{
										}
								}

				#endif
						}
		
    /* 检查用户代码有没有问题 */
    if (Boot_CheckApp(m) != HAL_OK)
        return HAL_ERROR;
    return Boot_MetaSetDone(meta_addr);
}

/* Bootloader跳转判断 */
static uint8_t Boot_CanJump(void)
{
    BootMeta_t m;
    BootMetaState_t s = Boot_MetaLatest(&m, NULL);
	
		/* Meta信息记录空白，没记录过更新信息 */
    if (s == BOOT_META_EMPTY)
        return Boot_IsAppValid();
		
		/* Meta信息记录有误 */
    if (s != BOOT_META_VALID)
        return 0U;
		
		/* 用户程序更新完成 */
    return (uint8_t)((m.done != 0xFFFFU) && (Boot_CheckApp(&m) == HAL_OK));
}

/* 固件升级状态判断与恢复 */
static void Boot_Recover(void)
{
    BootMeta_t m;
    uint32_t addr;

    /* 查找之前保存的有效升级记录 */
    BootMetaState_t s = Boot_MetaLatest(&m, &addr);

    /* Boot_Meta区没有固件记录 */
    if (s == BOOT_META_EMPTY)
        return;

    /* Boot_Meta区有误(记录头写坏 / Flash 位损坏)，需要异常处理 */
    if (s == BOOT_META_INVALID)
    {
        Boot_Print("\r\n[WARN] Invalid metadata; APP auto-boot disabled\r\n");
        return;
    }

    /* 上次升级已完整收尾。进行一次 APP 的CRC校验*/
    if ((m.done != 0xFFFFU) && Boot_CheckApp(&m) == HAL_OK)
        return;
    Boot_Print("\r\n[RECOVERY] Installing verified staged image...\r\n");

    /* 重新执行固件安装 */
    if (Boot_Install(&m, addr) == HAL_OK)
        Boot_Print("[RECOVERY] PASS\r\n");
    else
        Boot_Print("[RECOVERY] FAIL. Remain in bootloader.\r\n");
}

/* 将固件烧写到暂存区 */
static HAL_StatusTypeDef Boot_ReceiveXmodem(void)
{
    uint8_t frame[XM_FRAME_SIZE];
    uint8_t ch, expected = 1U, first = 1U;
    uint32_t received = 0U, bad = 0U, handshake = 0U;
    uint32_t offset, n, copy_size;
    uint16_t rx_crc;
    BootPackage_t image = {0};

    /* 检查 META 分区还有没有空间写入新的升级记录 */
    if (!Boot_MetaHasRoom())
        return HAL_ERROR;

    /* 清空暂存区，准备烧写固件 */
    if (Boot_FlashErase(BOOT_STAGE_BASE, BOOT_STAGE_SIZE) != HAL_OK)
        return HAL_ERROR;

		/* 串口发送 'C' */
    Boot_SendByte(XM_CRC_REQUEST);
		
    while (1)
    {
			/* 如果超时时间 1s 下还没有接收到一个字节 */
        if (HAL_UART_Receive(s_uart, &ch, 1U, 1000U) != HAL_OK)
        {
            if (first != 0U)	/* 第一次握手期间 */
            {
                if (++handshake > 30U)
                    break;
								/* 串口发送 'C' */
                Boot_SendByte(XM_CRC_REQUEST);
            }
            else	/* 传输中途 */
            {
							/* 错误计数累计上限，超过就异常返回 */
                if (++bad > XM_MAX_ERRORS)
                    break;
                Boot_SendByte(XM_NAK);
            }
            continue;
        }

				/* 帧格式 */
				/* 帧头        序列号        序列号反码      		数据段       			校验位 	*/
				/* SOH    +    1 seq    +    1 ~seq    +    128 data    +    2 CRC16 			*/
				/* 				    frame[0]	     frame[1]       frame[...]    frame[130,131]	*/
				
				/* 对方主动放弃传输 */
        if (ch == XM_CAN)
            return HAL_ERROR;
				
				/* 正常结束信号 */
        if (ch == XM_EOT)
        {
						/* first != 0    			一帧都没收到就结束了，传输异常 */
						/* received != size   实收字节数不等于包头声明值，传输异常 */
            if (first != 0U || received != image.size)
                break;
						
						/* 暂存区代码整体CRC校验 */
						/* 暂存区代码中断向量表和字节大小校验 */						
            if (Boot_CRC32((const uint8_t *)BOOT_STAGE_BASE, image.size) != image.crc || !Boot_ImageVectorsValid(BOOT_STAGE_BASE, image.size))
                break;
						
						/* 添加Meta信息 */
            if (Boot_MetaAppend(image.size, image.crc, image.version) != HAL_OK)
                break;
						
						/* ACK应答 */
            Boot_SendByte(XM_ACK);
            return HAL_OK;
        }
				
				/* 数据帧开始 */
        if (ch != XM_SOH)
        {
            if (++bad > XM_MAX_ERRORS)
                break;
            Boot_SendByte(XM_NAK);
            continue;
        }

				/* 阻塞式接收一整帧数据 */
        if (HAL_UART_Receive(s_uart, frame, XM_FRAME_SIZE, 2000U) != HAL_OK)
            break;

				/* CRC校验值 */
        rx_crc = (uint16_t)(((uint16_t)frame[130] << 8) | frame[131]);
				
				/* 帧数据CRC校验 */
        if (((uint8_t)(frame[0] ^ frame[1]) != 0xFFU) || (rx_crc != Boot_CRC16_Xmodem(&frame[2], 128U)))
        {
            if (++bad > XM_MAX_ERRORS)
                break;
						
            Boot_SendByte(XM_NAK);
            continue;
        }

				/* 上一次的 ACK 丢了，对方重发了同一帧 */
        if (!first && frame[0] == (uint8_t)(expected - 1U))
        {
            Boot_SendByte(XM_ACK);
            continue;
        }
				
				/* 数据包错乱 */
        if (frame[0] != expected || (received != 0U && received == image.size))
        {
            if (++bad > XM_MAX_ERRORS)
                break;
            Boot_SendByte(XM_NAK);
            continue;
        }

        offset = 0U;
				
				/* 判断是否首帧数据 */
				/* 首帧		payload[128]	= [ 头部 24B ][ 固件前 104B ] */
				/* 后续帧	payload[128]	= [ 固件 128B ] 							*/
        if (first != 0U)
        {
					/* BUP格式和信息判断 */
            if (!Boot_ParsePackage(&frame[2], &image))
                break;
						
            offset = BOOT_PACKAGE_HEADER;
            first = 0U;
        }
				
				/* 本帧可用字节数 */
        n = 128U - offset;
				
				/* 还差多少没写 */
        copy_size = image.size - received;
				
        if (copy_size > n)
            copy_size = n;
				
        if (copy_size == 0U)
            break;
				
        if (Boot_FlashWrite(BOOT_STAGE_BASE + received,&frame[2U + offset], copy_size) != HAL_OK)
            break;
				
				/* 已烧写固件数据 */
        received += copy_size;
				/* 下一个期待收到的帧序号 */
        expected++;
				/* 清理错误计数 */
        bad = 0U;
				/* 应答 */
        Boot_SendByte(XM_ACK);
    }
    Boot_CancelTransfer();
    return HAL_ERROR;
}

static void Boot_Menu(void)
{
    Boot_Print("\r\n=== Bootloader / 16K+24K+23K+1K ===\r\n");
    Boot_Print("u: XMODEM-CRC upgrade (.bup, 128-byte packets)\r\n");
    Boot_Print("j: Jump to verified APP\r\n");
    Boot_Print("i: Partition/status information\r\n");
    Boot_Print("h: Menu\r\n");
    Boot_Print("m: Dump upgrade metadata\r\n");
}

/* 显示 APP/STAGE/META 分区信息，读取最新元数据并报告固件 CRC32、大小和启动许可 */
static void Boot_Info(void)
{
    BootMeta_t m;
    BootMetaState_t s = Boot_MetaLatest(&m, NULL);
    Boot_Print("APP   0x08004000 (24 KiB)\r\n");
    Boot_Print("STAGE 0x0800A000 (23 KiB)\r\n");
    Boot_Print("META  0x0800FC00 ( 1 KiB)\r\n");
    if (s == BOOT_META_EMPTY)
        Boot_Print("Metadata: EMPTY\r\n");
    else if (s == BOOT_META_INVALID)
        Boot_Print("Metadata: INVALID\r\n");
    else
    {
        Boot_Print("Image bytes: ");
        Boot_PrintHex32(m.size);
        Boot_Print(" CRC32: ");
        Boot_PrintHex32(m.crc32);
        Boot_Print((m.done != 0xFFFFU) ? " DONE\r\n" : " PENDING\r\n");
    }
    Boot_Print(Boot_CanJump() ? "APP launch: ALLOWED\r\n" : "APP launch: BLOCKED\r\n");
}

void Boot_Run(UART_HandleTypeDef *uart)
{
    uint8_t ch;
    uint32_t boot_start;
    BootMeta_t m;
    uint32_t addr;

    s_uart = uart;
    Boot_Print("\r\nBootloader starting...\r\n");
	
	/* 状态恢复 */
    Boot_Recover();

    /* Bootloader操作菜单 */
    Boot_Menu();
    Boot_Print("Send 'u' within 5 s, otherwise valid APP runs.\r\n");

    /* 记录当前时刻 */
    boot_start = HAL_GetTick();

    while (1)
    {
			/* 等待一段时间后自动跳转用户程序 */
        if (Boot_CanJump() &&
            (uint32_t)(HAL_GetTick() - boot_start) >= BOOT_AUTOJUMP_MS)
        {
            Boot_Print("Jumping to APP...\r\n");
            JumpToApp();
            boot_start = HAL_GetTick();
        }

        /* 串口通信 */
        if (HAL_UART_Receive(s_uart, &ch, 1U, 50U) != HAL_OK)
            continue;
        if (ch == 'h' || ch == 'H')
            /* 显示菜单 */
            Boot_Menu();

        else if (ch == 'i' || ch == 'I')
            /* 显示Boot详细信息 */
            Boot_Info();

        else if (ch == 'j' || ch == 'J')
        {
            if (Boot_CanJump())
                /* 跳转程序 */
                JumpToApp();
            else
                Boot_Print("APP not verified; jump refused.\r\n");
        }
        else if (ch == 'u' || ch == 'U')
        {
            Boot_Print("Start XMODEM-CRC sender now (128-byte mode).\r\n");

            /* 固件下载暂存 */
            if (Boot_ReceiveXmodem() == HAL_OK)
            {
                Boot_Print("\r\nDownload verified. Installing...\r\n");
                if (Boot_MetaLatest(&m, &addr) == BOOT_META_VALID && Boot_Install(&m, addr) == HAL_OK)
                    Boot_Print("Install PASS. APP ready.\r\n");
                else
                    Boot_Print("Install FAIL; recovery on next boot.\r\n");
            }
            else
                Boot_Print("\r\nDownload FAIL; old APP untouched by download.\r\n");
            boot_start = HAL_GetTick();
        }
        else if (ch == 'm' || ch == 'M')
        {
            Boot_MetaDump();
        }
    }
}

/* 检查元数据记录槽是否为空 */
static uint8_t Boot_MetaSlotEmpty(uint32_t Address)
{
    uint32_t Offset;

    for (Offset = 0U;
         Offset < sizeof(BootMeta_t);
         Offset += 2U)
    {
        if (*(volatile const uint16_t *)(Address + Offset) != 0xFFFFU)
        {
            return 0U;
        }
    }

    return 1U;
}

/* 输出 Flash 升级状态区中的记录 */
static void Boot_MetaDump(void)
{
    uint32_t i;
    uint32_t Address;
    uint32_t UsedCount = 0U;
    uint32_t LatestAddress = 0U;

    BootMeta_t Record;
    BootMeta_t Latest;

    BootMetaState_t State;

    Boot_Print("\r\n=== Metadata Dump ===\r\n");

    for (i = 0U;
         i < BOOT_META_SIZE / sizeof(BootMeta_t);
         i++)
    {
        Address = BOOT_META_BASE +
                  i * sizeof(BootMeta_t);

        if (Boot_MetaSlotEmpty(Address))
        {
            continue;
        }

        UsedCount++;

        memcpy(&Record,
               (const void *)Address,
               sizeof(BootMeta_t));

        Boot_Print("\r\nSlot: ");
        Boot_PrintHex32(i);

        Boot_Print(" Address: ");
        Boot_PrintHex32(Address);

        Boot_Print("\r\nSize: ");
        Boot_PrintHex32(Record.size);

        Boot_Print(" CRC32: ");
        Boot_PrintHex32(Record.crc32);

        Boot_Print("\r\nVersion: ");
        Boot_PrintHex32(Record.version);

        Boot_Print("\r\nREADY: ");
        Boot_PrintHex32(Record.ready);

        Boot_Print(" STARTED: ");
        Boot_PrintHex32(Record.started);

        Boot_Print(" DONE: ");
        Boot_PrintHex32(Record.done);

        /* 校验升级记录头部 */
        if ((Record.magic == BOOT_META_MAGIC) &&
            (Record.size >= 8U) &&
            (Record.size <= BOOT_MAX_IMAGE) &&
            (Record.ready == BOOT_META_MARK) &&
            (Record.header_crc ==
             Boot_CRC32(
                 (const uint8_t *)&Record,
                 16U)))
        {
            Boot_Print("\r\nHeader: VALID\r\n");
        }
        else
        {
            Boot_Print("\r\nHeader: INVALID\r\n");
        }
    }

    /* 获取目前识别出的最新有效记录 */
    State = Boot_MetaLatest(
        &Latest,
        &LatestAddress);

    Boot_Print("\r\nUsed Slots: ");
    Boot_PrintHex32(UsedCount);

    Boot_Print("\r\nLatest State: ");

    if (State == BOOT_META_VALID)
    {
        Boot_Print("VALID");

        Boot_Print("\r\nLatest Address: ");
        Boot_PrintHex32(LatestAddress);
    }
    else if (State == BOOT_META_EMPTY)
    {
        Boot_Print("EMPTY");
    }
    else
    {
        Boot_Print("INVALID");
    }

    Boot_Print("\r\nMetadata Room: ");
    Boot_Print(Boot_MetaHasRoom() ? "YES\r\n" : "NO\r\n");
}
