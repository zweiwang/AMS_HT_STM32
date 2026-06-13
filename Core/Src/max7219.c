#include "max7219.h"

/* 
 * 7段显示段码定义 (共阴极)
 *  bit7 bit6 bit5 bit4 bit3 bit2 bit1 bit0
 *   DP    a    b    c    d    e    f    g
 * 
 *      a
 *    f   b
 *      g
 *    e   c
 *      d    DP
 */
#define SEG_A  0x40
#define SEG_B  0x20
#define SEG_C  0x10
#define SEG_D  0x08
#define SEG_E  0x04
#define SEG_F  0x02
#define SEG_G  0x01
#define SEG_DP 0x80

/* ASCII字符到段码的映射表 (ASCII 0x20-0x7F) */
static const uint8_t ascii_to_seg[128] = {
    /* 0x00-0x1F: 不可见字符，全灭 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    
    /* 0x20: 空格 */
    0x00,  /* 空格 */
    
    /* 0x21: ! - 无法显示 */
    0x00,
    
    /* 0x22: " - 无法显示 */
    0x00,
    
    /* 0x23: # - 无法显示 */
    0x00,
    
    /* 0x24: $  S类似 */
    SEG_A|SEG_F|SEG_G|SEG_C|SEG_D,  /* 5 */
    
    /* 0x25: % - 无法显示 */
    0x00,
    
    /* 0x26: & - 无法显示 */
    0x00,
    
    /* 0x27: ' - 无法显示 */
    0x00,
    
    /* 0x28: ( - C类似 */
    SEG_A|SEG_F|SEG_E|SEG_D,
    
    /* 0x29: ) - 无法显示 */
    0x00,
    
    /* 0x2A: * - 无法显示 */
    0x00,
    
    /* 0x2B: + - 上半部分类似 */
    SEG_B|SEG_C|SEG_F|SEG_G,
    
    /* 0x2C: , - 无法显示 */
    0x00,
    
    /* 0x2D: - 减号 */
    SEG_G,
    
    /* 0x2E: . - 小数点（特殊处理） */
    0x00,
    
    /* 0x2F: / - 无法显示 */
    0x00,
    
    /* 0x30-0x39: 0-9 数字 */
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,        /* 0 */
    SEG_B|SEG_C,                                  /* 1 */
    SEG_A|SEG_B|SEG_G|SEG_E|SEG_D,                /* 2 */
    SEG_A|SEG_B|SEG_G|SEG_C|SEG_D,                /* 3 */
    SEG_F|SEG_G|SEG_B|SEG_C,                      /* 4 */
    SEG_A|SEG_F|SEG_G|SEG_C|SEG_D,                /* 5 */
    SEG_A|SEG_F|SEG_G|SEG_C|SEG_D|SEG_E,          /* 6 */
    SEG_A|SEG_B|SEG_C,                            /* 7 */
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,    /* 8 */
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_F|SEG_G,          /* 9 */
    
    /* 0x3A: : - 无法显示 */
    0x00,
    
    /* 0x3B: ; - 无法显示 */
    0x00,
    
    /* 0x3C: < - 无法显示 */
    0x00,
    
    /* 0x3D: = 等号 */
    SEG_G|SEG_D,
    
    /* 0x3E: > - 无法显示 */
    0x00,
    
    /* 0x3F: ? - 无法显示 */
    0x00,
    
    /* 0x40: @ - 无法显示 */
    0x00,
    
    /* 0x41-0x5A: 大写字母 A-Z */
    SEG_A|SEG_B|SEG_C|SEG_E|SEG_F|SEG_G,  /* A */
    SEG_F|SEG_E|SEG_G|SEG_C|SEG_D,        /* B (小写b) */
    SEG_A|SEG_F|SEG_E|SEG_D,              /* C */
    SEG_B|SEG_C|SEG_G|SEG_E|SEG_D,        /* D (小写d) */
    SEG_A|SEG_F|SEG_G|SEG_E|SEG_D,        /* E */
    SEG_A|SEG_F|SEG_G|SEG_E,              /* F */
    SEG_A|SEG_F|SEG_E|SEG_D|SEG_C,        /* G */
    SEG_F|SEG_G|SEG_B|SEG_E|SEG_C,        /* H */
    SEG_B|SEG_C,                          /* I */
    SEG_B|SEG_C|SEG_D|SEG_E,              /* J */
    0x00,                                 /* K - 无法显示 */
    SEG_F|SEG_E|SEG_D,                    /* L */
    0x00,                                 /* M - 无法显示 */
    SEG_E|SEG_G|SEG_C,                    /* N (小写n) */
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,  /* O */
    SEG_A|SEG_B|SEG_E|SEG_F|SEG_G,        /* P */
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_F|SEG_G,  /* Q */
    SEG_E|SEG_G,                          /* R (小写r) */
    SEG_A|SEG_F|SEG_G|SEG_C|SEG_D,        /* S */
    SEG_F|SEG_E|SEG_G|SEG_D,              /* T (小写t) */
    SEG_F|SEG_E|SEG_D|SEG_C|SEG_B,        /* U */
    0x00,                                 /* V - 无法显示 */
    0x00,                                 /* W - 无法显示 */
    0x00,                                 /* X - 无法显示 */
    SEG_F|SEG_G|SEG_B|SEG_C|SEG_D,        /* Y */
    SEG_A|SEG_B|SEG_G|SEG_E|SEG_D,        /* Z */
    
    /* 0x5B: [ - C类似 */
    SEG_A|SEG_F|SEG_E|SEG_D,
    
    /* 0x5C: \ - 无法显示 */
    0x00,
    
    /* 0x5D: ] - 无法显示 */
    0x00,
    
    /* 0x5E: ^ - 无法显示 */
    0x00,
    
    /* 0x5F: _ 下划线 */
    SEG_D,
    
    /* 0x60: ` - 无法显示 */
    0x00,
    
    /* 0x61-0x7A: 小写字母 a-z (多数使用大写显示) */
    SEG_A|SEG_B|SEG_C|SEG_E|SEG_F|SEG_G,  /* a -> A */
    SEG_F|SEG_E|SEG_G|SEG_C|SEG_D,        /* b */
    SEG_G|SEG_E|SEG_D,                    /* c */
    SEG_B|SEG_C|SEG_G|SEG_E|SEG_D,        /* d */
    SEG_A|SEG_F|SEG_G|SEG_E|SEG_D,        /* e -> E */
    SEG_A|SEG_F|SEG_G|SEG_E,              /* f -> F */
    SEG_A|SEG_F|SEG_E|SEG_D|SEG_C|SEG_G,  /* g */
    SEG_F|SEG_G|SEG_B|SEG_E|SEG_C,        /* h */
    SEG_B|SEG_C,                          /* i -> I */
    SEG_B|SEG_C|SEG_D|SEG_E,              /* j -> J */
    0x00,                                 /* k - 无法显示 */
    SEG_F|SEG_E|SEG_D,                    /* l -> L */
    0x00,                                 /* m - 无法显示 */
    SEG_E|SEG_G|SEG_C,                    /* n */
    SEG_G|SEG_E|SEG_D|SEG_C,              /* o */
    SEG_A|SEG_B|SEG_E|SEG_F|SEG_G,        /* p -> P */
    SEG_A|SEG_B|SEG_C|SEG_F|SEG_G,        /* q */
    SEG_E|SEG_G,                          /* r */
    SEG_A|SEG_F|SEG_G|SEG_C|SEG_D,        /* s -> S */
    SEG_F|SEG_E|SEG_G|SEG_D,              /* t */
    SEG_F|SEG_E|SEG_D|SEG_C|SEG_B,        /* u */
    0x00,                                 /* v - 无法显示 */
    0x00,                                 /* w - 无法显示 */
    0x00,                                 /* x - 无法显示 */
    SEG_F|SEG_G|SEG_B|SEG_C|SEG_D,        /* y */
    SEG_A|SEG_B|SEG_G|SEG_E|SEG_D,        /* z -> Z */
    
    /* 0x7B-0x7F: 其他符号 */
    0x00,  /* { */
    0x00,  /* | */
    0x00,  /* } */
    0x00,  /* ~ */
    0x00   /* DEL */
};

/* 前向声明 */
void MAX7219_WriteReg_Dual(uint8_t addr1, uint8_t data1, uint8_t addr2, uint8_t data2);

/**
  * @brief  初始化MAX7219
  */
void MAX7219_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 启用GPIO时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 配置CS引脚 */
    GPIO_InitStruct.Pin = MAX7219_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MAX7219_CS_PORT, &GPIO_InitStruct);

    /* CS拉高 */
    HAL_GPIO_WritePin(MAX7219_CS_PORT, MAX7219_CS_PIN, GPIO_PIN_SET);

    HAL_Delay(100);

    /* 配置两个级联的MAX7219 */
    MAX7219_WriteReg_Dual(MAX7219_DISPLAYTEST, 0x00, MAX7219_DISPLAYTEST, 0x00);
    MAX7219_WriteReg_Dual(MAX7219_SHUTDOWN, 0x01, MAX7219_SHUTDOWN, 0x01);
    MAX7219_WriteReg_Dual(MAX7219_SCANLIMIT, 0x07, MAX7219_SCANLIMIT, 0x07);
    MAX7219_WriteReg_Dual(MAX7219_DECODEMODE, 0x00, MAX7219_DECODEMODE, 0x00);
    MAX7219_WriteReg_Dual(MAX7219_INTENSITY, 0x01, MAX7219_INTENSITY, 0x01);

    HAL_Delay(10);

    /* 清屏 */
    for (uint8_t i = 0; i < 8; i++)
    {
        MAX7219_WriteReg_Dual(MAX7219_DIGIT0 + i, 0x00, MAX7219_DIGIT0 + i, 0x00);
        HAL_Delay(2);
    }
}

/**
  * @brief  向MAX7219写入寄存器
  * @param  addr: 寄存器地址
  * @param  data: 数据
  */
void MAX7219_WriteReg(uint8_t addr, uint8_t data)
{
    uint8_t tx_data[2] = {addr, data};

    /* CS拉低 */
    HAL_GPIO_WritePin(MAX7219_CS_PORT, MAX7219_CS_PIN, GPIO_PIN_RESET);
    
    for (volatile int i = 0; i < 50; i++);

    /* SPI发送 */
    HAL_SPI_Transmit(&hspi1, tx_data, 2, 100);

    /* 等待完成 */
    while (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY));
    
    for (volatile int i = 0; i < 50; i++);

    /* CS拉高 */
    HAL_GPIO_WritePin(MAX7219_CS_PORT, MAX7219_CS_PIN, GPIO_PIN_SET);
    
    for (volatile int i = 0; i < 50; i++);
}

/**
  * @brief  显示字符串 (直接使用ASCII映射表)
  */
void MAX7219_DisplayStr(const char *str)
{
    uint8_t seg_codes[8] = {0};
    uint8_t count = 0;

    while (*str && count < 8)
    {
        if (*str == '.')
        {
            /* 小数点：添加到前一个字符 */
            if (count > 0)
            {
                seg_codes[count - 1] |= SEG_DP;
            }
        }
        else if ((uint8_t)*str < 128)
        {
            /* 查表得到段码 */
            seg_codes[count++] = ascii_to_seg[(uint8_t)*str];
        }
        else
        {
            /* 无效字符，空格 */
            seg_codes[count++] = 0;
        }
        str++;
    }

    /* 输出到数码管 */
    for (uint8_t i = 0; i < 8; i++)
    {
        MAX7219_WriteReg(MAX7219_DIGIT0 + i, seg_codes[7 - i]);  /* 反序 */
    }
}

/**
  * @brief  向两个级联的MAX7219写入寄存器 (第二个芯片 + 第一个芯片)
  * @param  addr1: 第一个芯片的寄存器地址
  * @param  data1: 第一个芯片的数据
  * @param  addr2: 第二个芯片的寄存器地址
  * @param  data2: 第二个芯片的数据
  */
void MAX7219_WriteReg_Dual(uint8_t addr1, uint8_t data1, uint8_t addr2, uint8_t data2)
{
    uint8_t tx_data[4] = {addr2, data2, addr1, data1};  /* 第二个芯片先发，第一个芯片后发 */

    /* CS拉低 */
    HAL_GPIO_WritePin(MAX7219_CS_PORT, MAX7219_CS_PIN, GPIO_PIN_RESET);
    
    for (volatile int i = 0; i < 50; i++);

    /* SPI发送4个字节 */
    HAL_SPI_Transmit(&hspi1, tx_data, 4, 100);

    /* 等待完成 */
    while (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY));
    
    for (volatile int i = 0; i < 50; i++);

    /* CS拉高 */
    HAL_GPIO_WritePin(MAX7219_CS_PORT, MAX7219_CS_PIN, GPIO_PIN_SET);
    
    for (volatile int i = 0; i < 50; i++);
}

/**
  * @brief  显示两个MAX7219的字符串 (级联模式) - 使用ASCII映射表
  * @param  str1: 第一个芯片显示的字符串
  * @param  str2: 第二个芯片显示的字符串
  */
void MAX7219_DisplayStr_Dual(const char *str1, const char *str2)
{
    uint8_t seg_codes1[8] = {0};
    uint8_t seg_codes2[8] = {0};
    uint8_t count = 0;

    /* 第一个字符串 */
    count = 0;
    while (*str1 && count < 8)
    {
        if (*str1 == '.')
        {
            /* 小数点：添加到前一个字符 */
            if (count > 0)
            {
                seg_codes1[count - 1] |= SEG_DP;
            }
        }
        else if ((uint8_t)*str1 < 128)
        {
            /* 查表得到段码 */
            seg_codes1[count++] = ascii_to_seg[(uint8_t)*str1];
        }
        else
        {
            /* 无效字符，空格 */
            seg_codes1[count++] = 0;
        }
        str1++;
    }

    /* 第二个字符串 */
    count = 0;
    while (*str2 && count < 8)
    {
        if (*str2 == '.')
        {
            /* 小数点：添加到前一个字符 */
            if (count > 0)
            {
                seg_codes2[count - 1] |= SEG_DP;
            }
        }
        else if ((uint8_t)*str2 < 128)
        {
            /* 查表得到段码 */
            seg_codes2[count++] = ascii_to_seg[(uint8_t)*str2];
        }
        else
        {
            /* 无效字符，空格 */
            seg_codes2[count++] = 0;
        }
        str2++;
    }

    /* 同时输出到两个芯片 */
    for (uint8_t i = 0; i < 8; i++)
    {
        MAX7219_WriteReg_Dual(MAX7219_DIGIT0 + i, seg_codes1[7 - i], 
                               MAX7219_DIGIT0 + i, seg_codes2[7 - i]);
    }
}
