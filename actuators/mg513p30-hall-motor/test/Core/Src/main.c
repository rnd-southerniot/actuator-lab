/*
 * MG513P30 12V (Hall encoder) — bench firmware. Ported from the REV Core Hex build (same rig:
 * F429I-DISC1 + Waveshare 2×MC33886 + INA238). Deltas: ENC_COUNTS_PER_OUT_REV 288→1560
 * (13 PPR × 4 × 30 gear), VEL_MAX_CPS 700→11000 (free speed ≈10200 cps). Gains re-tune on bench.
 *
 * Adds to Step 2: INA238 current/voltage sensing (I2C3 @ 0x40), a DISARMED/ARMED/FAULT
 * safety state machine, an MC33886 FS fault-trip (EXTI on PB7 + a 1 kHz backstop poll),
 * a gated serial console (arm/disarm/jog/vel/pos/gain/stop/reset), and a 1 kHz TIM6 PID
 * loop (velocity + position).
 *
 * SAFETY MODEL (motion is gated, defence-in-depth):
 *   - Boots **DISARMED** with PWM = 0. The motor cannot move until an explicit `arm`.
 *   - The control ISR forces output 0 and holds integrators reset whenever state != ARMED.
 *   - Default PID gains are 0 → even ARMED with a setpoint produces no motion until gains
 *     are set on the bench. `jog` is open-loop but bounded (<=JOG_MAX_MS, auto-stop).
 *   - FS LOW (MC33886 fault) trips to FAULT instantly via EXTI and is re-checked every
 *     1 ms in the ISR; FAULT forces PWM 0 and latches until `reset` (with FS clear).
 *   - Error_Handler re-forces PB4 to a GPIO LOW output (overrides TIM3 AF) → PWMA dead.
 *
 *   Bench motion still requires written go-ahead per ../../SAFETY-CHECKLIST.md. This file
 *   only makes motion *possible under command*; it never self-commands motion.
 *
 * NVIC: encoder EXTI4/EXTI9_5 and the FS fault share priority 2 (PB7 is EXTI line 7, in
 * the EXTI9_5 group with PC5, so FS cannot take an independent vector — it is checked
 * FIRST inside the shared handler, and the 1 kHz poll is the backstop). PID(TIM6)=6,
 * console UART=8, SysTick=15. So encoder+fault preempt PID; PID preempts the console.
 *
 * Pin map + citations: Core/Inc/main.h, ../../WIRING.md, ../../SPECS.md.
 */

#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"

static const char *const kStateName[3] = {"DISARMED", "ARMED", "FAULT"};
static const char *const kModeName[4] = {"IDLE", "JOG", "VEL", "POS"};

static TIM_HandleTypeDef htim2;  /* free-running 1 µs timebase for edge timestamps */
static TIM_HandleTypeDef htim3;  /* TIM3_CH1 PWM on PB4 */
static TIM_HandleTypeDef htim6;  /* 1 kHz control ISR */
static UART_HandleTypeDef huart1;
static I2C_HandleTypeDef hi2c3;

/* ===== Encoder state (written in EXTI ISR, read elsewhere) ===== */
static volatile int32_t g_enc_count;
static volatile uint32_t g_last_edge_us;
static volatile uint32_t g_last_interval_us;
static volatile int8_t g_last_dir;
static volatile uint8_t g_enc_prev_state; /* (A<<1)|B */

static const int8_t kQuadLUT[16] = {
    0, -1, +1, 0,
    +1, 0, 0, -1,
    -1, 0, 0, +1,
    0, +1, -1, 0};

/* ===== Safety / control state ===== */
typedef enum { ST_DISARMED = 0, ST_ARMED, ST_FAULT } drive_state_t;
typedef enum { MODE_IDLE = 0, MODE_JOG, MODE_VEL, MODE_POS } ctrl_mode_t;

static volatile drive_state_t g_state = ST_DISARMED;
static volatile ctrl_mode_t g_mode = MODE_IDLE;
static volatile int32_t g_last_duty;        /* signed applied duty, for telemetry */

/* jog (open-loop, bounded) */
static volatile int32_t g_jog_duty;
static volatile uint32_t g_jog_until_ms;
/* setpoints */
static volatile int32_t g_vel_sp_cps;
static volatile int32_t g_pos_sp_counts;
/* PID gains (default 0 → no motion until tuned on the bench) */
static volatile float g_kvp, g_kvi;          /* velocity PI */
static volatile float g_kpp, g_kpi, g_kpd;   /* position PID */
/* integrator / derivative memory (ISR-only) */
static float g_vel_i, g_pos_i;
static int32_t g_pos_prev;
static float g_vel_filt;                     /* EMA-filtered velocity (ISR), used by the loop */
static volatile int32_t g_vel_filt_cps;      /* snapshot for telemetry */

/* ===== INA238 sampled values (updated in main loop, read in telemetry) ===== */
static volatile int32_t g_ina_current_ma;
static volatile int32_t g_ina_bus_mv;
static volatile uint8_t g_ina_ok;

/* ===== Console RX ===== */
static volatile uint8_t g_rx_byte;
static char g_rx_line[64];
static volatile uint8_t g_rx_idx;
static char g_cmd[64];
static volatile uint8_t g_cmd_ready;

static void SystemClock_Config(void);
static void Timebase_Init(void);
static void MotorDrive_Init(void);
static void Fault_Init(void);
static void Encoder_Init(void);
static void Telem_Init(void);
static void INA238_Init(void);
static void PidTimer_Init(void);
static void StatusLeds_Init(void);
static void apply_output(int32_t duty);
static void fault_trip(void);

static inline uint32_t micros(void) { return __HAL_TIM_GET_COUNTER(&htim2); }

static uint8_t enc_read_state(void) {
  /* Both phases on GPIOC — one IDR read gives a coherent (A,B) pair. */
  uint32_t idr = ENC_A_PORT->IDR;
  uint8_t a = (idr & ENC_A_PIN) ? 1U : 0U;
  uint8_t b = (idr & ENC_B_PIN) ? 1U : 0U;
  return (uint8_t)((a << 1) | b);
}

static int32_t enc_velocity_cps(void) {
  uint32_t now = micros();
  /* Save/restore PRIMASK (not unconditional __enable_irq): this is also called from the
   * TIM6 ISR, so it must not clobber a caller's interrupt-masking state. */
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  uint32_t last = g_last_edge_us;
  uint32_t interval = g_last_interval_us;
  int8_t dir = g_last_dir;
  __set_PRIMASK(primask);

  if (interval == 0U) {
    return 0;
  }
  if ((now - last) > ENC_VEL_STALE_US) {
    return 0;
  }
  /* Sanity-clamp: a very short interval (contact bounce / two edges in ~1 µs) yields a huge
   * spurious cps — reject it so it can't kick the filter/loop. */
  int32_t cps = (int32_t)dir * (int32_t)(TIM2_TICK_HZ / interval);
  if (cps > VEL_MAX_CPS) {
    cps = VEL_MAX_CPS;
  } else if (cps < -VEL_MAX_CPS) {
    cps = -VEL_MAX_CPS;
  }
  return cps;
}

/* ===== INA238 driver (blocking I2C — called only from main context, never the ISR) ===== */
static HAL_StatusTypeDef ina238_write(uint8_t reg, uint16_t val) {
  uint8_t b[2] = {(uint8_t)(val >> 8), (uint8_t)(val & 0xFFU)};
  return HAL_I2C_Mem_Write(&hi2c3, INA238_I2C_ADDR8, reg, I2C_MEMADD_SIZE_8BIT, b, 2, 10);
}

static HAL_StatusTypeDef ina238_read(uint8_t reg, uint16_t *val) {
  uint8_t b[2];
  HAL_StatusTypeDef st = HAL_I2C_Mem_Read(&hi2c3, INA238_I2C_ADDR8, reg, I2C_MEMADD_SIZE_8BIT, b, 2, 10);
  if (st == HAL_OK) {
    *val = (uint16_t)((b[0] << 8) | b[1]);
  }
  return st;
}

static void INA238_Init(void) {
  GPIO_InitTypeDef gpio = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_I2C3_CLK_ENABLE();

  /* PA8 = I2C3_SCL, PC9 = I2C3_SDA (AF4, open-drain). External pull-ups on the board. */
  gpio.Mode = GPIO_MODE_AF_OD;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF4_I2C3;
  gpio.Pin = INA238_I2C_SCL_PIN;
  HAL_GPIO_Init(INA238_I2C_SCL_PORT, &gpio);
  gpio.Pin = INA238_I2C_SDA_PIN;
  HAL_GPIO_Init(INA238_I2C_SDA_PORT, &gpio);

  hi2c3.Instance = INA238_I2C_INSTANCE;
  hi2c3.Init.ClockSpeed = 400000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK) {
    Error_Handler();
  }

  /* Presence check, then configure: ±163.84 mV range (ADCRANGE=0, ≤10 A) + shunt cal for the
   * Adafruit 15 mΩ module. A missing/mis-wired INA238 is non-fatal (telemetry ina_ok=0); must
   * NOT block boot-to-idle. */
  uint16_t id = 0;
  g_ina_ok = 0;
  if (ina238_read(INA238_REG_MANUF_ID, &id) == HAL_OK && id == INA238_MANUF_ID) {
    if (ina238_write(INA238_REG_CONFIG, INA238_CONFIG_VALUE) == HAL_OK &&
        ina238_write(INA238_REG_SHUNT_CAL, INA238_SHUNT_CAL_VALUE) == HAL_OK) {
      g_ina_ok = 1;
    }
  }
}

static void ina238_sample(void) {
  uint16_t cur = 0, bus = 0;
  if (!g_ina_ok) {
    return;
  }
  /* Stop polling on the first failure so a chip that falls off the bus can't repeatedly
   * stall the main loop (and thus command processing) on the I2C timeout. */
  if (ina238_read(INA238_REG_CURRENT, &cur) != HAL_OK) {
    g_ina_ok = 0;
    return;
  }
  g_ina_current_ma = (int32_t)((int16_t)cur) / 4;          /* 0.25 mA/LSB */
  if (ina238_read(INA238_REG_VBUS, &bus) != HAL_OK) {
    g_ina_ok = 0;
    return;
  }
  g_ina_bus_mv = ((int32_t)bus * 25) / 8;                  /* 3.125 mV/LSB */
}

/* ===== Motor output (single gated choke-point) ===== */
static void apply_output(int32_t duty) {
  if (duty > PID_OUT_MAX) {
    duty = PID_OUT_MAX;
  } else if (duty < -PID_OUT_MAX) {
    duty = -PID_OUT_MAX;
  }

  /* Mask the FS-fault / encoder EXTI (priority 2) so fault_trip() cannot preempt between
   * the arm-gate re-check and the compare write — otherwise a stale non-zero duty computed
   * before the fault could land in CCR1 *after* the fault zeroed it. With prio 2 masked,
   * fault_trip() runs fully before (gate sees ST_FAULT → 0) or fully after (it writes 0 last). */
  uint32_t basepri = __get_BASEPRI();
  __set_BASEPRI(2U << (8U - __NVIC_PRIO_BITS));

  if (g_state != ST_ARMED) {
    duty = 0;
  }
  /* Set direction BEFORE the compare (sign-magnitude; PWMA=0 coasts regardless). */
  if (duty > 0) {
    HAL_GPIO_WritePin(MOTOR_DIR_M1_PORT, MOTOR_DIR_M1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_DIR_M2_PORT, MOTOR_DIR_M2_PIN, GPIO_PIN_RESET);
  } else if (duty < 0) {
    HAL_GPIO_WritePin(MOTOR_DIR_M1_PORT, MOTOR_DIR_M1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_DIR_M2_PORT, MOTOR_DIR_M2_PIN, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(MOTOR_DIR_M1_PORT, MOTOR_DIR_M1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_DIR_M2_PORT, MOTOR_DIR_M2_PIN, GPIO_PIN_RESET);
  }
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (uint32_t)((duty < 0) ? -duty : duty));
  g_last_duty = duty;

  __set_BASEPRI(basepri);
}

/* Latch FAULT and kill output. Safe from ISR or main context. */
static void fault_trip(void) {
  g_state = ST_FAULT;
  g_mode = MODE_IDLE;
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
  HAL_GPIO_WritePin(MOTOR_DIR_M1_PORT, MOTOR_DIR_M1_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MOTOR_DIR_M2_PORT, MOTOR_DIR_M2_PIN, GPIO_PIN_RESET);
  g_last_duty = 0;
}

static inline int fs_is_fault(void) {
  /* MC33886 FS: LOW = fault. */
  return (HAL_GPIO_ReadPin(MOTOR_FS_PORT, MOTOR_FS_PIN) == GPIO_PIN_RESET);
}

static float clampf(float v, float lo, float hi) {
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

/* ===== 1 kHz control ISR (TIM6) ===== */
static void control_isr(void) {
  /* Debounced FS-fault poll (the sole fault latch now — the raw EXTI edge is ignored, see
   * HAL_GPIO_EXTI_Callback). Require FS LOW on N consecutive 1 kHz ticks so sub-ms EMI glitches
   * on the unfiltered PB7 line don't false-trip; a real fault holds LOW → latched in N ms. */
  static uint8_t fs_low_ticks = 0U;
  if (fs_is_fault()) {
    if (++fs_low_ticks >= FS_DEBOUNCE_TICKS) {
      fault_trip();
    }
  } else {
    fs_low_ticks = 0U;
  }

  /* Low-pass the (sanity-clamped) velocity every tick — keeps the estimate usable despite
   * the coarse low-CPR T-method, stays warm while disarmed, and feeds telemetry. */
  g_vel_filt += VEL_LPF_ALPHA * ((float)enc_velocity_cps() - g_vel_filt);
  g_vel_filt_cps = (int32_t)g_vel_filt;

  if (g_state != ST_ARMED) {
    g_vel_i = 0.0f;
    g_pos_i = 0.0f;
    g_pos_prev = g_enc_count;
    apply_output(0);
    return;
  }

  /* Reset integrator/derivative memory on any mode change so a stale integral from the
   * previous mode (counts vs counts/s) can't kick the output on switch-over. */
  static ctrl_mode_t prev_mode = MODE_IDLE;
  if (g_mode != prev_mode) {
    g_vel_i = 0.0f;
    g_pos_i = 0.0f;
    g_pos_prev = g_enc_count;
    prev_mode = g_mode;
  }

  const float out_max = (float)PID_OUT_MAX;
  const float dt = 1.0f / (float)CTRL_ISR_HZ; /* 1 ms — proper integral/derivative units */

  switch (g_mode) {
    case MODE_JOG:
      if (HAL_GetTick() < g_jog_until_ms) {
        apply_output(g_jog_duty);
      } else {
        g_mode = MODE_IDLE;
        apply_output(0);
      }
      break;

    case MODE_VEL: {
      float err = (float)g_vel_sp_cps - g_vel_filt; /* filtered feedback */
      g_vel_i += err * dt;
      float i_term = g_kvi * g_vel_i;
      /* anti-windup: clamp the integral TERM to the output range, back-calc the state */
      if (i_term > out_max) {
        i_term = out_max;
        if (g_kvi > 1e-6f) g_vel_i = i_term / g_kvi;
      } else if (i_term < -out_max) {
        i_term = -out_max;
        if (g_kvi > 1e-6f) g_vel_i = i_term / g_kvi;
      }
      float u = g_kvp * err + i_term;
      apply_output((int32_t)clampf(u, -out_max, out_max));
      break;
    }

    case MODE_POS: {
      /* CASCADE: outer position-P sets a clamped velocity setpoint; the inner velocity PI
       * (same as MODE_VEL) turns it into duty. The inner loop absorbs stiction/breakaway, so the
       * approach decelerates smoothly instead of the direct-duty limit-cycle. Only g_kpp is used
       * (cps per count); g_kpi/g_kpd are legacy/unused in the cascade. */
      int32_t pos = g_enc_count;
      float pos_err = (float)g_pos_sp_counts - (float)pos;
      float vel_sp;
      if (pos_err > (float)POS_DEADBAND_COUNTS || pos_err < -(float)POS_DEADBAND_COUNTS) {
        vel_sp = clampf(g_kpp * pos_err, -(float)POS_VEL_CAP_CPS, (float)POS_VEL_CAP_CPS);
      } else {
        vel_sp = 0.0f;      /* within deadband → command a stop (no hunt at target) */
        g_vel_i = 0.0f;     /* park the inner integrator so it doesn't wind up while holding */
      }
      /* inner velocity PI (mirrors MODE_VEL) */
      float err = vel_sp - g_vel_filt;
      g_vel_i += err * dt;
      float i_term = g_kvi * g_vel_i;
      if (i_term > out_max) {
        i_term = out_max;
        if (g_kvi > 1e-6f) g_vel_i = i_term / g_kvi;
      } else if (i_term < -out_max) {
        i_term = -out_max;
        if (g_kvi > 1e-6f) g_vel_i = i_term / g_kvi;
      }
      float u = g_kvp * err + i_term;
      apply_output((int32_t)clampf(u, -out_max, out_max));
      break;
    }

    case MODE_IDLE:
    default:
      apply_output(0);
      break;
  }

  /* Locked-rotor / stall detector (reaches here only when ARMED). Sustained near-max commanded
   * duty with ~zero velocity ⇒ the rotor is stuck (or the closed loop has wound to the rail
   * against a load). Latches a fault so it can't sit drawing stall current. Encoder+duty only —
   * the INA238 is unusable under PWM. Threshold/latch are above the from-rest breakaway so normal
   * starts (velocity rises well within STALL_LATCH_TICKS) do not false-trip. */
  static uint16_t hi_duty_ticks = 0U;
  static int32_t stall_ref_pos = 0;
  int32_t abs_duty = (g_last_duty < 0) ? -g_last_duty : g_last_duty;
  if (abs_duty >= STALL_DUTY_MIN) {
    if (hi_duty_ticks == 0U) {
      stall_ref_pos = g_enc_count;             /* open a fresh window */
    }
    if (++hi_duty_ticks >= STALL_LATCH_TICKS) {
      int32_t moved = g_enc_count - stall_ref_pos;
      if (moved < 0) moved = -moved;
      hi_duty_ticks = 0U;                       /* window closed → restart either way */
      if (moved < STALL_MOVE_COUNTS) {
        fault_trip();                           /* high duty but barely moved ⇒ locked rotor */
      }
    }
  } else {
    hi_duty_ticks = 0U;
  }
}

/* ===== Console ===== */
static void uart_print(const char *s) {
  HAL_UART_Transmit(&huart1, (const uint8_t *)s, (uint16_t)strlen(s), 20U);
}

static void print_status(void) {
  char line[160];
  __disable_irq();
  int32_t pos = g_enc_count;
  __enable_irq();
  int n = snprintf(line, sizeof(line),
                   "STATUS,state=%s,mode=%s,pos=%ld,vel_cps=%ld,vfilt=%ld,duty=%ld,"
                   "cur_ma=%ld,bus_mv=%ld,ina_ok=%u,fs=%d\r\n",
                   kStateName[g_state], kModeName[g_mode], (long)pos, (long)enc_velocity_cps(),
                   (long)g_vel_filt_cps, (long)g_last_duty, (long)g_ina_current_ma, (long)g_ina_bus_mv,
                   (unsigned)g_ina_ok, fs_is_fault() ? 0 : 1);
  if (n > 0) {
    HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)n, 20U);
  }
}

static int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi) {
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

static void console_parse(char *cmd) {
  if (strcmp(cmd, "status") == 0) {
    print_status();
  } else if (strcmp(cmd, "arm") == 0) {
    if (g_state == ST_DISARMED && !fs_is_fault()) {
      g_state = ST_ARMED;
      g_mode = MODE_IDLE;
      uart_print("OK armed\r\n");
    } else {
      uart_print("ERR cannot arm (faulted or not disarmed)\r\n");
    }
  } else if (strcmp(cmd, "disarm") == 0) {
    g_state = ST_DISARMED;
    g_mode = MODE_IDLE;
    uart_print("OK disarmed\r\n");
  } else if (strcmp(cmd, "stop") == 0) {
    g_mode = MODE_IDLE;
    uart_print("OK stop\r\n");
  } else if (strcmp(cmd, "reset") == 0) {
    if (g_state == ST_FAULT && !fs_is_fault()) {
      g_state = ST_DISARMED;
      g_mode = MODE_IDLE;
      uart_print("OK reset\r\n");
    } else {
      uart_print("ERR cannot reset (FS still low or not faulted)\r\n");
    }
  } else if (strncmp(cmd, "jog ", 4) == 0) {
    int d = 0, ms = 0;
    if (sscanf(cmd + 4, "%d %d", &d, &ms) == 2 && g_state == ST_ARMED) {
      g_jog_duty = clamp_i32(d, -PID_OUT_MAX, PID_OUT_MAX);
      g_jog_until_ms = HAL_GetTick() + (uint32_t)clamp_i32(ms, 0, (int32_t)JOG_MAX_MS);
      g_mode = MODE_JOG;
      uart_print("OK jog\r\n");
    } else {
      uart_print("ERR jog <duty> <ms> (must be armed)\r\n");
    }
  } else if (strncmp(cmd, "vel ", 4) == 0) {
    int c = 0;
    if (sscanf(cmd + 4, "%d", &c) == 1 && g_state == ST_ARMED) {
      g_vel_sp_cps = c;
      g_mode = MODE_VEL;
      uart_print("OK vel\r\n");
    } else {
      uart_print("ERR vel <cps> (must be armed)\r\n");
    }
  } else if (strncmp(cmd, "pos ", 4) == 0) {
    int c = 0;
    if (sscanf(cmd + 4, "%d", &c) == 1 && g_state == ST_ARMED) {
      g_pos_sp_counts = c;
      g_mode = MODE_POS;
      uart_print("OK pos\r\n");
    } else {
      uart_print("ERR pos <counts> (must be armed)\r\n");
    }
  } else if (strncmp(cmd, "gainv ", 6) == 0) {
    float kp = 0, ki = 0;
    if (sscanf(cmd + 6, "%f %f", &kp, &ki) == 2) {
      g_kvp = kp;
      g_kvi = ki;
      uart_print("OK gainv\r\n");
    } else {
      uart_print("ERR gainv <kp> <ki>\r\n");
    }
  } else if (strncmp(cmd, "gainp ", 6) == 0) {
    float kp = 0, ki = 0, kd = 0;
    if (sscanf(cmd + 6, "%f %f %f", &kp, &ki, &kd) == 3) {
      g_kpp = kp;
      g_kpi = ki;
      g_kpd = kd;
      uart_print("OK gainp\r\n");
    } else {
      uart_print("ERR gainp <kp> <ki> <kd>\r\n");
    }
  } else {
    uart_print("ERR ? (status arm disarm stop reset jog vel pos gainv gainp)\r\n");
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();

  StatusLeds_Init();
  Timebase_Init();
  MotorDrive_Init(); /* PWM 0% = coast, before anything else can run */
  Fault_Init();
  Encoder_Init();
  Telem_Init();
  INA238_Init();
  PidTimer_Init(); /* control ISR starts; state DISARMED → it only ever applies 0 */
  display_init();  /* optional LCD; non-fatal if absent */

  HAL_UART_Receive_IT(&huart1, (uint8_t *)&g_rx_byte, 1);
  uart_print("MG513P30 bench fw. DISARMED. type 'status'.\r\n");

  uint32_t last_telem = HAL_GetTick();
  for (;;) {
    if (g_cmd_ready) {
      console_parse(g_cmd);
      g_cmd_ready = 0;
    }

    uint32_t now_ms = HAL_GetTick();
    if ((now_ms - last_telem) >= TELEM_PERIOD_MS) {
      last_telem = now_ms;
      HAL_GPIO_TogglePin(LED_HEARTBEAT_PORT, LED_HEARTBEAT_PIN);
      if (g_state == ST_FAULT) {
        HAL_GPIO_WritePin(LED_FAULT_PORT, LED_FAULT_PIN, GPIO_PIN_SET);
      } else {
        HAL_GPIO_WritePin(LED_FAULT_PORT, LED_FAULT_PIN, GPIO_PIN_RESET);
      }
      ina238_sample(); /* blocking I2C — main context only */
      print_status();

      if (display_ok()) {
        __disable_irq();
        int32_t pos = g_enc_count;
        __enable_irq();
        display_data_t dd = {
            .state = kStateName[g_state],
            .mode = kModeName[g_mode],
            .pos = pos,
            .vel_cps = enc_velocity_cps(),
            .cur_ma = g_ina_current_ma,
            .bus_mv = g_ina_bus_mv,
            .duty = g_last_duty,
            .fs_ok = (uint8_t)(fs_is_fault() ? 0U : 1U),
            .ina_ok = g_ina_ok,
        };
        display_update(&dd);
      }
    }
  }
}

/* ===== Init ===== */
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

static void MotorDrive_Init(void) {
  GPIO_InitTypeDef gpio = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(MOTOR_DIR_M1_PORT, MOTOR_DIR_M1_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MOTOR_DIR_M2_PORT, MOTOR_DIR_M2_PIN, GPIO_PIN_RESET);
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Pin = MOTOR_DIR_M1_PIN;
  HAL_GPIO_Init(MOTOR_DIR_M1_PORT, &gpio);
  gpio.Pin = MOTOR_DIR_M2_PIN;
  HAL_GPIO_Init(MOTOR_DIR_M2_PORT, &gpio);

  /* PB4 → TIM3_CH1 (AF2), PWM started at 0% = coast. */
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF2_TIM3;
  gpio.Pin = MOTOR_PWM_PIN;
  HAL_GPIO_Init(MOTOR_PWM_PORT, &gpio);

  __HAL_RCC_TIM3_CLK_ENABLE();
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = PWM_PSC; /* 1 MHz timer clock → 1 kHz PWM (MC33886 enable limit) */
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = PWM_ARR;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
    Error_Handler();
  }
  TIM_OC_InitTypeDef oc = {0};
  oc.OCMode = TIM_OCMODE_PWM1;
  oc.Pulse = 0;
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &oc, TIM_CHANNEL_1) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK) {
    Error_Handler();
  }
}

/* PB7 = MC33886 FS fault → EXTI falling edge (HIGH idle → LOW = fault). Shares EXTI9_5
 * with PC5 (encoder B); NVIC for EXTI9_5 is enabled in Encoder_Init. No internal pull
 * (board's 1k→5V owns the line). */
static void Fault_Init(void) {
  GPIO_InitTypeDef gpio = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();
  gpio.Mode = GPIO_MODE_IT_FALLING;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Pin = MOTOR_FS_PIN;
  HAL_GPIO_Init(MOTOR_FS_PORT, &gpio);
}

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

  /* Encoder + FS fault are high priority (preempt the PID). FS shares EXTI9_5. */
  HAL_NVIC_SetPriority(EXTI4_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
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
  HAL_NVIC_SetPriority(USART1_IRQn, 8, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}

static void PidTimer_Init(void) {
  __HAL_RCC_TIM6_CLK_ENABLE();
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = (uint32_t)((TIM_APB1_HZ / 1000000UL) - 1U); /* 90 → 1 MHz */
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = (uint32_t)((1000000UL / CTRL_ISR_HZ) - 1U);    /* 999 → 1 kHz */
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
    Error_Handler();
  }
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
  if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK) {
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

/* ===== Interrupt path ===== */
void EXTI4_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(ENC_A_PIN); }   /* PC4 = Ch A */
void EXTI9_5_IRQHandler(void) {
  /* Check the FS fault (PB7, line 7) FIRST, then encoder B (PC5, line 5). */
  HAL_GPIO_EXTI_IRQHandler(MOTOR_FS_PIN);
  HAL_GPIO_EXTI_IRQHandler(ENC_B_PIN);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == MOTOR_FS_PIN) {
    /* Do NOT latch on the raw FS edge: the unfiltered PB7 line picks up EMI glitches at speed
     * that produce spurious falling edges. Real faults are caught by the debounced 1 kHz poll
     * in control_isr (FS held LOW for FS_DEBOUNCE_TICKS). */
    return;
  }
  /* encoder edge (PC4 or PC5): decode against the previous coherent state */
  uint8_t cur = enc_read_state();
  /* MG513P30: this motor's A/B orientation counts opposite to the drive direction
   * (+duty measured −pos on the bench). Negate the decode so +duty → +pos and +vel
   * (loop-sign correctness). Flips g_enc_count and g_last_dir together. */
  int8_t d = -kQuadLUT[(g_enc_prev_state << 2) | cur];
  if (d != 0) {
    uint32_t now = micros();
    g_enc_count += d;
    g_last_interval_us = now - g_last_edge_us;
    g_last_edge_us = now;
    g_last_dir = d;
  }
  g_enc_prev_state = cur;
}

void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&huart1); }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  char c = (char)g_rx_byte;
  if (c == '\r' || c == '\n') {
    if (g_rx_idx > 0) {
      if (!g_cmd_ready) {
        memcpy(g_cmd, g_rx_line, g_rx_idx);
        g_cmd[g_rx_idx] = '\0';
        g_cmd_ready = 1;
      }
      g_rx_idx = 0;
    }
  } else if (g_rx_idx < (sizeof(g_rx_line) - 1)) {
    g_rx_line[g_rx_idx++] = c;
  }
  HAL_UART_Receive_IT(huart, (uint8_t *)&g_rx_byte, 1);
}

void TIM6_DAC_IRQHandler(void) { HAL_TIM_IRQHandler(&htim6); }

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM6) {
    control_isr();
  }
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
  /* Guard the 90 MHz APB1 timer-clock assumption behind PWM_ARR / TIM2 / TIM6 derivations. */
  if ((HAL_RCC_GetPCLK1Freq() * 2U) != TIM_APB1_HZ) {
    Error_Handler();
  }
}

/* Fault sink: force PB4 to GPIO LOW (overriding TIM3 AF) so PWMA cannot drive, drop
 * direction LOW, blink red. */
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
