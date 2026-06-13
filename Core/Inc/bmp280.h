#ifndef __BMP280_H
#define __BMP280_H

#include "main.h"

/* BMP280 I2C slave address */
#define BMP280_ADDRESS          0x77

/* BMP280 register addresses */
#define BMP280_CHIPID_REG       0xD0
#define BMP280_CTRL_MEAS_REG    0xF4
#define BMP280_CONFIG_REG       0xF5
#define BMP280_PRESSURE_REG     0xF7
#define BMP280_TEMPERATURE_REG  0xFA
#define BMP280_CALIB_REG        0x88

/* BMP280 calibration data structure */
typedef struct {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    int32_t t_fine;
} BMP280_Calib_t;

/* Data structure */
typedef struct {
    float temperature;  /* Temperature in Celsius */
    float pressure;     /* Pressure in Pa */
} BMP280_Data_t;

/* Function prototypes */
uint8_t BMP280_Init(void);
uint8_t BMP280_Read(BMP280_Data_t *data);

#endif /* __BMP280_H */
