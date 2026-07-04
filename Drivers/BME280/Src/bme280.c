#include "bme280.h"
#include "stm32f4xx_hal_i2c.h"

/* static function prototypes */
static int bme280_i2c_read_reg(uint16_t memAddress, uint8_t *buf,
                               uint16_t len, bme280_cfg *cfg);
static int bme280_i2c_write_reg(uint16_t memAddress, uint8_t *buf,
                                uint16_t len, bme280_cfg *cfg);
static int bme280_i2c_read_cfg(bme280_cfg *cfg);
static int bme280_wait_idle(bme280_cfg *cfg);


int bme280_init(bme280_cfg *cfg, I2C_HandleTypeDef *hi2c)
{
  *cfg = (bme280_cfg){
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
  
  /* Read cfg of BME280 using I2C */
  if (bme280_i2c_read_cfg(cfg)) return 1;

  return 0;
}

static int bme280_i2c_read_cfg(bme280_cfg *cfg)
{
  /* 0xF2 -> 0xF5 , 4 bytes */
  uint8_t raw[BME280_CFG_LEN];
  if (bme280_i2c_read_reg(BME280_CFG_ADDR_START, raw,
                          BME280_CFG_LEN, cfg)) {
    return 1; // I2C read error
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

  return 0;
}

int bme280_read_data(bme280_data *data, bme280_cfg *cfg)
{
  uint8_t raw[BME280_DATA_LEN_HUM];
  uint16_t data_len = (cfg->os_hum_cfg != BME280_HUMIDITY_OS_SKIP) ?
                       BME280_DATA_LEN_HUM : BME280_DATA_LEN_TEMP;

  if (bme280_i2c_read_reg(BME280_DATA_ADDR_START, raw,
                          data_len, cfg)) {
    return 1; // I2C read error
  }

  /* Check if pressure is enabled */
  if (cfg->os_pres_cfg != BME280_PRESSURE_OS_SKIP) {
    /* raw[0]=press_msb raw[1]=press_lsb raw[2]=press_xlsb<7:4> */
    data->pressure = ((uint32_t)raw[0] << BME280_PRESS_MSB_SHIFT) |
                     ((uint32_t)raw[1] << BME280_PRESS_LSB_SHIFT) |
                     (raw[2] >> BME280_PRESS_XLSB_Pos);
  }
  else { /* pressure not enabled */
    data->pressure = INT32_MIN;
  }

  /* Check if temperature is enabled */
  if (cfg->os_temp_cfg != BME280_TEMPERATURE_OS_SKIP) {
    /* raw[3]=temp_msb raw[4]=temp_lsb raw[5]=temp_xlsb<7:4> */
    data->temperature = ((uint32_t)raw[3] << BME280_TEMP_MSB_SHIFT) |
                        ((uint32_t)raw[4] << BME280_TEMP_LSB_SHIFT) |
                        (raw[5] >> BME280_TEMP_XLSB_Pos);
  }
  else { /* temperature not enabled */
    data->temperature = INT32_MIN;
  }

  // Check if humidity is enabled
  if (cfg->os_hum_cfg != BME280_HUMIDITY_OS_SKIP) {
    /* raw[6]=hum_msb raw[7]=hum_lsb */
    data->humidity = ((uint32_t)raw[6] << BME280_HUM_MSB_SHIFT) | raw[7];
  }
  else {  /* humidity not enabled */
    data->humidity = INT32_MIN;
  }

  return 0;
}

int bme280_set_mode(bme280_cfg *cfg, uint8_t target_mode)
{
  uint8_t ctrl_meas = 0;
  uint8_t transition_cmd = 0;
  uint8_t sleep_cmd = 0;

  if (bme280_i2c_read_reg(BME280_CTRL_MEAS_ADDR, &ctrl_meas, 1, cfg)) return 1;
  uint8_t current_mode = ctrl_meas & BME280_CTRL_MEAS_MODE_Msk;

  if (target_mode == current_mode) {
    return 0; // same mode
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
    if (bme280_i2c_write_reg(BME280_CTRL_MEAS_ADDR, &sleep_cmd, 1, cfg)) return 1;
    current_mode = BME280_MODE_SLEEP;
  }
  
  /* return since we put mode to sleep */
  if (target_mode == BME280_MODE_SLEEP) {
    cfg->mode = BME280_MODE_SLEEP;
    return 0;
  }

  transition_cmd = ((target_mode == BME280_MODE_NORMAL) ?
                     BME280_MODE_SLEEP_TO_NORMAL :
                     BME280_MODE_SLEEP_TO_FORCED) &
                     BME280_CTRL_MEAS_MODE_Msk;
  transition_cmd |= (ctrl_meas >> 2) << 2;

  /* Wait for measurement to clear before write */
  if (bme280_wait_idle(cfg)) return 1;

  if (bme280_i2c_write_reg(BME280_CTRL_MEAS_ADDR, &transition_cmd, 1, cfg)) return 1;

  cfg->mode = target_mode;
  return 0;
}

int bme280_write_cfg(bme280_cfg *cfg)
{
  uint8_t ctrl_hum = cfg->os_hum_cfg & BME280_CTRL_HUM_OSRS_H_Msk;
  uint8_t ctrl_meas = ((cfg->os_temp_cfg << BME280_CTRL_MEAS_OSRS_T_Pos) & BME280_CTRL_MEAS_OSRS_T_Msk) |
                      ((cfg->os_pres_cfg << BME280_CTRL_MEAS_OSRS_P_Pos) & BME280_CTRL_MEAS_OSRS_P_Msk) |
                      (cfg->mode & BME280_CTRL_MEAS_MODE_Msk);
  uint8_t config = ((cfg->t_sb << BME280_CONFIG_T_SB_Pos) & BME280_CONFIG_T_SB_Msk) |
                   ((cfg->filter << BME280_CONFIG_FILTER_Pos) & BME280_CONFIG_FILTER_Msk) |
                   ((uint8_t)cfg->spi_enabled & BME280_CONFIG_SPI3W_EN_Msk);

  /* config reg reliably accepts writes in sleep mode */
  if (bme280_set_mode(cfg, BME280_MODE_SLEEP)) {
    return 1; // failed to set to sleep mode
  }
  if (bme280_i2c_write_reg(BME280_CONFIG_ADDR, &config, 1, cfg)) return 1;

  /* ctrl_hum must be before ctrl_meas for writing */
  if (bme280_i2c_write_reg(BME280_CTRL_HUM_ADDR, &ctrl_hum, 1, cfg)) return 1;
  if (bme280_i2c_write_reg(BME280_CTRL_MEAS_ADDR, &ctrl_meas, 1, cfg)) return 1;

  return 0;
}

static int bme280_i2c_read_reg(uint16_t memAddress, uint8_t *buf,
                               uint16_t len, bme280_cfg *cfg)
{
  return HAL_I2C_Mem_Read(cfg->hi2c, cfg->i2c_address,
                          memAddress, I2C_MEMADD_SIZE_8BIT, 
                          buf, len, HAL_MAX_DELAY);
}

static int bme280_i2c_write_reg(uint16_t memAddress, uint8_t *buf,
                                uint16_t len, bme280_cfg *cfg)
{
  return HAL_I2C_Mem_Write(cfg->hi2c, cfg->i2c_address,
                           memAddress, I2C_MEMADD_SIZE_8BIT,
                           buf, len, HAL_MAX_DELAY);
}

/**
 * @brief Checks if the results have been transferred to the data registers
 *        
 */
static int bme280_wait_idle(bme280_cfg *cfg)
{
  uint8_t status;
  for (uint8_t i = 0; i < BME280_MEAS_TIMEOUT_MS; i++) {
    if (bme280_i2c_read_reg(BME280_STATUS_ADDR, &status, 1, cfg)) return 1;
    if (!(status & BME280_STATUS_MEASURING_Msk)) return 0;  /* idle -> done */
  }
  return 1; /* timed out */
}

// TODO: ADD HAL_ERROR_MESSAGES