#ifndef BME280_DEFS_H
#define BME280_DEFS_H

#define BME280_MEAS_TIMEOUT_MS		  150

/* 7-bit I2C address, selected by the SDO pin */
#define BME280_I2C_ADDR_SDO_LOW     0x76u   /* SDO = GND   → 0b1110110 */
#define BME280_I2C_ADDR_SDO_HIGH    0x77u   /* SDO = VDDIO → 0b1110111 */

/* Based on BME280 Data Sheet Memory Map (pg 27) */
#define BME280_CFG_ADDR_START		    0xF2
#define BME280_CFG_ADDR_END			    0xF5
#define BME280_CFG_LEN				      4		    /* 4 bytes */

#define BME280_DATA_ADDR_START		  0xF7
#define BME280_DATA_ADDR_END		    0xFE
#define BME280_DATA_LEN_HUM			    8		    /* 8 bytes: with humidity included */
#define BME280_DATA_LEN_TEMP		    6		    /* 6 bytes: with humidity not included */

/* ========== MEMORY MAP MACROS ========== */
/* ===== HUM_LSB ===== */
#define BME280_HUM_LSB_ADDR			    0xFE

/* ===== HUM_MSB ===== */
#define BME280_HUM_MSB_ADDR			    0xFD
#define BME280_HUM_MSB_SHIFT		    8u

/* ===== TEMP_XLSB ===== */
#define BME280_TEMP_XLSB_ADDR		    0xFC
#define BME280_TEMP_XLSB_Pos		    4u
#define BME280_TEMP_XLSB_Len		    0x0Fu
#define BME280_TEMP_XLSB_Msk		    (BME280_TEMP_XLSB_Len << BME280_TEMP_XLSB_Pos)

/* ===== TEMP_LSB ===== */
#define BME280_TEMP_LSB_ADDR		    0xFB
#define BME280_TEMP_LSB_SHIFT		    4u

/* ===== TEMP_MSB ===== */
#define BME280_TEMP_MSB_ADDR		    0xFA
#define BME280_TEMP_MSB_SHIFT		    12u

/* ===== PRESS_XLSB ===== */
#define BME280_PRESS_XLSB_ADDR		  0xF9
#define BME280_PRESS_XLSB_Pos		    4u
#define BME280_PRESS_XLSB_Len		    0x0Fu
#define BME280_PRESS_XLSB_Msk		    (BME280_PRESS_XLSB_Len << BME280_PRESS_XLSB_Pos)

/* ===== PRESS_LSB ===== */
#define BME280_PRESS_LSB_ADDR		    0xF8
#define BME280_PRESS_LSB_SHIFT		  4u

/* ===== PRESS_MSB ===== */
#define BME280_PRESS_MSB_ADDR		    0xF7
#define BME280_PRESS_MSB_SHIFT		  12u

/* ===== CONFIG ===== */
#define BME280_CONFIG_ADDR			    0xF5
#define BME280_CONFIG_T_SB_Pos		  5u
#define BME280_CONFIG_T_SB_Len		  0x07u
#define BME280_CONFIG_T_SB_Msk		  (BME280_CONFIG_T_SB_Len << BME280_CONFIG_T_SB_Pos)

#define BME280_CONFIG_FILTER_Pos	  2u
#define BME280_CONFIG_FILTER_Len	  0x07u
#define BME280_CONFIG_FILTER_Msk	  (BME280_CONFIG_FILTER_Len << BME280_CONFIG_FILTER_Pos)

#define BME280_CONFIG_SPI3W_EN_Pos	0u
#define BME280_CONFIG_SPI3W_EN_Len	0x01u
#define BME280_CONFIG_SPI3W_EN_Msk	(BME280_CONFIG_SPI3W_EN_Len << BME280_CONFIG_SPI3W_EN_Pos)

/* ===== CTRL_MEAS ===== */
#define BME280_CTRL_MEAS_ADDR		    0xF4
#define BME280_CTRL_MEAS_OSRS_T_Pos 5u
#define BME280_CTRL_MEAS_OSRS_T_Len 0x07u
#define BME280_CTRL_MEAS_OSRS_T_Msk (BME280_CTRL_MEAS_OSRS_T_Len << BME280_CTRL_MEAS_OSRS_T_Pos)

#define BME280_CTRL_MEAS_OSRS_P_Pos 2u
#define BME280_CTRL_MEAS_OSRS_P_Len 0x07u
#define BME280_CTRL_MEAS_OSRS_P_Msk (BME280_CTRL_MEAS_OSRS_P_Len << BME280_CTRL_MEAS_OSRS_P_Pos)

#define BME280_CTRL_MEAS_MODE_Pos 	0u
#define BME280_CTRL_MEAS_MODE_Len 	0x03u
#define BME280_CTRL_MEAS_MODE_Msk 	(BME280_CTRL_MEAS_MODE_Len << BME280_CTRL_MEAS_MODE_Pos)

/* ===== STATUS ===== */
#define BME280_STATUS_ADDR			    0xF3
#define BME280_STATUS_MEASURING_Pos 3u
#define BME280_STATUS_MEASURING_Len 0x01u
#define BME280_STATUS_MEASURING_Msk (BME280_STATUS_MEASURING_Len << BME280_STATUS_MEASURING_Pos)

#define BME280_STATUS_IM_UPDATE_Pos	0u
#define BME280_STATUS_IM_UPDATE_Len 0x01u
#define BME280_STATUS_IM_UPDATE_Msk	(BME280_STATUS_IM_UPDATE_Len << BME280_STATUS_IM_UPDATE_Pos)

/* ===== CTRL_HUM ===== */
#define BME280_CTRL_HUM_ADDR		    0xF2
#define BME280_CTRL_HUM_OSRS_H_Pos	0u
#define BME280_CTRL_HUM_OSRS_H_Len	0x07u
#define BME280_CTRL_HUM_OSRS_H_Msk	(BME280_CTRL_HUM_OSRS_H_Len << BME280_CTRL_HUM_OSRS_H_Pos)

/* ===== RESET ===== */
#define BME280_RESET_ADDR			      0xE0	/* write only */
#define BME280_RESET_CODE			      0xB6	/* Reset code using the complete power-on-reset procedure */

/* ===== ID ===== */
#define BME280_CHIP_ID_ADDR				  0xD0	/* Chip ID: 0x60 which can be read as soon 
										   	                     as the device finished the power-on-reset */
#define BME280_CHIP_ID              0x60

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

typedef enum {
  BME280_SPI_ENABLED      = 0,
  BME280_I2C_ENABLED      = 1,
} bme280_spi_or_i2c_t;

typedef enum {
  BME280_STATUS_CODE_OK      =  0,
  BME280_ERROR_NULL_PTR      = -1,
  BME280_ERROR_INVALID_LEN   = -2,
  BME280_ERROR_I2C_READ      = -3,
  BME280_ERROR_I2C_WRITE     = -4,
  BME280_ERROR_CHIP_ID       = -5,
  BME280_ERROR_TIMEOUT       = -6,
  BME280_ERROR_TEMP_REQUIRED = -7,
} bme280_status_t;

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

  bme280_spi_or_i2c_t spi_enabled; /* Enables 3-wire SPI interface when set to ‘1’ */
  
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
  uint32_t pressure;        /* addr: 0xF7, bit-format: 20 bits */
  int32_t temperature;      /* addr: 0xFA, bit-format: 20 bits */
  uint32_t humidity;        /* addr: 0xFD, bit-format: 16 bits */
} bme280_data;

#endif // BME280_DEFS_H