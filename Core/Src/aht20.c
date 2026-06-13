#include "aht20.h"
#include "main.h"

/* External I2C handle */
extern I2C_HandleTypeDef hi2c1;

/**
  * @brief  Read AHT20 status register (no register address needed)
  * @retval Status byte
  */
uint8_t AHT20_ReadStatus(void)
{
    uint8_t status;
    /* AHT20: Read status by sending START + device address + read + data + STOP (no register address) */
    HAL_I2C_Master_Receive(&hi2c1, (AHT20_ADDRESS << 1) | 1, &status, 1, 100);
    return status;
}

/**
  * @brief  Initialize AHT20 sensor
  * @retval 0 = success, 1 = failure
  */
uint8_t AHT20_Init(void)
{
    uint8_t cmd[3];
    uint8_t retry = 0;
    uint8_t reset_cmd = AHT20_RESET_CMD;
    uint8_t status = 0;
    
    HAL_Delay(40);
    
    /* Send soft reset command (0xBA) */
    if (HAL_I2C_Master_Transmit(&hi2c1, (AHT20_ADDRESS << 1), &reset_cmd, 1, 100) != HAL_OK) {
        return 1;
    }
    HAL_Delay(20);
    
    /* Send init command (0xBE) + parameters (0x08, 0x00) */
    cmd[0] = AHT20_INIT_CMD;
    cmd[1] = 0x08;
    cmd[2] = 0x00;
    if (HAL_I2C_Master_Transmit(&hi2c1, (AHT20_ADDRESS << 1), cmd, 3, 100) != HAL_OK) {
        return 1;
    }
    
    /* Wait for initialization to complete (typically 10ms) */
    HAL_Delay(50);
    
    /* Wait for calibration to complete (Bit 3 should be set) */
    retry = 0;
    while (retry < 10) {
        HAL_Delay(50);
        status = AHT20_ReadStatus();
        /* Check if in normal mode and calibration is enabled (Bit 3 = 1, Bit[7:6] = 0) */
        /* In normal operation: status & 0x68 should equal 0x08 */
        if ((status & 0x68) == 0x08) {
            return 0;  /* Success */
        }
        retry++;
    }
    
    return 1;  /* Initialization failed */
}

/**
  * @brief  Read temperature and humidity from AHT20
  * @param  data: Pointer to data structure
  * @retval 0 = success, 1 = failure (timeout), 2 = I2C error
  */
uint8_t AHT20_Read(AHT20_Data_t *data)
{
    uint8_t cmd[3];
    uint8_t raw_data[7];
    uint32_t adc_temp, adc_hum;
    uint16_t retry = 0;
    uint8_t status = 0;
    HAL_StatusTypeDef hal_status;
    
    if (data == NULL) {
        return 2;
    }
    
    /* Send measurement command (0xAC) with parameters (0x33, 0x00) */
    cmd[0] = AHT20_MEASURE_CMD;
    cmd[1] = 0x33;
    cmd[2] = 0x00;
    hal_status = HAL_I2C_Master_Transmit(&hi2c1, (AHT20_ADDRESS << 1), cmd, 3, 100);
    if (hal_status != HAL_OK) {
        return 2;  /* I2C error */
    }
    
    /* Wait for measurement to complete (max 80ms) */
    /* Poll status register Bit 7 (busy flag). When Bit 7 = 0, measurement is done */
    retry = 0;
    while (retry < 100) {
        HAL_Delay(1);
        status = AHT20_ReadStatus();
        /* Check if measurement is complete (Busy bit = 0) */
        if ((status & 0x80) == 0) {
            break;  /* Measurement complete */
        }
        retry++;
    }
    
    if (retry >= 100) {
        return 1;  /* Timeout - measurement did not complete */
    }
    
    /* Add small delay before reading data */
    HAL_Delay(1);
    
    /* Read 7 bytes of data directly (no register address) */
    hal_status = HAL_I2C_Master_Receive(&hi2c1, (AHT20_ADDRESS << 1) | 1, raw_data, 7, 100);
    if (hal_status != HAL_OK) {
        return 2;  /* I2C error */
    }
    
    /* Data format:
     * Byte 0: Status register
     * Byte 1-2 + upper 4 bits of Byte 3: Humidity data (20-bit)
     * Lower 4 bits of Byte 3 + Byte 4-5: Temperature data (20-bit)
     */
    
    /* Parse humidity (20-bit) from bytes 1, 2, and upper 4 bits of byte 3 */
    adc_hum = ((uint32_t)raw_data[1]) << 12;
    adc_hum |= ((uint32_t)raw_data[2]) << 4;
    adc_hum |= ((uint32_t)(raw_data[3] >> 4)) & 0x0F;
    adc_hum &= 0xFFFFF;
    
    /* Parse temperature (20-bit) from lower 4 bits of byte 3, byte 4, and byte 5 */
    adc_temp = ((uint32_t)(raw_data[3] & 0x0F)) << 16;
    adc_temp |= ((uint32_t)raw_data[4]) << 8;
    adc_temp |= ((uint32_t)raw_data[5]);
    adc_temp &= 0xFFFFF;
    
    /* Convert to actual values using formulas from datasheet */
    /* Temperature = (raw / 2^20) * 200 - 50  (in Celsius) */
    /* Humidity = (raw / 2^20) * 100  (in %) */
    data->temperature = (float)adc_temp / 1048576.0f * 200.0f - 50.0f;
    data->humidity = (float)adc_hum / 1048576.0f * 100.0f;
    
    return 0;  /* Success */
}
