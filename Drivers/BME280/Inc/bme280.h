#ifndef BME280_H
#define BME280_H

#include "bme280_defs.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initializes dev struct, cfg, and calibration data
 * 
 * @param[out] dev : Device handle for BME280
 * @param[in] intf : Enum to select communication SPI or I2C
 * @param[in] h_intf : Pointer to insert handle for SPI or I2C
 * 
 * @return Result of execution status
 */
bme280_status_t bme280_dev_init(bme280_dev *dev, bme280_intf_t intf, void *h_intf);

/**
 * @brief TODO
 * 
 * @param[in] cfg : TODO
 * 
 * @return Result of execution status
 */
bme280_status_t bme280_read_cfg(bme280_dev *dev);

/**
 * @brief Reads data for humidity, temp, and pressure
 * 
 * @param[out] data : Struct to collect hum, temp, and pressure
 * @param[in] cfg : TODO
 * 
 * @return Result of execution status
 */
bme280_status_t bme280_read_data(bme280_dev *dev, bme280_data *data);

/**
 * @brief Sets mode for BME280
 * 
 * @param[in] cfg : TODO
 * @param target_mode : Sets target mode for BME280
 * 
 * @return Result of execution status
 */
bme280_status_t bme280_set_mode(bme280_dev *dev, uint8_t target_mode);

#endif // BME280_H