#include "bootloader.h"
#include "stm32f1xx_hal.h"
#include "usart.h"

/* 镜像文件的中断向量表判断 */
uint8_t Boot_ImageVectorsValid(uint32_t image_base, uint32_t size)
{
    uint32_t msp, reset, code;
    if (size < 8U || size > BOOT_APP_SIZE)
        return 0U;
    msp = *(volatile const uint32_t *)image_base;
    reset = *(volatile const uint32_t *)(image_base + 4U);
    code = reset & ~1UL;
    if (msp <= BOOT_RAM_BASE || msp > BOOT_RAM_END || (msp & 7U) != 0U)
        return 0U;
    if ((reset & 1U) == 0U)
        return 0U;
    if (code < BOOT_APP_BASE || code >= BOOT_APP_BASE + size)
        return 0U;
    return 1U;
}

uint8_t Boot_IsAppValid(void)
{
    return Boot_ImageVectorsValid(BOOT_APP_BASE, BOOT_APP_SIZE);
}


__asm void Boot_JumpAssembly(uint32_t new_msp, uint32_t new_pc)
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
	
	DSB
	ISB

	/* 开启中断 */
	CPSIE i
	/* 跳转到R1寄存器中的地址，即 app_reset */
	BX r1
}

void JumpToApp(void)
{
    uint32_t i, msp, reset;
	
		/* 获取当前正在执行的异常编号，判断当前运行状态 */
		/* 并判断用户程序的地址状态 */
    if (__get_IPSR() != 0U || Boot_IsAppValid() == 0U)
        return;
		
    msp = *(volatile const uint32_t *)BOOT_APP_BASE;
    reset = *(volatile const uint32_t *)(BOOT_APP_BASE + 4U);

		/* 关闭串口中断 */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    if (HAL_UART_DeInit(&huart1) != HAL_OK)
        return;
		
		/* 复位时钟，后续重新配置 */
    if (HAL_RCC_DeInit() != HAL_OK)
        return;
		
		/* 复位外设配置 */
    if (HAL_DeInit() != HAL_OK)
        return;

		/* 关闭中断 */
    __disable_irq();
		
		/* 关闭Systick滴答定时器 */
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
		
		/* 清除中断寄存器 */
    for (i = 0U; i < 2U; i++)
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
    SCB->VTOR = BOOT_APP_BASE;
		
		/* 内存同步 指令同步 */
    __DSB();
    __ISB();
		
		/* 用户程序跳转 */
    Boot_JumpAssembly(msp, reset);
		
		/* 指令不会执行 */
		while(1);
} 



