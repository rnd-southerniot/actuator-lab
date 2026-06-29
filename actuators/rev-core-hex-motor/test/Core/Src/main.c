/*
 * REV Core Hex Motor — bench firmware, Step 2.
 *
 * Adds to the Step 1 skeleton (still NO motion — PWM duty stays 0):
 *   - TIM3_CH1 PWM on PB4 (~20 kHz), started at 0% duty = coast.
 *   - Direction GPIOs PA5/PA7 (held LOW).
 *   - MC33886 FS fault input on PB7 (read-only here; EXTI trip lands in Step 3).
 *   - Quadrature encoder PC4/PC5 decoded by EXTI on both edges, with a free-running
 *     TIM2 (32-bit, 1 µs) providing edge timestamps for T-method velocity. Counts/sample
 *     is unusable at this CPR (≈600 cps), so velocity comes from the edge interval.
 *   - USART1 telemetry (115200 8N1): a periodic STATUS line.
 *
 * Motion is still gated off: nothing ever writes a non-zero PWM compare. Bench Phase 2
 * uses the telemetry to hand-verify encoder direction/CPR and FS level before any jog.
 *
 * Pin map + citations: Core/Inc/main.h, ../../WIRING.md, ../../SPECS.md.
 */

#include "main.h"

#include <stdio.h>

static TIM_HandleTypeDef htim2; /* free-running 1 µs timebase for edge timestamps */
static TIM_HandleTypeDef htim3; /* TIM3_CH1 PWM on PB4 */
static UART_HandleTypeDef huart1;

/* --- Encoder state (written in EXTI ISR, read in main loop) --- */
static volatile int32_t g_enc_count;
static volatile uint32_t g_last_edge_us;
static volatile uint32_t g_last_interval_us;
static volatile int8_t g_last_dir;
static volatile uint8_t g_enc_prev_state; /* (A<<1)|B */

/* Quadrature transition table, indexed by (prev_state<<2)|curr_state. */
static const int8_t kQuadLUT[16] = {
    0, -1, +1, 0,
    +1, 0, 0, -1,
    -1, 0, 0, +1,
    0, +1, -1, 0};

static void SystemClock_Config(void);
static void Timebase_Init(void);
static void MotorDrive_Init(void);
static void Encoder_Init(void);
static void Telem_Init(void);
static void StatusLeds_Init(void);

static inline uint32_t micros(void) { return __HAL_TIM_GET_COUNTER(&htim2); }

static uint8_t enc_read_state(void) {
  /* Both phases are on GPIOC — read IDR once so A and B come from a single coherent
   * latch (two separate reads could straddle an edge and return an inconsistent pair). */
  uint32_t idr = ENC_A_PORT->IDR;
  uint8_t a = (idr & ENC_A_PIN) ? 1U : 0U;
  uint8_t b = (idr & ENC_B_PIN) ? 1U : 0U;
  return (uint8_t)((a << 1) | b);
}

/* T-method velocity from the most recent edge interval; 0 if the shaft is stalled/stale. */
static int32_t enc_velocity_cps(void) {
  uint32_t now = micros();
  __disable_irq();
  uint32_t last = g_last_edge_us;
  uint32_t interval = g_last_interval_us;
  int8_t dir = g_last_dir;
  __enable_irq();

  if (interval == 0U) {
    return 0;
  }
  if ((now - last) > ENC_VEL_STALE_US) {
    return 0;
  }
  return (int32_t)dir * (int32_t)(TIM2_TICK_HZ / interval);
}

int main(void) {
  HAL_Init();
  SystemClock_Config();

  StatusLeds_Init();
  Timebase_Init();
  MotorDrive_Init(); /* PWM started at 0% = coast, before anything else can run */
  Encoder_Init();
  Telem_Init();

  char line[96];
  uint32_t last_telem = HAL_GetTick();

  for (;;) {
    uint32_t now_ms = HAL_GetTick();
    if ((now_ms - last_telem) >= TELEM_PERIOD_MS) {
      last_telem = now_ms;
      HAL_GPIO_TogglePin(LED_HEARTBEAT_PORT, LED_HEARTBEAT_PIN);

      __disable_irq();
      int32_t pos = g_enc_count;
      __enable_irq();
      int32_t vel = enc_velocity_cps();
      int fs_ok = (HAL_GPIO_ReadPin(MOTOR_FS_PORT, MOTOR_FS_PIN) == GPIO_PIN_SET) ? 1 : 0;
      uint32_t duty = __HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_1);

      int n = snprintf(line, sizeof(line),
                       "STATUS,pos=%ld,vel_cps=%ld,fs=%d,duty=%lu\r\n",
                       (long)pos, (long)vel, fs_ok, (unsigned long)duty);
      if (n > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)n, 10U);
      }
    }
  }
}

/* TIM2: 32-bit free-running at 1 MHz → 1 µs edge timestamps (T/M-method timebase). */
static void Timebase_Init(void) {
  __HAL_RCC_TIM2_CLK_ENABLE();
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = (uint32_t)((TIM_APB1_HZ / TIM2_TICK_HZ) - 1U); /* 90 → 1 MHz */
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xFFFFFFFFUL;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_TIM_Base_Start(&htim2) != HAL_OK) {
    Error_Handler();
  }
}

/*
 * PB4 = TIM3_CH1 PWM (~20 kHz), PA5/PA7 = direction (LOW), PB7 = FS input.
 * Park direction LOW and start PWM at 0% so PWMA holds LOW (coast) from boot.
 */
static void MotorDrive_Init(void) {
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Direction outputs, LOW. */
  HAL_GPIO_WritePin(MOTOR_DIR_M1_PORT, MOTOR_DIR_M1_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MOTOR_DIR_M2_PORT, MOTOR_DIR_M2_PIN, GPIO_PIN_RESET);
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Pin = MOTOR_DIR_M1_PIN;
  HAL_GPIO_Init(MOTOR_DIR_M1_PORT, &gpio);
  gpio.Pin = MOTOR_DIR_M2_PIN;
  HAL_GPIO_Init(MOTOR_DIR_M2_PORT, &gpio);

  /* FS fault input: float (board pulls 1k→5V); PB7 is 5 V-tolerant. LOW = fault. */
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_NOPULL;
  gpio.Pin = MOTOR_FS_PIN;
  HAL_GPIO_Init(MOTOR_FS_PORT, &gpio);

  /* PB4 → TIM3_CH1 (AF2). */
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF2_TIM3;
  gpio.Pin = MOTOR_PWM_PIN;
  HAL_GPIO_Init(MOTOR_PWM_PORT, &gpio);

  __HAL_RCC_TIM3_CLK_ENABLE();
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = PWM_ARR;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
    Error_Handler();
  }

  TIM_OC_InitTypeDef oc = {0};
  oc.OCMode = TIM_OCMODE_PWM1;
  oc.Pulse = 0; /* 0% duty → PWMA LOW → coast. NO motion. */
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &oc, TIM_CHANNEL_1) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK) {
    Error_Handler();
  }
}

/* PC4/PC5 quadrature → EXTI on both edges, decoded against TIM2 timestamps. */
static void Encoder_Init(void) {
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();

  gpio.Mode = GPIO_MODE_IT_RISING_FALLING;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Pin = ENC_A_PIN;
  HAL_GPIO_Init(ENC_A_PORT, &gpio);
  gpio.Pin = ENC_B_PIN;
  HAL_GPIO_Init(ENC_B_PORT, &gpio);

  g_enc_prev_state = enc_read_state();
  g_enc_count = 0;
  g_last_edge_us = micros();
  g_last_interval_us = 0;
  g_last_dir = 0;

  HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

static void Telem_Init(void) {
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();

  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF7_USART1;
  gpio.Pin = TELEM_UART_TX_PIN;
  HAL_GPIO_Init(TELEM_UART_TX_PORT, &gpio);
  gpio.Pin = TELEM_UART_RX_PIN;
  HAL_GPIO_Init(TELEM_UART_RX_PORT, &gpio);

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
  }
}

static void StatusLeds_Init(void) {
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_GPIO_WritePin(LED_HEARTBEAT_PORT, LED_HEARTBEAT_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_FAULT_PORT, LED_FAULT_PIN, GPIO_PIN_RESET);

  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Pin = LED_HEARTBEAT_PIN;
  HAL_GPIO_Init(LED_HEARTBEAT_PORT, &gpio);
  gpio.Pin = LED_FAULT_PIN;
  HAL_GPIO_Init(LED_FAULT_PORT, &gpio);
}

/* --- Encoder interrupt path --- */
void EXTI4_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(ENC_A_PIN); }   /* PC4 = Ch A */
void EXTI9_5_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(ENC_B_PIN); } /* PC5 = Ch B */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  (void)GPIO_Pin; /* either edge re-reads both channels */
  uint8_t cur = enc_read_state();
  int8_t d = kQuadLUT[(g_enc_prev_state << 2) | cur];
  if (d != 0) {
    uint32_t now = micros();
    g_enc_count += d;
    g_last_interval_us = now - g_last_edge_us;
    g_last_edge_us = now;
    g_last_dir = d;
  }
  g_enc_prev_state = cur;
}

void SysTick_Handler(void) { HAL_IncTick(); }

static void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    Error_Handler();
  }
}

/*
 * Fault sink: kill motion and hold it. Force PB4 to a GPIO LOW output (overriding the
 * TIM3 AF) so PWMA cannot drive regardless of timer state, drop direction LOW, blink red.
 */
void Error_Handler(void) {
  __disable_irq();

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {0};
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_WritePin(MOTOR_PWM_PORT, MOTOR_PWM_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MOTOR_DIR_M1_PORT, MOTOR_DIR_M1_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MOTOR_DIR_M2_PORT, MOTOR_DIR_M2_PIN, GPIO_PIN_RESET);
  gpio.Pin = MOTOR_PWM_PIN;
  HAL_GPIO_Init(MOTOR_PWM_PORT, &gpio);
  gpio.Pin = MOTOR_DIR_M1_PIN;
  HAL_GPIO_Init(MOTOR_DIR_M1_PORT, &gpio);
  gpio.Pin = MOTOR_DIR_M2_PIN;
  HAL_GPIO_Init(MOTOR_DIR_M2_PORT, &gpio);

  gpio.Pin = LED_FAULT_PIN;
  HAL_GPIO_Init(LED_FAULT_PORT, &gpio);

  for (;;) {
    HAL_GPIO_TogglePin(LED_FAULT_PORT, LED_FAULT_PIN);
    for (volatile uint32_t d = 0; d < 1000000U; ++d) {
      __NOP();
    }
  }
}
