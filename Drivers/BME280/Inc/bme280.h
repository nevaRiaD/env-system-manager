#ifndef BME280_H
#define BME280_H

#include "bme280_defs.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief TODO
 * 
 * @param[in] cfg : TODO
 * @param[in] hi2c : Attachs I2C handle to cfg
 * 
 * @return Result of execution status
 * @retval 
 */
bme280_status_t bme280_init(bme280_cfg *cfg, I2C_HandleTypeDef *hi2c);

/**
 * @brief TODO
 * 
 * @param[in] cfg : TODO
 * 
 * @return Result of execution status
 */
bme280_status_t bme280_read_cfg(bme280_cfg *cfg);

/**
 * @brief Reads data for humidity, temp, and pressure
 * 
 * @param[out] data : Struct to collect hum, temp, and pressure
 * @param[in] cfg : TODO
 * 
 * @return Result of execution status
 */
bme280_status_t bme280_read_data(bme280_data *data, bme280_cfg *cfg);

/**
 * @brief Sets mode for BME280
 * 
 * @param[in] cfg : TODO
 * @param target_mode : Sets target mode for BME280
 * 
 * @return Result of execution status
 */
bme280_status_t bme280_set_mode(bme280_cfg *cfg, uint8_t target_mode);

#endif // BME280_H