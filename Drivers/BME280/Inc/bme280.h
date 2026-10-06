#ifndef BME280_H
#define BME280_H

#include "bme280_defs.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initializes dev struct, cfg, and calibration data
 * 
 * @param[out] dev 			: Device handle for BME280
 * @param[in] intf_type : Enum to select communication SPI or I2C
 * @param[in] h_intf 		: Pointer to insert handle for SPI or I2C
 * @param[in] mode 			: Optional initial mode. Pass BME280_MODE_DEFAULT for default.
 * 									 			Only BME280_MODE_NORMAL is honored; Any other value
 * 									 			starts the device asleep since forced mode is
 * 									 			is transient and bme280_read_data() already
 * 									 			performs measurements depending on mode.
 * 
 * @return Result of execution status
 */
bme280_status_t bme280_dev_init(bme280_dev *dev, bme280_intf_t intf_type, void *h_intf, bme280_mode_t mode);


/**
 * @brief Reads current cfg values and appends to dev->cfg
 * 
 * @param[out] dev : Passes dev to collect current cfg values from BME280
 */
bme280_status_t bme280_read_cfg(bme280_dev *dev);


/**
 * @brief Reads data for humidity, temp, and pressure
 * 
 * @param[in] dev 	: Contains intf handle and calibration data
 * @param[out] data : Struct to collect hum, temp, and pressure
 * 
 * @return Result of execution status
 */
bme280_status_t bme280_read_data(bme280_dev *dev, bme280_data *data);


/**
 * @brief Reads the current mode
 * 
 * @param[in] dev 					: Contains intf handle
 * @param[out] current_mode : Current mode the BME280 is on
 * 
 * @return Result of execution status
 */
bme280_status_t bme280_read_mode(bme280_dev *dev, bme280_mode_t *current_mode);


/**
 * @brief Sets mode for BME280
 * 
 * @param[in] dev 				: Contains intf handle
 * @param[in] target_mode : Sets target mode for BME280
 * 
 * @return Result of execution status
 */
bme280_status_t bme280_set_mode(bme280_dev *dev, bme280_mode_t target_mode);

#endif // BME280_H