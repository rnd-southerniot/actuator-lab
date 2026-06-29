#include <stdint.h>

extern int main(void);
extern void SystemInit(void);
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

void Reset_Handler(void);
void Default_Handler(void);

void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* Peripheral IRQs used by this firmware. Weak → safe if undefined; main.c provides the
 * strong definitions. Vector index = 16 + IRQn.
 * EXTI4_IRQn = 10 (PC4 = Ch A), EXTI9_5_IRQn = 23 (PC5 = Ch B AND PB7 = FS fault),
 * USART1_IRQn = 37 (console RX), TIM6_DAC_IRQn = 54 (1 kHz PID loop). */
void EXTI4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI9_5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM6_DAC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

__attribute__((section(".isr_vector"))) void (*const g_pfnVectors[])(void) = {
    [0] = (void (*)(void))(&_estack),
    [1] = Reset_Handler,
    [2] = NMI_Handler,
    [3] = HardFault_Handler,
    [4] = MemManage_Handler,
    [5] = BusFault_Handler,
    [6] = UsageFault_Handler,
    [11] = SVC_Handler,
    [12] = DebugMon_Handler,
    [14] = PendSV_Handler,
    [15] = SysTick_Handler,
    [16 ... 127] = Default_Handler,
    /* Override specific peripheral slots (these come AFTER the range, so they win). */
    [16 + 10] = EXTI4_IRQHandler,     /* EXTI4_IRQn    = 10 */
    [16 + 23] = EXTI9_5_IRQHandler,   /* EXTI9_5_IRQn  = 23 */
    [16 + 37] = USART1_IRQHandler,    /* USART1_IRQn   = 37 */
    [16 + 54] = TIM6_DAC_IRQHandler,  /* TIM6_DAC_IRQn = 54 */
};

void Reset_Handler(void) {
  SystemInit();

  uint32_t *src = &_sidata;
  uint32_t *dst = &_sdata;

  while (dst < &_edata) {
    *dst++ = *src++;
  }

  for (dst = &_sbss; dst < &_ebss; dst++) {
    *dst = 0;
  }

  (void)main();
  while (1) {
  }
}

void Default_Handler(void) {
  while (1) {
  }
}
