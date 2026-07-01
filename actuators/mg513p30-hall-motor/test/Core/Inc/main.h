#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/*
 * REV Core Hex Motor — bench firmware pin map (single source of truth in firmware).
 * Audited 2026-06-29 against rnd-southerniot/st-discovery docs/stm32f429i-disc1-pin-mapping.md.
 * See ../../WIRING.md / ../../SPECS.md for rationale + datasheet citations.
 *
 * Board: STM32F429I-DISC1 (STM32F429ZI, 180 MHz).
 * Driver: Waveshare RPi Motor Driver Board (2x MC33886), Motor-A channel.
 *
 * Step 1 (this scaffold) configures ONLY the safe-idle outputs + heartbeat LED.
 * Encoder / FS / INA238-I2C3 / USART1 / control ISR arrive in Step 2+.
 */

/* --- Motor-A drive (sign-magnitude): PWMA=enable, M1/M2=direction --- */
#define MOTOR_PWM_PORT          GPIOB
#define MOTOR_PWM_PIN           GPIO_PIN_4   /* PB4  -> board PWMA (TIM3_CH1 in Step 2; GPIO-LOW here) */
#define MOTOR_DIR_M1_PORT       GPIOA
#define MOTOR_DIR_M1_PIN        GPIO_PIN_5   /* PA5  -> board M1 (direction)  */
#define MOTOR_DIR_M2_PORT       GPIOA
#define MOTOR_DIR_M2_PIN        GPIO_PIN_7   /* PA7  -> board M2 (direction)  */

/* --- Driver fault: MC33886 FS, open-drain active-LOW, 1k->5V. PB7 is 5V-tolerant. --- */
#define MOTOR_FS_PORT           GPIOB
#define MOTOR_FS_PIN            GPIO_PIN_7   /* PB7  <- board FS1 pad (LOW = fault) */
#define FS_DEBOUNCE_TICKS       3U           /* FS must read LOW on N consecutive 1 kHz polls to latch
                                              * a fault — rejects EMI glitches on the unfiltered PB7
                                              * line; a real (held-LOW) MC33886 fault latches in N ms */

/* --- Quadrature encoder (EXTI both-edges + TIM2 timestamp velocity in Step 2) --- */
#define ENC_A_PORT              GPIOC
#define ENC_A_PIN               GPIO_PIN_4   /* PC4 <- encoder Ch A */
#define ENC_B_PORT              GPIOC
#define ENC_B_PIN               GPIO_PIN_5   /* PC5 <- encoder Ch B */

/* --- INA238 current/voltage monitor on I2C3 (shared touch bus; INA238 @ 0x40) --- */
#define INA238_I2C_INSTANCE     I2C3
#define INA238_I2C_SCL_PORT     GPIOA
#define INA238_I2C_SCL_PIN      GPIO_PIN_8   /* PA8 = I2C3_SCL */
#define INA238_I2C_SDA_PORT     GPIOC
#define INA238_I2C_SDA_PIN      GPIO_PIN_9   /* PC9 = I2C3_SDA */
#define INA238_I2C_ADDR_7B      0x40U        /* A1=GND, A0=GND (Table 6-2); avoids STMPE811 @ 0x41 */

/* --- Telemetry: USART1 115200 8N1 --- */
#define TELEM_UART_INSTANCE     USART1
#define TELEM_UART_TX_PORT      GPIOA
#define TELEM_UART_TX_PIN       GPIO_PIN_9   /* PA9  = USART1_TX */
#define TELEM_UART_RX_PORT      GPIOA
#define TELEM_UART_RX_PIN       GPIO_PIN_10  /* PA10 = USART1_RX */

/* --- On-board status LEDs (st-discovery pin map) --- */
#define LED_HEARTBEAT_PORT      GPIOG
#define LED_HEARTBEAT_PIN       GPIO_PIN_13  /* LD3 green */
#define LED_FAULT_PORT          GPIOG
#define LED_FAULT_PIN           GPIO_PIN_14  /* LD4 red */

/* --- Timing config --- */
/* TIM2 & TIM3 are on APB1: at SYSCLK 180 MHz with APB1 prescaler /4, the APB1 timer
 * clock is 90 MHz (x2 rule). */
#define TIM_APB1_HZ             90000000UL
/* PWMA drives the MC33886 ENABLE; that bridge can't switch its enable at 20 kHz (Waveshare's
 * own demo PWMs it at 500 Hz). Confirmed on the bench: 20 kHz → bridge never enables, outputs
 * float to +12 V, no drive; DC/low-freq enable → motor runs. Run ~1 kHz via a prescaler
 * (TIM3 is 16-bit, so 90 MHz/1 kHz won't fit in ARR without one). */
#define PWM_FREQ_HZ             1000UL
#define PWM_TIM_CLK_HZ          1000000UL                     /* TIM3 clock after prescaler = 1 MHz */
#define PWM_PSC                 ((TIM_APB1_HZ / PWM_TIM_CLK_HZ) - 1U)  /* = 89 */
#define PWM_ARR                 ((PWM_TIM_CLK_HZ / PWM_FREQ_HZ) - 1U)  /* = 999 → 1 kHz, 1000 duty steps */
#define TIM2_TICK_HZ            1000000UL                     /* 1 µs edge timestamps (T-method) */
#define ENC_COUNTS_PER_OUT_REV  1456                          /* BENCH-MEASURED @P4 2026-07-02: 1455 (1-rev) & 1466 (5-rev) → 1456 = 13 PPR × 4 × 28:1 (actual gear ≈28, NOT nominal 30) */
#define ENC_VEL_STALE_US        100000UL                      /* >100 ms since last edge ⇒ vel = 0 */
#define VEL_MAX_CPS             11000L                        /* sanity clamp; free speed ≈ 1560×392/60 ≈ 10200 cps @ 12 V */
#define VEL_LPF_ALPHA           0.10f                         /* EMA on velocity feedback (~16 Hz @ 1 kHz loop) */
#define POS_VEL_CAP_CPS         4000L                         /* MODE_POS cascade: max inner velocity setpoint (~165 RPM) */
#define POS_DEADBAND_COUNTS     12L                           /* MODE_POS: |err|≤this ⇒ hold (no hunt); ~3° at 1456 CPR */
#define STALL_DUTY_MIN          850L                          /* stall detect: near-max commanded duty ... */
#define STALL_MOVE_COUNTS       80L                           /* ... yet the encoder moves < this over the window ... */
#define STALL_LATCH_TICKS       500U                          /* ... i.e. near-stationary for N ms ⇒ locked rotor → fault. Position-windowed (robust to velocity noise from a hand-held stall); N > worst-case breakaway so normal starts don't false-trip. Uses encoder+duty, not the PWM-noisy INA238. */
#define TELEM_PERIOD_MS         100U                          /* STATUS line cadence */
#define CTRL_ISR_HZ             1000U                         /* TIM6 PID loop rate */

/* --- INA238 current/voltage monitor (I2C3 @ 0x40) — datasheet SLYS025B --- */
#define INA238_I2C_ADDR8        (INA238_I2C_ADDR_7B << 1)     /* HAL uses 8-bit addr */
#define INA238_REG_CONFIG       0x00U
#define INA238_REG_ADC_CONFIG   0x01U
#define INA238_REG_SHUNT_CAL    0x02U
#define INA238_REG_VSHUNT       0x04U
#define INA238_REG_VBUS         0x05U
#define INA238_REG_DIETEMP      0x06U
#define INA238_REG_CURRENT      0x07U
#define INA238_REG_MANUF_ID     0x3EU
#define INA238_MANUF_ID         0x5449U   /* ASCII "TI" — presence check */
#define INA238_CONFIG_VALUE     0x0000U   /* ADCRANGE=0 → ±163.84 mV (≤10 A). Adafruit #6349 onboard
                                             15 mΩ: 4.4 A stall = 66 mV would CLIP the ±40.96 mV range */
#define INA238_SHUNT_CAL_VALUE  3072U     /* Adafruit 15 mΩ, CURRENT_LSB 250 µA, ADCRANGE=0
                                             (= 819.2e6 × 250e-6 × 0.015); current_mA = reg/4 unchanged */
/* Readback scaling (chosen so both are exact small-integer ops):
 *   current_mA = (int16)CURRENT × 0.25  = CURRENT / 4
 *   bus_mV     = VBUS × 3.125 mV         = VBUS × 25 / 8                                        */

/* --- Control / safety --- */
#define JOG_MAX_MS              2000U                         /* bounded open-loop jog window */
#define PID_OUT_MAX             ((int32_t)PWM_ARR)            /* duty magnitude clamp = 100% */

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
