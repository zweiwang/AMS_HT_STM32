#ifndef __AHT20_H
#define __AHT20_H

#include "main.h"

/* AHT20 I2C slave address */
#define AHT20_ADDRESS           0x38

/* AHT20 commands */
#define AHT20_INIT_CMD          0xBE
#define AHT20_RESET_CMD         0xBA
#define AHT20_MEASURE_CMD       0xAC

/* AHT20 status register bits */
#define AHT20_CALIB_OK          0x08
#define AHT20_BUSY              0x80

/* Data structure */
typedef struct {
    float temperature;  /* Temperature in Celsius */
    float humidity;     /* Humidity in % */
} AHT20_Data_t;

/* Function prototypes */
uint8_t AHT20_Init(void);
uint8_t AHT20_ReadStatus(void);
uint8_t AHT20_Read(AHT20_Data_t *data);

#endif /* __AHT20_H */
