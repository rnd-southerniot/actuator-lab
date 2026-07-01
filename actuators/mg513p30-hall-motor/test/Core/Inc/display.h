#ifndef __DISPLAY_H
#define __DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Snapshot of telemetry to render on the on-board 2.4" LCD (optional). */
typedef struct {
  const char *state;  /* "DISARMED" / "ARMED" / "FAULT" */
  const char *mode;   /* "IDLE" / "JOG" / "VEL" / "POS"  */
  int32_t pos;        /* encoder counts                  */
  int32_t vel_cps;    /* output velocity, counts/s       */
  int32_t cur_ma;     /* INA238 current, mA              */
  int32_t bus_mv;     /* INA238 bus voltage, mV          */
  int32_t duty;       /* applied signed PWM duty         */
  uint8_t fs_ok;      /* 1 = FS high (no fault)          */
  uint8_t ina_ok;     /* 1 = INA238 present/ok           */
} display_data_t;

/* Bring up the LTDC/ILI9341 LCD over SDRAM. Non-fatal: if the panel fails to init,
 * display_ok() returns 0 and display_update() is a no-op (the LCD is optional). */
void display_init(void);
int display_ok(void);
void display_update(const display_data_t *d);

#ifdef __cplusplus
}
#endif

#endif /* __DISPLAY_H */
