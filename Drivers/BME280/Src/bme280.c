#include "bme280.h"
#include "stm32f4xx_hal_i2c.h"

/* static function prototypes */
static int bme280_i2c_read_reg(uint8_t reg, uint8_t *buf, uint16_t len, I2C_HandleTypeDef *hi2c);
static int bme280_i2c_write_reg(uint8_t reg, uint8_t val);

int bme280_init(bme280_cfg *cfg, I2C_HandleTypeDef *hi2c)
{
	cfg->hi2c = hi2c;
	cfg->spi_enabled = false;

	/* Read cfg of BME280 using I2C */
	if (!bme280_i2c_read_cfg(cfg)) {
		// TODO: Insert Console Error print statement
		return 1;
	}
}

int bme280_read_data(bme280_data *data, bme280_cfg *cfg)
{
	uint16_t data_len = cfg->hum_enabled ? BME280_DATA_LEN_HUM : BME280_DATA_LEN_TEMP;
	uint8_t raw[data_len];
	bme280_i2c_read_reg(BME280_DATA_ADDR_START, raw, data_len, cfg->hi2c);

	/* Read buffer and append values */
	
}

int bme280_i2c_read_cfg(bme280_cfg *cfg)
{
	/* 0xF2 -> 0xF5 , 4 bytes */
	uint8_t raw[BME280_CFG_LEN];
	bme280_i2c_read_reg(BME280_CFG_ADDR_START, raw, BME280_CFG_LEN, cfg->hi2c);

	for (int i = 0; i < BME280_CFG_LEN; i++) {
		switch(i)
		{
		/* ctrl_hum: 0xF2 */
		case 0:
			cfg->os_hum_cfg = 1;
			continue;
		/* status: 0xF3 */
		case 1:
			/* nothing related to cfg */
			continue;
		/* ctrl_meas: 0xF4 */
		case 2:
			cfg->mode = 1;
			cfg->os_pres_cfg = 1;
			cfg->os_temp_cfg = 1;
			continue;
		/* config: 0xF5 */
		case 3:
			cfg->filter = 1;
			cfg->t_sb = 1;
			continue;
		}
	}
}

static int bme280_i2c_read_reg(uint8_t reg, uint8_t *buf, uint16_t len, I2C_HandleTypeDef *hi2c)
{
	return HAL_I2C_Mem_Read(hi2c, uint16_t DevAddress,
							uint16_t MemAddress, uint16_t MemAddSize, 
							uint8_t *pData, uint16_t Size, uint32_t Timeout);
}

