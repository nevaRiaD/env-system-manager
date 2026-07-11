#include "bme280.h"
#include "bme280_defs.h"
#include "stm32f4xx_hal_i2c.h"

/**
 * @brief Calibration data
 */
typedef struct bme280_calib_data {
  /* Temperature */
  uint16_t dig_T1;  /* addr: 0x88/0x89      , content: [7:0]/[15:8] */
  int16_t  dig_T2;  /* addr: 0x8A/0x8B      , content: [7:0]/[15:8] */
  int16_t  dig_T3;  /* addr: 0x8C/0x8D      , content: [7:0]/[15:8] */

  /* Pressure */
  uint16_t dig_P1;  /* addr: 0x8E/0x8F      , content: [7:0]/[15:8] */
  int16_t  dig_P2;  /* addr: 0x90/0x91      , content: [7:0]/[15:8] */
  int16_t  dig_P3;  /* addr: 0x92/0x93      , content: [7:0]/[15:8] */
  int16_t  dig_P4;  /* addr: 0x94/0x95      , content: [7:0]/[15:8] */
  int16_t  dig_P5;  /* addr: 0x96/0x97      , content: [7:0]/[15:8] */
  int16_t  dig_P6;  /* addr: 0x98/0x99      , content: [7:0]/[15:8] */
  int16_t  dig_P7;  /* addr: 0x9A/0x9B      , content: [7:0]/[15:8] */
  int16_t  dig_P8;  /* addr: 0x9C/0x9D      , content: [7:0]/[15:8] */
  int16_t  dig_P9;  /* addr: 0x9E/0x9F      , content: [7:0]/[15:8] */

  /* Humidity */
  uint8_t  dig_H1;  /* addr: 0xA1           , content: [7:0]        */
  int16_t  dig_H2;  /* addr: 0xE1/0xE2      , content: [7:0]/[15:8] */
  uint8_t  dig_H3;  /* addr: 0xE3           , content: [7:0]        */
  int16_t  dig_H4;  /* addr: 0xE4/0xE5[3:0] , content: [11:4]/[3:0] */
  int16_t  dig_H5;  /* addr: 0xE5[7:4]/0xE6 , content: [3:0]/[11:4] */
  int8_t   dig_H6;  /* addr: 0xE7           , content: [8:0]        */

  int32_t  t_fine;
} bme280_calib_data;

/* static function prototypes */
static bme280_status_t bme280_read_calib_data(bme280_cfg *cfg, bme280_calib_data *c_data);
static bme280_status_t bme280_read_cfg(bme280_cfg *cfg);
static bme280_status_t bme280_write_cfg(bme280_cfg *cfg);
static bme280_status_t bme280_read_id(bme280_cfg *cfg);
static bme280_status_t bme280_wait_idle(bme280_cfg *cfg);

/* Compensation function prototypes */
static int32_t  bme280_compensate_T_int32(const bme280_calib_data *c_data, int32_t *t_fine, int32_t adc_T);
static uint32_t bme280_compensate_P_int32(const bme280_calib_data *c_data, int32_t t_fine, int32_t adc_p);
static uint32_t bme280_compensate_H_int32(const bme280_calib_data *c_data, int32_t t_fine, int32_t adc_H);

/* I2C function prototypes */

static int bme280_i2c_read_reg(uint16_t memAddress, uint8_t *buf,
                               uint16_t len, bme280_cfg *cfg);
static int bme280_i2c_write_reg(uint16_t memAddress, uint8_t *buf,
                                uint16_t len, bme280_cfg *cfg);


/* ========== PUBLIC FUNCTION DEFINITIONS ========== */

bme280_status_t bme280_init(bme280_cfg *cfg, I2C_HandleTypeDef *hi2c)
{
  // Return if I2C handler is null
  if (hi2c == NULL) {
    return BME280_ERROR_NULL_PTR;
  }

  /* Check if cfg is null or has null values */
  if (cfg == NULL) {
    *cfg = (bme280_cfg) {
      .mode        = BME280_MODE_NORMAL,
      .os_hum_cfg  = BME280_HUMIDITY_OS_1,
      .os_pres_cfg = BME280_PRESSURE_OS_1,
      .os_temp_cfg = BME280_TEMPERATURE_OS_1,
      .t_sb        = BME280_STANDBY_500_MS,
      .filter      = BME280_FILTER_COEFF_2,
      .spi_enabled = false,                   /* using I2C */
      .hi2c        = hi2c,
      .i2c_address = BME280_I2C_ADDR_SDO_LOW, /* SDO->GND */
    };
  }

  /* Read chip-id */
  uint8_t chip_id = 0;
  bme280_status_t status = bme280_read_id(cfg);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }

  /* Write cfg to BME280*/
  bme280_status_t status = bme280_write_cfg(cfg);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }

  return BME280_STATUS_CODE_OK;
}

bme280_status_t bme280_read_data(bme280_data *data, bme280_cfg *cfg)
{
  uint8_t raw[BME280_DATA_LEN_HUM];
  uint16_t data_len = (cfg->os_hum_cfg != BME280_HUMIDITY_OS_SKIP) ?
                       BME280_DATA_LEN_HUM : BME280_DATA_LEN_TEMP;
  
  /* adc values read from registers */
  int32_t t_fine;
  int32_t adc_t;
  int32_t adc_p;
  int32_t adc_h;

  /**
   * TODO: REMOVE and use dev struct
   */
  bme280_calib_data c_data;

  if (bme280_i2c_read_reg(BME280_DATA_ADDR_START, raw,
                          data_len, cfg)) {
    return BME280_ERROR_I2C_READ; // I2C read error
  }

  /* Check if temperature is enabled, cannot calculate 
   * humidity and pressure without t_fine. 
  */
  if (cfg->os_temp_cfg == BME280_TEMPERATURE_OS_SKIP) {
    data->temperature = INT32_MIN;
    data->pressure = INT32_MIN;
    data->humidity = INT32_MIN;

    return BME280_ERROR_TEMP_REQUIRED;
  }

  /* raw[3]=temp_msb raw[4]=temp_lsb raw[5]=temp_xlsb<7:4> */
  adc_t = ((uint32_t)raw[3] << BME280_TEMP_MSB_SHIFT) |
          ((uint32_t)raw[4] << BME280_TEMP_LSB_SHIFT) |
          (raw[5] >> BME280_TEMP_XLSB_Pos);

  /* Calculates t_fine and temperature */
  data->temperature = bme280_compensate_T_int32(&c_data, &t_fine, adc_t);
  
  /* Check if pressure is enabled */
  if (cfg->os_pres_cfg != BME280_PRESSURE_OS_SKIP) {
    /* raw[0]=press_msb raw[1]=press_lsb raw[2]=press_xlsb<7:4> */
    adc_p = ((uint32_t)raw[0] << BME280_PRESS_MSB_SHIFT) |
            ((uint32_t)raw[1] << BME280_PRESS_LSB_SHIFT) |
            (raw[2] >> BME280_PRESS_XLSB_Pos);
    data->pressure = bme280_compensate_P_int32(&c_data, t_fine, adc_p);
  }
  else { /* pressure not enabled */
    data->pressure = INT32_MIN;
  }

  // Check if humidity is enabled
  if (cfg->os_hum_cfg != BME280_HUMIDITY_OS_SKIP) {
    /* raw[6]=hum_msb raw[7]=hum_lsb */
    adc_h = ((uint32_t)raw[6] << BME280_HUM_MSB_SHIFT) | raw[7];
    data->humidity = bme280_compensate_H_int32(&c_data, t_fine, adc_h);
  }
  else {  /* humidity not enabled */
    data->humidity = INT32_MIN;
  }

  return BME280_STATUS_CODE_OK;
}

bme280_status_t bme280_set_mode(bme280_cfg *cfg, uint8_t target_mode)
{
  uint8_t ctrl_meas = 0;
  uint8_t transition_cmd = 0;
  uint8_t sleep_cmd = 0;

  /* Read ctrl_meas reg: for mode, osrs_p, and osrs_t */
  if (bme280_i2c_read_reg(BME280_CTRL_MEAS_ADDR, &ctrl_meas, 1, cfg)) {
    return BME280_ERROR_I2C_READ;
  }
  
  uint8_t current_mode = ctrl_meas & BME280_CTRL_MEAS_MODE_Msk;

  if (target_mode == current_mode) {
    return BME280_STATUS_CODE_OK; // same mode
  }

  /* BME280 needs to be in sleep mode to write cfg */
  if (current_mode != BME280_MODE_SLEEP) {
    /* transition cmd to set to sleep mode depending if 
     * it's normal mode or forced mode */
    sleep_cmd = ((current_mode == BME280_MODE_NORMAL) ?
                  BME280_MODE_NORMAL_TO_SLEEP :
                  BME280_MODE_FORCED_TO_SLEEP) &
                  BME280_CTRL_MEAS_MODE_Msk;
    
    /* retain osrs_t<7:5> & osrs_p<4:2> */
    sleep_cmd |= (ctrl_meas >> 2) << 2;
    
    // Write to ctrl_meas reg to transition modes to sleep mode
    if (bme280_i2c_write_reg(BME280_CTRL_MEAS_ADDR, &sleep_cmd, 1, cfg)) {
      return BME280_ERROR_I2C_WRITE;
    }

    /* Set current mode to sleep */
    current_mode = BME280_MODE_SLEEP;
  }
  
  /* return since we put mode to sleep */
  if (target_mode == BME280_MODE_SLEEP) {
    cfg->mode = BME280_MODE_SLEEP;
    return BME280_STATUS_CODE_OK;
  }

  transition_cmd = ((target_mode == BME280_MODE_NORMAL) ?
                     BME280_MODE_SLEEP_TO_NORMAL :
                     BME280_MODE_SLEEP_TO_FORCED) &
                     BME280_CTRL_MEAS_MODE_Msk;
  transition_cmd |= (ctrl_meas >> 2) << 2;

  /* Wait for measurement to clear before write */
  bme280_status_t status = bme280_wait_idle(cfg);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }
  
  // Write transition cmd to ctrl_meas reg to change to targeted mode
  if (bme280_i2c_write_reg(BME280_CTRL_MEAS_ADDR, &transition_cmd, 1, cfg)) {
    return BME280_ERROR_I2C_WRITE;
  }

  cfg->mode = target_mode;
  return BME280_STATUS_CODE_OK;
}


/* ========== STATIC FUNCTION DEFINITIONS ========== */

/**
 * @brief Reads calibration data for compensation formulas
 * 
 * @param[out] cfg : contains I2C handler
 * @param[out] c_data : Contains the calibration constants for compensation formulas
 */
static bme280_status_t bme280_read_calib_data(bme280_cfg *cfg, bme280_calib_data *c_data)
{
  /* TODO: FIRST GET THE TWO CHUNKS WITH BOTH PAIRS OF STARTING AND ENDING ADDRESS */
  /* Then mask and then append values to c_data */

  return BME280_STATUS_CODE_OK;
}

/**
 * @brief Reads current cfg values and appends to cfg
 * 
 * @param[out] cfg : Passes cfg to collect current cfg values from BME280
 */
static bme280_status_t bme280_read_cfg(bme280_cfg *cfg)
{
  /* 0xF2 -> 0xF5 , 4 bytes */
  uint8_t raw[BME280_CFG_LEN];
  if (bme280_i2c_read_reg(BME280_CFG_ADDR_START, raw,
                          BME280_CFG_LEN, cfg)) {
    return BME280_ERROR_I2C_READ; // I2C read error
  }

  /* raw[0] = ctrl_hum */
  cfg->os_hum_cfg  = (raw[0] & BME280_CTRL_HUM_OSRS_H_Msk) >> 
                      BME280_CTRL_HUM_OSRS_H_Pos;

  /* raw[1] = status - nothing for cfg */

  /* raw[2] = ctrl_meas */
  cfg->mode        = (raw[2] & BME280_CTRL_MEAS_MODE_Msk) >> 
                      BME280_CTRL_MEAS_MODE_Pos;
  cfg->os_pres_cfg = (raw[2] & BME280_CTRL_MEAS_OSRS_P_Msk) >> 
                      BME280_CTRL_MEAS_OSRS_P_Pos;
  cfg->os_temp_cfg = (raw[2] & BME280_CTRL_MEAS_OSRS_T_Msk) >> 
                      BME280_CTRL_MEAS_OSRS_T_Pos;

  /* raw[3] = config */
  cfg->filter      = (raw[3] & BME280_CONFIG_FILTER_Msk) >> 
                      BME280_CONFIG_FILTER_Pos;
  cfg->t_sb        = (raw[3] & BME280_CONFIG_T_SB_Msk) >>
                      BME280_CONFIG_T_SB_Pos;

  return BME280_STATUS_CODE_OK;
}

/**
 * @brief Writes cfg to BME280
 * 
 * @param[in] cfg : Config used to write to BME280
 * 
 * @return Result of execution status
 */
static bme280_status_t bme280_write_cfg(bme280_cfg *cfg)
{
  uint8_t ctrl_hum = cfg->os_hum_cfg & BME280_CTRL_HUM_OSRS_H_Msk;
  uint8_t ctrl_meas = ((cfg->os_temp_cfg << BME280_CTRL_MEAS_OSRS_T_Pos) & BME280_CTRL_MEAS_OSRS_T_Msk) |
                      ((cfg->os_pres_cfg << BME280_CTRL_MEAS_OSRS_P_Pos) & BME280_CTRL_MEAS_OSRS_P_Msk) |
                      (cfg->mode & BME280_CTRL_MEAS_MODE_Msk);
  uint8_t config = ((cfg->t_sb << BME280_CONFIG_T_SB_Pos) & BME280_CONFIG_T_SB_Msk) |
                   ((cfg->filter << BME280_CONFIG_FILTER_Pos) & BME280_CONFIG_FILTER_Msk) |
                   ((uint8_t)cfg->spi_enabled & BME280_CONFIG_SPI3W_EN_Msk);

  /* config reg reliably accepts writes in sleep mode */
  bme280_status_t status = bme280_set_mode(cfg, BME280_MODE_SLEEP);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }

  if (cfg->os_temp_cfg == BME280_TEMPERATURE_OS_SKIP &&
     (cfg->os_pres_cfg != BME280_PRESSURE_OS_SKIP ||
      cfg->os_hum_cfg  != BME280_HUMIDITY_OS_SKIP)) {
    return BME280_ERROR_TEMP_REQUIRED; /* can't skip temperature since it's used
                                        * to calculate pressure and humidity . */
  }

  if (bme280_i2c_write_reg(BME280_CONFIG_ADDR, &config, 1, cfg)) {
    return BME280_ERROR_I2C_WRITE;
  }

  /* ctrl_hum must be before ctrl_meas for writing */
  if (bme280_i2c_write_reg(BME280_CTRL_HUM_ADDR, &ctrl_hum, 1, cfg)) {
    return BME280_ERROR_I2C_WRITE;
  }

  if (bme280_i2c_write_reg(BME280_CTRL_MEAS_ADDR, &ctrl_meas, 1, cfg)) {
    return BME280_ERROR_I2C_WRITE;
  }

  return BME280_STATUS_CODE_OK;
}

/**
 * @brief Reads chip id and checks if chip id matches BME280_CHIP_ID
 * 
 * @param[in] cfg : Contains I2C address to communicate
 * 
 * @return Result of execution status
 */
static bme280_status_t bme280_read_id(bme280_cfg *cfg)
{
  uint8_t chip_id = 0;

  /* Read chip-id */
  bme280_status_t status = bme280_i2c_read_reg(BME280_CHIP_ID_ADDR, &chip_id, 1, cfg);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }

  /* chip_id does not match BME280's chip id: 0x60 */
  if (chip_id != BME280_CHIP_ID) {
    return BME280_ERROR_CHIP_ID;
  }

  return BME280_STATUS_CODE_OK;
}

/**
 * @brief Checks if the results have been transferred to the data registers
 *
 * @param[in] cfg : For I2C handler and I2C address
 * 
 * @return If function timed out
 */
static bme280_status_t bme280_wait_idle(bme280_cfg *cfg)
{
  uint8_t status;
  for (uint16_t i = 0; i < BME280_MEAS_TIMEOUT_MS; i++) {
    if (bme280_i2c_read_reg(BME280_STATUS_ADDR, &status, 1, cfg))
      return BME280_ERROR_I2C_READ;

    if (!(status & BME280_STATUS_MEASURING_Msk))
      return BME280_STATUS_CODE_OK;  /* idle -> done */

    HAL_Delay(1);
  }

  return BME280_ERROR_TIMEOUT; /* timed out */
}

/* ========== COMPENSATION FUNTION DEFINITIONS ========== */

/**
 * @brief Calculate temperature using adc values and calibration data
 * 
 * @return TODO: pg 25 for comment
 */
static int32_t bme280_compensate_T_int32(const bme280_calib_data *c_data, int32_t *t_fine, int32_t adc_T)
{
  // TODO
}

/**
 * @brief Calculate pressure using adc values and calibration data
 * 
 * @return TODO: pg 25 for comment
 */
static uint32_t bme280_compensate_P_int32(const bme280_calib_data *c_data, int32_t t_fine, int32_t adc_P)
{
  // TODO
}

/**
 * @brief Calculate humidity using adc values and calibration data
 * 
 * @return TODO: pg 25 for comment
 */
static uint32_t bme280_compensate_H_int32(const bme280_calib_data *c_data, int32_t t_fine, int32_t adc_H)
{
  // TODO
}


/* ========== I2C FUNCTION DEFINITIONS ========== */

/**
 * @brief Reads BME280 reg using HAL I2C mem read
 * 
 * @param memAddress : Starting address for BME280
 * @param[out] buf : Buffer value to read from register
 * @param len : Length of buffer
 * @param[in] cfg : For I2C handle and I2C address
 * 
 * @return Result of execution status
 */
static int bme280_i2c_read_reg(uint16_t memAddress, uint8_t *buf,
                               uint16_t len, bme280_cfg *cfg)
{
  return HAL_I2C_Mem_Read(cfg->hi2c, cfg->i2c_address,
                          memAddress, I2C_MEMADD_SIZE_8BIT, 
                          buf, len, HAL_MAX_DELAY);
}

/**
 * @brief Writes to BME280 reg using HAL I2C write
 * 
 * @param memAddress : Starting address for BME280
 * @param[in] buf : Buffer value to write to register
 * @param len : Length of buffer
 * @param[in] cfg : For I2C handle and I2C address
 * 
 * @return Result of execution status
 */
static int bme280_i2c_write_reg(uint16_t memAddress, uint8_t *buf,
                                uint16_t len, bme280_cfg *cfg)
{
  return HAL_I2C_Mem_Write(cfg->hi2c, cfg->i2c_address,
                           memAddress, I2C_MEMADD_SIZE_8BIT,
                           buf, len, HAL_MAX_DELAY);
}

// TODO: ADD HAL_ERROR_MESSAGES