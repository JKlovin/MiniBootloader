#include "bootloader.h"
#include "stm32f1xx_hal.h"
#include "usart.h"

uint8_t Boot_ImageVectorsValid(uint32_t image_base, uint32_t size)
{
    uint32_t msp, reset, code;
    if (size < 8U || size > BOOT_APP_SIZE) return 0U;
    msp = *(volatile const uint32_t *)image_base;
    reset = *(volatile const uint32_t *)(image_base + 4U);
    code = reset & ~1UL;
    if (msp <= BOOT_RAM_BASE || msp > BOOT_RAM_END || (msp & 7U) != 0U)
        return 0U;
    if ((reset & 1U) == 0U) return 0U;
    if (code < BOOT_APP_BASE || code >= BOOT_APP_BASE + size) return 0U;
    return 1U;
}

uint8_t Boot_IsAppValid(void)
{
    return Boot_ImageVectorsValid(BOOT_APP_BASE, BOOT_APP_SIZE);
}

/* ARM Compiler 5: switch MSP only in naked-style assembly. */
__asm void Boot_JumpAssembly(uint32_t new_msp, uint32_t new_pc)
{
    PRESERVE8
    MOVS r2, #0
    MSR CONTROL, r2
    ISB
    MSR MSP, r0
    ISB
    CPSIE i
    BX r1
}

void JumpToApp(void)
{
    uint32_t i, msp, reset;
    if (__get_IPSR() != 0U || Boot_IsAppValid() == 0U) return;
    msp = *(volatile const uint32_t *)BOOT_APP_BASE;
    reset = *(volatile const uint32_t *)(BOOT_APP_BASE + 4U);

    HAL_NVIC_DisableIRQ(USART1_IRQn);
    if (HAL_UART_DeInit(&huart1) != HAL_OK) return;
    if (HAL_RCC_DeInit() != HAL_OK) return;
    if (HAL_DeInit() != HAL_OK) return;

    __disable_irq();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
    for (i = 0U; i < 2U; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    __set_BASEPRI(0U);
    __set_FAULTMASK(0U);
    SCB->VTOR = BOOT_APP_BASE;
    __DSB();
    __ISB();
    Boot_JumpAssembly(msp, reset);
    while (1) { }
}
