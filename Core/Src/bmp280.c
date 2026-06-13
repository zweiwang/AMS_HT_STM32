#include "bmp280.h"
#include "main.h"

/* External I2C handle */
extern I2C_HandleTypeDef hi2c1;

/* Calibration data */
static BMP280_Calib_t bmp280_calib;

/**
  * @brief  Initialize BMP280 sensor
  * @retval 0 = success, 1 = failure
  */
uint8_t BMP280_Init(void)
{
    uint8_t chipid;
    uint8_t calib_data[24];
    uint8_t cmd;
    
    /* Send soft reset (0xB6 to register 0xE0) */
    {
        uint8_t rst_cmd = 0xB6;
        if (HAL_I2C_Mem_Write(&hi2c1, (BMP280_ADDRESS << 1), 0xE0, 1, &rst_cmd, 1, 100) != HAL_OK) {
            return 1;
        }
        HAL_Delay(10);  /* Wait for reset to complete */
    }
    
    /* Read and verify chip ID */
    if (HAL_I2C_Mem_Read(&hi2c1, (BMP280_ADDRESS << 1), BMP280_CHIPID_REG, 1, &chipid, 1, 100) != HAL_OK) {
        return 1;
    }
    if (chipid != 0x58) {
        return 1;  /* Invalid chip ID */
    }
    
    /* Read calibration parameters */
    if (HAL_I2C_Mem_Read(&hi2c1, (BMP280_ADDRESS << 1), BMP280_CALIB_REG, 1, calib_data, 24, 100) != HAL_OK) {
        return 1;
    }
    
    /* Parse calibration data */
    bmp280_calib.dig_T1 = (uint16_t)(calib_data[1] << 8 | calib_data[0]);
    bmp280_calib.dig_T2 = (int16_t)(calib_data[3] << 8 | calib_data[2]);
    bmp280_calib.dig_T3 = (int16_t)(calib_data[5] << 8 | calib_data[4]);
    
    bmp280_calib.dig_P1 = (uint16_t)(calib_data[7] << 8 | calib_data[6]);
    bmp280_calib.dig_P2 = (int16_t)(calib_data[9] << 8 | calib_data[8]);
    bmp280_calib.dig_P3 = (int16_t)(calib_data[11] << 8 | calib_data[10]);
    bmp280_calib.dig_P4 = (int16_t)(calib_data[13] << 8 | calib_data[12]);
    bmp280_calib.dig_P5 = (int16_t)(calib_data[15] << 8 | calib_data[14]);
    bmp280_calib.dig_P6 = (int16_t)(calib_data[17] << 8 | calib_data[16]);
    bmp280_calib.dig_P7 = (int16_t)(calib_data[19] << 8 | calib_data[18]);
    bmp280_calib.dig_P8 = (int16_t)(calib_data[21] << 8 | calib_data[20]);
    bmp280_calib.dig_P9 = (int16_t)(calib_data[23] << 8 | calib_data[22]);
    
    /* Configure ctrl_meas register (temperature oversampling 16x, pressure oversampling 8x, normal mode) */
    cmd = (0x05 << 5) | (0x04 << 2) | 0x03;  /* T_OSR=16x, P_OSR=8x, Mode=Normal */
    if (HAL_I2C_Mem_Write(&hi2c1, (BMP280_ADDRESS << 1), BMP280_CTRL_MEAS_REG, 1, &cmd, 1, 100) != HAL_OK) {
        return 1;
    }
    
    /* Configure config register (IIR filter coefficient) */
    cmd = (5 << 2);  /* Filter coefficient = 16 */
    if (HAL_I2C_Mem_Write(&hi2c1, (BMP280_ADDRESS << 1), BMP280_CONFIG_REG, 1, &cmd, 1, 100) != HAL_OK) {
        return 1;
    }
    
    HAL_Delay(100);
    
    return 0;  /* Success */
}

/**
  * @brief  Compensate raw temperature value
  * @param  adc_T: Raw temperature ADC value
  * @retval Compensated temperature in 0.01 DegC (e.g., 5123 = 51.23 DegC)
  */
static int32_t BMP280_CompensateT(int32_t adc_T)
{
    int32_t var1, var2, T;
    
    var1 = ((((adc_T >> 3) - ((int32_t)bmp280_calib.dig_T1 << 1))) * ((int32_t)bmp280_calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1))) >> 12) * ((int32_t)bmp280_calib.dig_T3)) >> 14;
    bmp280_calib.t_fine = var1 + var2;
    
    T = (bmp280_calib.t_fine * 5 + 128) >> 8;
    
    return T;
}

/**
  * @brief  Compensate raw pressure value
  * @param  adc_P: Raw pressure ADC value
  * @retval Compensated pressure in Pa (Q24.8 format)
  */
static uint32_t BMP280_CompensateP(int32_t adc_P)
{
    int64_t var1, var2, p;
    
    var1 = ((int64_t)bmp280_calib.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp280_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bmp280_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp280_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bmp280_calib.dig_P3) >> 8) + ((var1 * (int64_t)bmp280_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bmp280_calib.dig_P1) >> 33;
    
    if (var1 == 0) {
        return 0;
    }
    
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmp280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp280_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)bmp280_calib.dig_P7) << 4);
    
    return (uint32_t)p;
}

/**
  * @brief  Read temperature and pressure from BMP280
  * @param  data: Pointer to data structure
  * @retval 0 = success, 1 = failure
  */
uint8_t BMP280_Read(BMP280_Data_t *data)
{
    uint8_t raw_data[6];
    int32_t adc_temp, adc_pres;
    int32_t temp_comp;
    uint32_t pres_comp;
    
    if (data == NULL) {
        return 1;
    }
    
    /* Read pressure and temperature data (6 bytes from 0xF7) */
    if (HAL_I2C_Mem_Read(&hi2c1, (BMP280_ADDRESS << 1), BMP280_PRESSURE_REG, 1, raw_data, 6, 100) != HAL_OK) {
        return 1;
    }
    
    /* Parse raw pressure data (20-bit) */
    adc_pres = ((int32_t)raw_data[0] << 12) | ((int32_t)raw_data[1] << 4) | ((int32_t)raw_data[2] >> 4);
    
    /* Parse raw temperature data (20-bit) */
    adc_temp = ((int32_t)raw_data[3] << 12) | ((int32_t)raw_data[4] << 4) | ((int32_t)raw_data[5] >> 4);
    
    /* Compensate temperature */
    temp_comp = BMP280_CompensateT(adc_temp);
    data->temperature = (float)temp_comp / 100.0f;  /* Convert to Celsius */
    
    /* Compensate pressure */
    pres_comp = BMP280_CompensateP(adc_pres);
    data->pressure = (float)pres_comp / 256.0f;  /* Convert from Q24.8 format to Pa */
    
    return 0;  /* Success */
}
