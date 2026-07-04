#ifndef BME280_H
#define BME280_H

#include "bme280_defs.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== ENUMS ========== */
typedef enum {
  BME280_FILTER_COEFF_OFF = 0b000,
  BME280_FILTER_COEFF_2   = 0b001,
  BME280_FILTER_COEFF_4   = 0b010,
  BME280_FILTER_COEFF_8   = 0b011,
  BME280_FILTER_COEFF_16  = 0b100,
} bme280_filter_t;

typedef enum {
  BME280_STANDBY_0_5_MS   = 0b000,
  BME280_STANDBY_62_5_MS  = 0b001,
  BME280_STANDBY_125_MS   = 0b010,
  BME280_STANDBY_250_MS   = 0b011,
  BME280_STANDBY_500_MS   = 0b100,
  BME280_STANDBY_1000_MS  = 0b101,
  BME280_STANDBY_10_MS    = 0b110,
  BME280_STANDBY_20_MS    = 0b111,
} bme280_standby_t;

typedef enum {
  BME280_MODE_SLEEP_TO_NORMAL = 0b11,
  BME280_MODE_SLEEP_TO_FORCED = 0b01,
  BME280_MODE_FORCED_TO_SLEEP = 0b01,
  BME280_MODE_NORMAL_TO_SLEEP = 0b00,
} bme280_mode_transition_cmds_t;

typedef enum {
  BME280_MODE_SLEEP  = 0b00,
  BME280_MODE_FORCED = 0b01,
  BME280_MODE_NORMAL = 0b11,
} bme280_mode_t;

typedef enum {
  BME280_TEMPERATURE_OS_SKIP = 0b000,
  BME280_TEMPERATURE_OS_1    = 0b001,
  BME280_TEMPERATURE_OS_2    = 0b010,
  BME280_TEMPERATURE_OS_4    = 0b011,
  BME280_TEMPERATURE_OS_8    = 0b100,
  BME280_TEMPERATURE_OS_16   = 0b101,
} bme280_osrs_t_t;

typedef enum {
  BME280_PRESSURE_OS_SKIP = 0b000,
  BME280_PRESSURE_OS_1    = 0b001,
  BME280_PRESSURE_OS_2    = 0b010,
  BME280_PRESSURE_OS_4    = 0b011,
  BME280_PRESSURE_OS_8    = 0b100,
  BME280_PRESSURE_OS_16   = 0b101,
} bme280_osrs_p_t;

typedef enum {
  BME280_HUMIDITY_OS_SKIP = 0b000,
  BME280_HUMIDITY_OS_1    = 0b001,
  BME280_HUMIDITY_OS_2    = 0b010,
  BME280_HUMIDITY_OS_4    = 0b011,
  BME280_HUMIDITY_OS_8    = 0b100,
  BME280_HUMIDITY_OS_16   = 0b101,
} bme280_osrs_h_t;


/* ========== STRUCTS ========== */

/**
 * @brief Contains config data from memory map for bme280
 *
 * Detailed description explaining when to use this configuration
 * and any constraints on its lifecycle.
 */
typedef struct bme280_cfg {
  /* ===== MEMORY MAP CONFIG ===== */

  /* Mode Config
   *
   * 0b00:        Sleep Mode  - No measurements performed and
   *                            power consumption at minimum.
   * 0b01 & 0b10: Forced Mode - Single measurement is performed,
   *                            cached, and then goes to sleep.
   * 0b11:        Normal Mode - Automatic perpetual cycling between
   *                            an active measurement period and
   *                            an inactive measurement period.
   */
  uint8_t mode; /* Power Settings for mode */

  /* Oversampling Config
   * 0b000:  Skipped (output set to 0x80000)
   * 0b001:  oversampling x 1
   * 0b010:  oversampling x 2
   * 0b011:  oversampling x 4
   * 0b100:  oversampling x 8
   * 0b101+: oversampling x 16
   * 
   * Note: Increasing oversampling increases latency
   */
  uint8_t os_hum_cfg;   /* Controls oversampling for humidity */
  uint8_t os_pres_cfg;  /* Controls oversampling for pressure */   
  uint8_t os_temp_cfg;  /* Controls oversampling for temperature */

  /* t_sb Config
   * 0b000: 0.5 ms
   * 0b001: 62.5 ms
   * 0b010: 125 ms
   * 0b011: 250 ms
   * 0b100: 500 ms
   * 0b101: 1000 ms
   * 0b110: 10 ms
   * 0b111: 20 ms
   */
  uint8_t t_sb;  /* Controls inactive duration for t_standby*/

  /* Filter Config
   *
   * 0b000: Filter off
   * 0b001: Filter Coefficient = 2
   * 0b010: Filter Coefficient = 4
   * 0b011: Filter Coefficient = 8
   * 0b100: Filter Coefficient = 16
   */
  uint8_t filter;   /* Controls time constant of the IIR filter */

  bool spi_enabled; /* Enables 3-wire SPI interface when set to ‘1’ */
  
  /* ===== USER CONFIG ===== */
  
  I2C_HandleTypeDef *hi2c;  /* Handle for STM32 HAL H2C */
  uint16_t i2c_address;     /* Select based SDO_LOW or SDO_HIGH */
} bme280_cfg;

/**
 * @brief Contains data for pressure, temperature, and humidity
 * 
 * Addresses are based on BME280 memory map (pg 27)
 */
typedef struct bme280_data {    
  int32_t pressure;       /* addr: 0xF7, bit-format: 20 bits */
  int32_t temperature;    /* addr: 0xFA, bit-format: 20 bits */
  int32_t humidity;       /* addr: 0xFD, bit-format: 16 bits */
} bme280_data;

int bme280_init(bme280_cfg *cfg, I2C_HandleTypeDef *hi2c);
int bme280_read_cfg(bme280_cfg *cfg);
int bme280_read_data(bme280_data *data, bme280_cfg *cfg);
int bme280_set_mode(bme280_cfg *cfg, uint8_t target_mode);
int bme280_write_cfg(bme280_cfg *cfg);

#endif // BME280_H