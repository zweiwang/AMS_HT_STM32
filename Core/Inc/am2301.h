#ifndef __AM2301_H
#define __AM2301_H

#include "main.h"

/* AM2301 引脚定义 */
#define AM2301_PORT     GPIOB
#define AM2301_PIN      GPIO_PIN_9       /* PB9 - 单总线 */

/* 返回值 */
#define AM2301_OK       0
#define AM2301_ERROR    -1

/* 结构体：存储温度和湿度 */
typedef struct
{
    float temperature;   /* 温度 (-40~80°C) */
    float humidity;      /* 湿度 (0~100%) */
    uint8_t checksum;    /* 校验和 */
} AM2301_Data_t;

/* 函数声明 */
void AM2301_Init(void);
int AM2301_Read(AM2301_Data_t *data);

#endif /* __AM2301_H */
