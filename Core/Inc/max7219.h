#ifndef __MAX7219_H
#define __MAX7219_H

#include "main.h"

/* MAX7219 寄存器地址 */
#define MAX7219_NOOP        0x00
#define MAX7219_DIGIT0      0x01
#define MAX7219_DIGIT1      0x02
#define MAX7219_DIGIT2      0x03
#define MAX7219_DIGIT3      0x04
#define MAX7219_DIGIT4      0x05
#define MAX7219_DIGIT5      0x06
#define MAX7219_DIGIT6      0x07
#define MAX7219_DIGIT7      0x08
#define MAX7219_DECODEMODE  0x09
#define MAX7219_INTENSITY   0x0A
#define MAX7219_SCANLIMIT   0x0B
#define MAX7219_SHUTDOWN    0x0C
#define MAX7219_DISPLAYTEST 0x0F

/* CS管脚定义 */
#define MAX7219_CS_PORT     GPIOB
#define MAX7219_CS_PIN      GPIO_PIN_8   /* PB8 */

/* 外部声明 */
extern SPI_HandleTypeDef hspi1;

/* 函数声明 */
void MAX7219_Init(void);
void MAX7219_WriteReg(uint8_t addr, uint8_t data);
void MAX7219_DisplayNum(uint8_t *digits);
void MAX7219_DisplayStr(const char *str);
void MAX7219_WriteReg_Dual(uint8_t addr1, uint8_t data1, uint8_t addr2, uint8_t data2);
void MAX7219_DisplayNum_Dual(uint8_t *digits1, uint8_t *digits2);
void MAX7219_DisplayStr_Dual(const char *str1, const char *str2);

#endif /* __MAX7219_H */
