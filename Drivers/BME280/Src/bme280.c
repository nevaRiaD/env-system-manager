#include "bme280.h"
#include "bme280_defs.h"
#include "stm32f4xx_hal_i2c.h"

/* static function prototypes */
static bme280_status_t bme280_read_calib_data(bme280_dev *dev);
static bme280_status_t bme280_cfg_init(bme280_dev *dev);
static bme280_status_t bme280_read_cfg(bme280_dev *dev);
static bme280_status_t bme280_write_cfg(bme280_dev *dev);
static bme280_status_t bme280_read_id(bme280_dev *dev);
static bme280_status_t bme280_wait_idle(bme280_dev *dev);

/* Compensation function prototypes */
static int32_t  bme280_compensate_T_int32(const bme280_calib_data *calib_data, int32_t *t_fine, int32_t adc_T);
static uint32_t bme280_compensate_P_int32(const bme280_calib_data *calib_data, int32_t t_fine, int32_t adc_p);
static uint32_t bme280_compensate_H_int32(const bme280_calib_data *calib_data, int32_t t_fine, int32_t adc_H);

/* I2C function prototypes */
static int8_t bme280_i2c_read_reg(bme280_dev *dev, uint16_t memAddress, uint8_t *buf, uint16_t len);
static int8_t bme280_i2c_write_reg(bme280_dev *dev, uint16_t memAddress, uint8_t *buf, uint16_t len);


/* ========== PUBLIC FUNCTION DEFINITIONS ========== */

bme280_status_t bme280_dev_init(bme280_dev *dev, bme280_intf_t intf, void *intf_handle)
{
  /* Return if pointer is null */
  if (dev == NULL || intf_handle == NULL) {
    return BME280_ERROR_NULL_PTR;
  }

  /* Append function pointers */
  switch(intf)
  {
    case BME280_SPI_ENABLED:
#ifdef HAL_SPI_MODULE_ENABLED
      dev->intf_handle.intf = intf;
      dev->intf_handle.hspi = intf_handle;
      dev->read = NULL; /* add functions for spi read */
      dev->write = NULL; /* add functions for spi write */
      dev->intf_handle.intf_addr = NULL;
#else /* SPI not enabled */
      return BME280_ERROR_SPI_NOT_ENABLED;
#endif
      break;
    case BME280_I2C_ENABLED:
#ifdef HAL_I2C_MODULE_ENABLED
      dev->intf_handle.intf_type = intf;
      dev->intf_handle.hi2c = intf_handle;
      dev->read = bme280_i2c_read_reg;
      dev->write = bme280_i2c_write_reg;
      dev->intf_handle.intf_addr = BME280_I2C_ADDR_SDO_LOW;
#else /* I2C not enabled */
      return BME280_ERROR_I2C_NOT_ENABLED;
#endif
      break;
    default:
      return BME280_ERROR_INTF_REQUIRED;
  }

  /* Read chip-id */
  bme280_status_t status = bme280_read_id(dev);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }

  /* Read calibration data */
  status = bme280_read_calib_data(dev);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }

  /* Initialize config */
  status = bme280_cfg_init(dev);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }
}

/**
 * @brief Initializes default values for cfg
 * 
 * @param[in] dev : Access cfg struct pointer
 * 
 * @return Result of execution status
 */
static bme280_status_t bme280_cfg_init(bme280_dev *dev)
{
  /* Set default cfg values */
  dev->cfg = (bme280_cfg){
    .mode        = BME280_MODE_NORMAL,
    .os_hum_cfg  = BME280_HUMIDITY_OS_1,
    .os_pres_cfg = BME280_PRESSURE_OS_1,
    .os_temp_cfg = BME280_TEMPERATURE_OS_1,
    .t_sb        = BME280_STANDBY_500_MS,
    .filter      = BME280_FILTER_COEFF_2,
  };

  /* Write cfg to BME280*/
  bme280_status_t status = bme280_write_cfg(dev);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }

  return BME280_STATUS_CODE_OK;
}

bme280_status_t bme280_read_data(bme280_dev *dev, bme280_data *data)
{
  uint8_t raw[BME280_DATA_LEN_HUM];
  uint16_t data_len = (dev->cfg.os_hum_cfg != BME280_HUMIDITY_OS_SKIP) ?
                       BME280_DATA_LEN_HUM : BME280_DATA_LEN_TEMP;
  
  /* adc values read from registers */
  int32_t t_fine;
  int32_t adc_t;
  int32_t adc_p;
  int32_t adc_h;

  if (dev->read(dev, BME280_DATA_ADDR_START, raw, data_len)) {
    return BME280_ERROR_I2C_READ; // I2C read error
  }

  /* Check if temperature is enabled, cannot calculate 
   * humidity and pressure without t_fine. 
  */
  if (dev->cfg.os_temp_cfg == BME280_TEMPERATURE_OS_SKIP) {
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
  data->temperature = bme280_compensate_T_int32(dev->calib_data, &t_fine, adc_t);
  
  /* Check if pressure is enabled */
  if (dev->cfg.os_pres_cfg != BME280_PRESSURE_OS_SKIP) {
    /* raw[0]=press_msb raw[1]=press_lsb raw[2]=press_xlsb<7:4> */
    adc_p = ((uint32_t)raw[0] << BME280_PRESS_MSB_SHIFT) |
            ((uint32_t)raw[1] << BME280_PRESS_LSB_SHIFT) |
            (raw[2] >> BME280_PRESS_XLSB_Pos);
    data->pressure = bme280_compensate_P_int32(dev->calib_data, t_fine, adc_p);
  }
  else { /* pressure not enabled */
    data->pressure = INT32_MIN;
  }

  // Check if humidity is enabled
  if (dev->cfg.os_hum_cfg != BME280_HUMIDITY_OS_SKIP) {
    /* raw[6]=hum_msb raw[7]=hum_lsb */
    adc_h = ((uint32_t)raw[6] << BME280_HUM_MSB_SHIFT) | raw[7];
    data->humidity = bme280_compensate_H_int32(dev->calib_data, t_fine, adc_h);
  }
  else {  /* humidity not enabled */
    data->humidity = INT32_MIN;
  }

  return BME280_STATUS_CODE_OK;
}

bme280_status_t bme280_set_mode(bme280_dev *dev, uint8_t target_mode)
{
  uint8_t ctrl_meas = 0;
  uint8_t transition_cmd = 0;
  uint8_t sleep_cmd = 0;

  /* Read ctrl_meas reg: for mode, osrs_p, and osrs_t */
  if (dev->read(dev, BME280_CTRL_MEAS_ADDR, &ctrl_meas, 1)) {
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
    if (dev->write(dev, BME280_CTRL_MEAS_ADDR, &sleep_cmd, 1)) {
      return BME280_ERROR_I2C_WRITE;
    }

    /* Set current mode to sleep */
    current_mode = BME280_MODE_SLEEP;
  }
  
  /* return since we put mode to sleep */
  if (target_mode == BME280_MODE_SLEEP) {
    dev->cfg.mode = BME280_MODE_SLEEP;
    return BME280_STATUS_CODE_OK;
  }

  transition_cmd = ((target_mode == BME280_MODE_NORMAL) ?
                     BME280_MODE_SLEEP_TO_NORMAL :
                     BME280_MODE_SLEEP_TO_FORCED) &
                     BME280_CTRL_MEAS_MODE_Msk;
  transition_cmd |= (ctrl_meas >> 2) << 2;

  /* Wait for measurement to clear before write */
  bme280_status_t status = bme280_wait_idle(dev);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }
  
  // Write transition cmd to ctrl_meas reg to change to targeted mode
  if (dev->write(dev, BME280_CTRL_MEAS_ADDR, &transition_cmd, 1)) {
    return BME280_ERROR_I2C_WRITE;
  }

  dev->cfg.mode = target_mode;
  return BME280_STATUS_CODE_OK;
}


/* ========== STATIC FUNCTION DEFINITIONS ========== */

/**
 * @brief Reads calibration data for compensation formulas
 * 
 * @param[out] dev : contains I2C handler
 * @param[out] calib_data : Contains the calibration constants for compensation formulas
 */
static bme280_status_t bme280_read_calib_data(bme280_dev *dev)
{
  uint8_t c_buf1[BME280_C_DATA1_BUF_SIZE];
  uint8_t c_buf2[BME280_C_DATA2_BUF_SIZE];

  bme280_status_t status = dev->read(dev, BME280_C_DATA1_ADDR_START, c_buf1, BME280_C_DATA1_BUF_SIZE);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }

  status = dev->read(dev, BME280_C_DATA2_ADDR_START, c_buf2, BME280_C_DATA2_BUF_SIZE);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }

  /* Define calibration data for c_buf1 */
  dev->calib_data->dig_T1 = (uint16_t)(((uint16_t)c_buf1[1] << 8u) | c_buf1[0]);
  dev->calib_data->dig_T2 = (int16_t)(((uint16_t)c_buf1[3] << 8u) | c_buf1[2]);
  dev->calib_data->dig_T3 = (int16_t)(((uint16_t)c_buf1[5] << 8u) | c_buf1[4]);

  dev->calib_data->dig_P1 = (uint16_t)(((uint16_t)c_buf1[7] << 8u) | c_buf1[6]);
  dev->calib_data->dig_P2 = (int16_t)(((uint16_t)c_buf1[9] << 8u) | c_buf1[8]);
  dev->calib_data->dig_P3 = (int16_t)(((uint16_t)c_buf1[11] << 8u) | c_buf1[10]);
  dev->calib_data->dig_P4 = (int16_t)(((uint16_t)c_buf1[13] << 8u) | c_buf1[12]);
  dev->calib_data->dig_P5 = (int16_t)(((uint16_t)c_buf1[15] << 8u) | c_buf1[14]);
  dev->calib_data->dig_P6 = (int16_t)(((uint16_t)c_buf1[17] << 8u) | c_buf1[16]);
  dev->calib_data->dig_P7 = (int16_t)(((uint16_t)c_buf1[19] << 8u) | c_buf1[18]);
  dev->calib_data->dig_P8 = (int16_t)(((uint16_t)c_buf1[21] << 8u) | c_buf1[20]);
  dev->calib_data->dig_P9 = (int16_t)(((uint16_t)c_buf1[23] << 8u) | c_buf1[22]);

  dev->calib_data->dig_H1 = c_buf1[25]; /* skip 0xA0 since dig_H1 is at 0xA1 */

  /* Define calibration data for c_buf2 */
  dev->calib_data->dig_H2 = (int16_t)(((uint16_t)c_buf2[1] << 8u) | c_buf2[0]);
  dev->calib_data->dig_H3 = (uint8_t)c_buf2[2];
  dev->calib_data->dig_H4 = (int16_t)(((uint16_t)c_buf2[3] << 4u) | (c_buf2[4] & BME280_DIG_H4_LSB_Msk));
  dev->calib_data->dig_H5 = (int16_t)(((uint16_t)c_buf2[5] << 4u) | (c_buf2[4] >> 4u));
  dev->calib_data->dig_H6 = (int8_t)c_buf2[6];

  return BME280_STATUS_CODE_OK;
}

/**
 * @brief Reads current cfg values and appends to dev->cfg
 * 
 * @param[out] dev : Passes dev to collect current cfg values from BME280
 */
static bme280_status_t bme280_read_cfg(bme280_dev *dev)
{
  /* 0xF2 -> 0xF5 , 4 bytes */
  uint8_t raw[BME280_CFG_LEN];
  if (dev->read(dev, BME280_CFG_ADDR_START, raw, BME280_CFG_LEN)) {
    return BME280_ERROR_I2C_READ; // I2C read error
  }

  /* raw[0] = ctrl_hum */
  dev->cfg.os_hum_cfg  = (raw[0] & BME280_CTRL_HUM_OSRS_H_Msk) >> 
                      BME280_CTRL_HUM_OSRS_H_Pos;

  /* raw[1] = status - nothing for cfg */

  /* raw[2] = ctrl_meas */
  dev->cfg.mode        = (raw[2] & BME280_CTRL_MEAS_MODE_Msk) >> 
                      BME280_CTRL_MEAS_MODE_Pos;
  dev->cfg.os_pres_cfg = (raw[2] & BME280_CTRL_MEAS_OSRS_P_Msk) >> 
                      BME280_CTRL_MEAS_OSRS_P_Pos;
  dev->cfg.os_temp_cfg = (raw[2] & BME280_CTRL_MEAS_OSRS_T_Msk) >> 
                      BME280_CTRL_MEAS_OSRS_T_Pos;

  /* raw[3] = config */
  dev->cfg.filter      = (raw[3] & BME280_CONFIG_FILTER_Msk) >> 
                      BME280_CONFIG_FILTER_Pos;
  dev->cfg.t_sb        = (raw[3] & BME280_CONFIG_T_SB_Msk) >>
                      BME280_CONFIG_T_SB_Pos;

  return BME280_STATUS_CODE_OK;
}

/**
 * @brief Writes cfg to BME280
 * 
 * @param[in] dev : Contains cfg used to write to BME280
 * 
 * @return Result of execution status
 */
static bme280_status_t bme280_write_cfg(bme280_dev *dev)
{
  uint8_t spi_enabled = (dev->intf_handle.intf_type == BME280_SPI_ENABLED) ? 1 : 0;
  uint8_t ctrl_hum = dev->cfg.os_hum_cfg & BME280_CTRL_HUM_OSRS_H_Msk;
  uint8_t ctrl_meas = ((dev->cfg.os_temp_cfg << BME280_CTRL_MEAS_OSRS_T_Pos) & BME280_CTRL_MEAS_OSRS_T_Msk) |
                      ((dev->cfg.os_pres_cfg << BME280_CTRL_MEAS_OSRS_P_Pos) & BME280_CTRL_MEAS_OSRS_P_Msk) |
                      (dev->cfg.mode & BME280_CTRL_MEAS_MODE_Msk);
  uint8_t config = ((dev->cfg.t_sb << BME280_CONFIG_T_SB_Pos) & BME280_CONFIG_T_SB_Msk) |
                   ((dev->cfg.filter << BME280_CONFIG_FILTER_Pos) & BME280_CONFIG_FILTER_Msk) |
                   (spi_enabled & BME280_CONFIG_SPI3W_EN_Msk);

  /* config reg reliably accepts writes in sleep mode */
  bme280_status_t status = bme280_set_mode(dev, BME280_MODE_SLEEP);
  if (status != BME280_STATUS_CODE_OK) {
    return status;
  }

  if (dev->cfg.os_temp_cfg == BME280_TEMPERATURE_OS_SKIP &&
     (dev->cfg.os_pres_cfg != BME280_PRESSURE_OS_SKIP ||
      dev->cfg.os_hum_cfg  != BME280_HUMIDITY_OS_SKIP)) {
    return BME280_ERROR_TEMP_REQUIRED; /* can't skip temperature since it's used
                                        * to calculate pressure and humidity . */
  }

  if (dev->write(dev, BME280_CONFIG_ADDR, &config, 1)) {
    return BME280_ERROR_I2C_WRITE;
  }

  /* ctrl_hum must be before ctrl_meas for writing */
  if (dev->write(dev, BME280_CTRL_HUM_ADDR, &ctrl_hum, 1)) {
    return BME280_ERROR_I2C_WRITE;
  }

  if (dev->write(dev, BME280_CTRL_MEAS_ADDR, &ctrl_meas, 1)) {
    return BME280_ERROR_I2C_WRITE;
  }

  return BME280_STATUS_CODE_OK;
}

/**
 * @brief Reads chip id and checks if chip id matches BME280_CHIP_ID
 * 
 * @param[in] dev : Contains I2C address to communicate
 * 
 * @return Result of execution status
 */
static bme280_status_t bme280_read_id(bme280_dev *dev)
{
  uint8_t chip_id = 0;

  /* Read chip-id */
  if (dev->read(dev, BME280_CHIP_ID_ADDR, &chip_id, 1)) {
    return BME280_ERROR_I2C_READ;
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
 * @param[in] dev : For I2C handler and I2C address
 * 
 * @return If function timed out
 */
static bme280_status_t bme280_wait_idle(bme280_dev *dev)
{
  /* register status */
  uint8_t status;

  for (uint16_t i = 0; i < BME280_MEAS_TIMEOUT_MS; i++) {
    if (dev->read(dev, BME280_STATUS_ADDR, &status, 1)) {
      return BME280_ERROR_I2C_READ;
    }

    if (!(status & BME280_STATUS_MEASURING_Msk)) {
      return BME280_STATUS_CODE_OK;  /* idle -> done */
    }

    HAL_Delay(1);
  }

  return BME280_ERROR_TIMEOUT; /* timed out */
}

/* ========== COMPENSATION FUNTION DEFINITIONS ========== */
/* NOTE: Compensation formulas used from BME280 datasheet */

/**
 * @brief Calculate temperature and t_fine using adc values and calibration data
 * 
 * @param[in] c_data : Calibration data used to calculate temperature
 * @param[out] t_fine : Carries fine resolution temp used to calculate pressure and humidity
 * @param[in] adc_T : Adc value recorded from BME280 to calculate temperature
 * 
 * @return Temperature in DegC, resolution is 0.01 DegC. Output value of "4123" equals 41.23 DegC
 */
static int32_t bme280_compensate_T_int32(const bme280_calib_data *c_data, int32_t *t_fine, int32_t adc_T)
{
  int32_t var1, var2;

  var1 = ((((adc_T >> 3) - ((int32_t)c_data->dig_T1 << 1))) * ((int32_t)c_data->dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)c_data->dig_T1)) * ((adc_T >> 4) - ((int32_t)c_data->dig_T1))) >> 12) * ((int32_t)c_data->dig_T3)) >> 14;

  *t_fine = var1 + var2;

  /* Return temperature */
  return ((*t_fine * 5 + 128) >> 8);
}

/**
 * @brief Calculate pressure using adc values and calibration data
 * 
 * @param[in] c_data : Calibration data
 * @param[in] t_fine : Fine resolution temperature
 * @param[in] adc_P : Adc pressure data recorded from BME280 to record pressure
 * 
 * @note Using 32bit function instead of 64 bit to calculate, refer to pg 50 of BME280 datasheet
 * 
 * @return Pressure in Pa as unsigned 32 bit integer. Output value of "94839" equals 94839 Pa = 948.39 hPa
 */
static uint32_t bme280_compensate_P_int32(const bme280_calib_data *c_data, int32_t t_fine, int32_t adc_P)
{
  int32_t var1, var2;
  uint32_t p;

  var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
  var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)c_data->dig_P6);
  var2 = var2 + ((var1 * ((int32_t)c_data->dig_P5)) << 1);
  var2 = (var2 >> 2) + (((int32_t)c_data->dig_P4) << 16);
  var1 = (((c_data->dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)c_data->dig_P2) * var1) >> 1)) >> 18;
  var1 = ((((32768 + var1)) * ((int32_t)c_data->dig_P1)) >> 15);

  if (var1 == 0) {
    return 0; /* avoid exception caused by division by zero */
  }

  p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
  if (p < 0x80000000) {
    p = (p << 1) / ((uint32_t)var1);
  }
  else {
    p = (p / (uint32_t)var1) * 2;
  }

  var1 = (((int32_t)c_data->dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
  var2 = (((int32_t)(p >> 2)) * ((int32_t)c_data->dig_P8)) >> 13;
  p = (uint32_t)((int32_t)p + ((var1 + var2 + c_data->dig_P7) >> 4));

  return p;
}

/**
 * @brief Calculate humidity using adc values, t_fine, and calibration data
 * 
 * @param[in] c_data : Calibration data
 * @param[in] t_fine : Fine resolution temperature
 * @param[in] adc_H : Adc humidity data recorded from BME280 to record humidity
 * 
 * @return TODO: pg 25 for comment
 */
static uint32_t bme280_compensate_H_int32(const bme280_calib_data *c_data, int32_t t_fine, int32_t adc_H)
{
  int32_t var1;

  var1 = t_fine - ((int32_t)76800);
  var1 = (((((adc_H << 14) - (((int32_t)c_data->dig_H4) << 20) - (((int32_t)c_data->dig_H5) * var1)) +
            ((int32_t)16384)) >> 15) * (((((((var1 * ((int32_t)c_data->dig_H6)) >> 10) * (((var1 *
            ((int32_t)c_data->dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) *
            ((int32_t)c_data->dig_H2) + 8192) >> 14));
  
  var1 = (var1 - (((((var1 >> 15) * (var1 >> 15)) >> 7) * ((int32_t)c_data->dig_H1)) >> 4));

  /* Check if var1 goes out of bounds */
  var1 = ((var1 < 0) ? 0 : var1);
  var1 = ((var1 > 419430400) ? 419430400 : var1);

  return (uint32_t)(var1 >> 12);
}


/* ========== I2C FUNCTION DEFINITIONS ========== */

/**
 * @brief Reads BME280 reg using HAL I2C mem read
 * 
 * @param memAddress : Starting address for BME280
 * @param[in] dev : For I2C handle and I2C address
 * @param[out] buf : Buffer value to read from register
 * @param len : Length of buffer
 * 
 * @return Result of execution status
 */
static int8_t bme280_i2c_read_reg(bme280_dev *dev, uint16_t memAddress, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Read(dev->intf_handle.hi2c, dev->intf_handle.intf_addr,
                          memAddress, I2C_MEMADD_SIZE_8BIT, 
                          buf, len, HAL_MAX_DELAY);
}

/**
 * @brief Writes to BME280 reg using HAL I2C write
 * 
 * @param memAddress : Starting address for BME280
 * @param[in] dev : For I2C handle and I2C address
 * @param[in] buf : Buffer value to write to register
 * @param len : Length of buffer
 * 
 * @return Result of execution status
 */
static int8_t bme280_i2c_write_reg(bme280_dev *dev, uint16_t memAddress, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Write(dev->intf_handle.hi2c, dev->intf_handle.intf_addr,
                           memAddress, I2C_MEMADD_SIZE_8BIT,
                           buf, len, HAL_MAX_DELAY);
}

// TODO: ADD HAL_ERROR_MESSAGES