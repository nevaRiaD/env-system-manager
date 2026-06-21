#ifndef BME280_H
#define BME280_H

#include "bme280_defs.h"
#include <stdint.h>

/**
 * @brief Contains config data from memory map for bme280
 *
 * Detailed description explaining when to use this configuration
 * and any constraints on its lifecycle.
 */
typedef struct bme280_cfg {
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
    uint8_t mode;           /* Power Settings for mode */

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
    uint8_t os_hum_cfg;     /* Controls oversampling for humidity*/
    uint8_t os_pres_cfg;    /* Controls oversampling for pressure */   
    uint8_t os_temp_cfg;    /* Controls oversampling for temperature */

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
    uint8_t t_sb;           /* Controls inactive duration for t_standby*/

    /* Filter Config
     *
     * 0b000: Filter off
     * 0b001: Filter Coefficient = 2
     * 0b010: Filter Coefficient = 4
     * 0b011: Filter Coefficient = 8
     * 0b100: Filter Coefficient = 16
     */
    uint8_t filter;         /* Controls time constant of the IIR filter */

    uint8_t spi_enabled;    /* Enables 3-wire SPI interface when set to ‘1’ */
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

int bme280_read_cfg(bme280_cfg *cfg);
int bme280_read_data(bme280_data *data);

#endif // BME280_H