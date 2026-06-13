#include "am2301.h"

/* 精确延时函数 */
static void AM2301_DelayUS(uint32_t us)
{
    volatile uint32_t i;
    /* 48MHz时钟，每个NOP约20ns，需要 us*50 个NOP ≈ 1us */
    for (i = 0; i < us * 12; i++)
    {
        __NOP();
    }
}

/**
  * @brief  初始化AM2301
  */
void AM2301_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 启用GPIO时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 配置PB9为开漏输出（单总线） + 内部上拉 */
    GPIO_InitStruct.Pin = AM2301_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;      /* 开漏输出 */
    GPIO_InitStruct.Pull = GPIO_PULLUP;              /* 内部上拉 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;    /* 高速 */
    HAL_GPIO_Init(AM2301_PORT, &GPIO_InitStruct);

    /* 初始状态拉高 */
    HAL_GPIO_WritePin(AM2301_PORT, AM2301_PIN, GPIO_PIN_SET);
    
    /* 等待1秒传感器准备稳定 */
    for (uint16_t i = 0; i < 1000; i++)
    {
        AM2301_DelayUS(1000);  /* 分散延时避免看门狗 */
    }
}

/**
  * @brief  读取一个bit (单总线) 
  * @retval bit值 (0 or 1)
  */
static uint8_t AM2301_ReadBit(void)
{
    uint8_t bit = 0;
    uint16_t timeout = 200;  /* 加长超时到200us */

    /* 等待线路被拉低 (传感器发送数据前拉低) */
    while (HAL_GPIO_ReadPin(AM2301_PORT, AM2301_PIN) && timeout)
    {
        AM2301_DelayUS(1);
        timeout--;
    }

    if (!timeout) return 0;

    AM2301_DelayUS(30);  /* 等待30μs */

    /* 判断线路电平：1为高，0为低 */
    if (HAL_GPIO_ReadPin(AM2301_PORT, AM2301_PIN))
    {
        bit = 1;
    }
    else
    {
        bit = 0;
    }

    /* 等待线路释放 (加长超时到200us) */
    timeout = 200;
    while (!HAL_GPIO_ReadPin(AM2301_PORT, AM2301_PIN) && timeout)
    {
        AM2301_DelayUS(1);
        timeout--;
    }

    return bit;
}

/**
  * @brief  读取一个字节 (单总线)
  * @retval 字节值
  */
static uint8_t AM2301_ReadByte(void)
{
    uint8_t byte = 0;

    for (uint8_t i = 0; i < 8; i++)
    {
        byte <<= 1;
        byte |= (AM2301_ReadBit() & 0x01);
    }

    return byte;
}

/**
  * @brief  读取AM2301的温度和湿度
  * @retval 0=成功, -1=失败
  */
int AM2301_Read(AM2301_Data_t *data)
{
    uint8_t buf[5] = {0};
    uint16_t humidity_raw;
    int16_t temperature_raw;

    if (!data) return AM2301_ERROR;

    /* 主机拉低至少18ms唤醒传感器 */
    HAL_GPIO_WritePin(AM2301_PORT, AM2301_PIN, GPIO_PIN_RESET);
    
    /* 使用微秒延时避免阻塞其他任务 */
    for (uint16_t i = 0; i < 20; i++)
    {
        AM2301_DelayUS(1000);  /* 20ms总等待 */
    }

    /* 释放总线 */
    HAL_GPIO_WritePin(AM2301_PORT, AM2301_PIN, GPIO_PIN_SET);
    AM2301_DelayUS(30);

    /* 等待传感器响应（拉低80μs然后拉高80μs） */
    uint16_t timeout = 200;  /* 加长到200us */
    while (HAL_GPIO_ReadPin(AM2301_PORT, AM2301_PIN) && timeout)
    {
        AM2301_DelayUS(1);
        timeout--;
    }

    if (!timeout) return AM2301_ERROR;  /* 无响应 */

    /* 等待传感器拉高 (加长到200us) */
    timeout = 200;
    while (!HAL_GPIO_ReadPin(AM2301_PORT, AM2301_PIN) && timeout)
    {
        AM2301_DelayUS(1);
        timeout--;
    }

    if (!timeout) return AM2301_ERROR;

    /* 读取5个字节 (湿度高8位 + 湿度低8位 + 温度高8位 + 温度低8位 + 校验和) */
    for (uint8_t i = 0; i < 5; i++)
    {
        buf[i] = AM2301_ReadByte();
    }

    /* 校验 */
    uint8_t checksum = buf[0] + buf[1] + buf[2] + buf[3];
    if (checksum != buf[4])
    {
        return AM2301_ERROR;  /* 校验失败 */
    }

    /* 提取湿度 */
    humidity_raw = (buf[0] << 8) | buf[1];
    data->humidity = humidity_raw * 0.1f;

    /* 提取温度 (有符号) */
    temperature_raw = (buf[2] << 8) | buf[3];
    if (temperature_raw & 0x8000)  /* 负数 */
    {
        temperature_raw = -(temperature_raw & 0x7FFF);
    }
    data->temperature = temperature_raw * 0.1f;

    data->checksum = buf[4];

    return AM2301_OK;
}
