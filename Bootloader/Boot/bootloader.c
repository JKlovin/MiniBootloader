#include "bootloader.h"
#include "stm32f1xx_hal.h"
#include "usart.h"


/* 获取指针地址中的内容 */
static uint32_t Boot_ReadVector(uint32_t address)
{
	return *(volatile const uint32_t *)address;
}

uint8_t Boot_IsAppValid(void)
{
	uint32_t app_msp		= *(volatile const uint32_t *)(APP_FLASH_ADDR);
	uint32_t app_reset	= *(volatile const uint32_t *)(APP_FLASH_ADDR + 4UL);
	uint32_t code_addr	= app_reset & ~1UL;

	/* 判断堆栈指针范围 */
	if ((app_msp <= SRAM_START_ADDR) || (app_msp > SRAM_END_ADDR))
	{
			return 0U;
	}

	/* 堆栈指针8字节对齐 */
	if ((app_msp & 0x7UL) != 0UL)
	{
			return 0U;
	}

	/* 判断复位函数Thumb位 */
	if ((app_reset & 0x1UL) == 0UL)
	{
			return 0U;
	}

	/* 判断复位函数地址范围 */
	if ((code_addr < APP_FLASH_ADDR) || (code_addr >= APP_FLASH_END_ADDR))
	{
			return 0U;
	}

	return 1U;
}

__asm void Boot_JumpAssembly(uint32_t app_msp, uint32_t app_reset)
{
	/* 8字节对齐 */
	PRESERVE8

	/* 将R2寄存器清零 */
	MOVS r2, #0
	/* 将R2寄存器中的值赋值给CONTROL */
	/* CONTROM = 0x00000000 */
	/* 设置当前位特权线程模式，使用MSP指针 */
	MSR CONTROL, r2
	/* 指令同步 */
	ISB

	/* uint32_t app_msp  	---->R0 */
	/* uint32_t app_reset  ---->R1 */
	/* 将R0寄存器中的值赋值给CONTROL */
	/* 设置MSP指针，重新复位堆栈 */
	MSR MSP, r0
	ISB

	/* 开启中断 */
	CPSIE i
	/* 跳转到R1寄存器中的地址，即 app_reset */
	BX r1
}

void JumpToApp(void)
{
	uint32_t i;
	uint32_t app_msp		= Boot_ReadVector(APP_FLASH_ADDR)				;
	uint32_t app_reset	= Boot_ReadVector(APP_FLASH_ADDR + 4UL)	;

	/* 获取当前正在执行的异常编号，判断当前运行状态 */
	if (__get_IPSR() != 0U)
	{
			return;
	}

	/* 判断用户程序的地址状态 */
	if (Boot_IsAppValid() == 0U)
	{
			return;
	}

	/* 关闭中断 */
	HAL_NVIC_DisableIRQ(USART1_IRQn);
	if (HAL_UART_DeInit(&huart1) != HAL_OK)
	{
			return;
	}
	
	/* 复位时钟，后续重新配置 */
	if (HAL_RCC_DeInit() != HAL_OK)
	{
			return;
	}
	
	/* 复位外设配置 */
	if (HAL_DeInit() != HAL_OK)
	{
			return;
	}

	/* 关闭中断 */
	__disable_irq();

	/* 关闭Systick滴答定时器 */
	SysTick->CTRL = 0U;
	SysTick->LOAD = 0U;
	SysTick->VAL  = 0U;

	/* 清除中断寄存器 */
	for (i = 0U; i < 2U; ++i)
	{
			NVIC->ICER[i] = 0xFFFFFFFFUL;
			NVIC->ICPR[i] = 0xFFFFFFFFUL;
	}

	/* 清除系统异常中断 */
	SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;

	/* 清除中断优先级屏蔽阈值 */
	__set_BASEPRI(0U);
	__set_FAULTMASK(0U);

	/* 设置中断向量表偏移 */
	SCB->VTOR = APP_FLASH_ADDR;
	
	/* 内存同步 指令同步 */
	__DSB();
	__ISB();

	/* 用户程序跳转 */
	Boot_JumpAssembly(app_msp, app_reset);

	/* 指令不会执行 */
	while(1);
}





