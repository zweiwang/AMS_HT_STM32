/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "max7219.h"
#include "aht20.h"
#include "bmp280.h"
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint32_t ledTickMs = 0;
uint8_t ledState = 0;
uint32_t displayUpdateTick = 0;
AHT20_Data_t aht20_data = {0, 0};
BMP280_Data_t bmp280_data = {0, 0};

/* EC11编码器变量 */
uint32_t ec11_value = 300;  /* 默认值5分钟 = 300秒 */
uint8_t ec11_countdown_mode = 0;  /* 0=正常模式，1=倒计时模式 */
uint8_t temp_threshold_mode = 0;  /* 0=正常，1=温度阈值配置模式 */
uint8_t temp_threshold_high = 65;  /* 高温阈值，默认65°C */
uint8_t temp_threshold_low = 55;   /* 低温阈值，默认55°C = 65-10 */
uint8_t temp_threshold_edit = 65;  /* 编辑中的温度阈值 */
uint32_t ec11_sw_press_start_time = 0;  /* 按键按下开始时间，用于长按检测 */
uint8_t ec11_sw_long_press_handled = 0;  /* 长按已处理标志 */
uint8_t ec11_prev_clk = 1;
uint8_t ec11_prev_sw = 1;
uint32_t ec11_sw_press_time = 0;
uint32_t ec11_last_process_time = 0;
uint8_t ec11_stable_clk = 1;
uint8_t ec11_state = 0;  /* 旋转状态机 */
uint32_t ec11_last_rotate_time = 0;  /* 上次旋转时间，用于加速计算 */
uint8_t ec11_accel = 1;  /* 旋转加速度，越快值越大 */

/* 蜂鸣器变量 */
uint8_t beep_mode = 0;  /* 0=停止，1=循环蜂鸣 */
uint32_t beep_start_time = 0;  /* 蜂鸣开始时间 */
uint32_t beep_cycle_time = 0;
uint8_t beep_state = 0;  /* 0=安静，1=响 */

/* 数码管轮播变量 */
uint8_t display_mode = 0;  /* 0=AHT20温湿度，1=BMP280温度气压 */
uint32_t display_switch_time = 0;
uint32_t system_run_time = 0;  /* 系统运行时间秒数，用于HH.MM.SS显示 */

/* ESP8266变量 */
uint8_t esp_connected = 0;  /* 0=未连接，1=已连接WiFi */
uint8_t esp_error = 0;      /* 0=正常，1=出错显示9999 */
uint8_t esp_wifi_disconnected = 0;  /* WiFi断开标记 */
uint8_t esp_waiting_response = 0;  /* 0=可发送，1=等待响应 */
uint32_t esp_init_time = 0;
uint32_t esp_step_delay_time = 0;  /* 步骤间延迟计时，独立于超时 */
uint8_t esp_init_step = 0;  /* 初始化步骤 */
uint8_t esp_retry_count = 0;  /* 重试计数 */
uint8_t esp_ip[4] = {0, 0, 0, 0};  /* IP地址四段 */
uint32_t esp_ip_last_query = 0;  /* 上次查询IP的时间 */
uint8_t esp_ip_idx = 0;  /* 当前显示的IP段索引 */
uint32_t esp_ip_switch_time = 0;  /* IP段切换时间 */
uint8_t esp_ip_query_done = 0;  /* IP查询完成标志 */
uint8_t esp_rx_buf[512];  /* 接收缓冲区 */
uint8_t esp_rx_len = 0;  /* 接收长度 */
uint32_t esp_busy_delay = 0;  /* busy状态延迟重试 */

/* HTTP响应发送状态机 */
uint8_t http_send_state = 0;  /* 0=空闲，1=已发送CIPSEND等待>，2=已发送响应 */
uint32_t http_send_time = 0;
uint8_t http_pending_link = 0;
uint32_t http_request_start = 0;  /* HTTP请求开始时间，超时自动关闭 */

/* 显示缓冲区 - 全局变量，避免栈问题，保证显示刷新快速 */
char display1_buf[16] = "0-00.00.00";
char display2_buf[16] = "025.3045.7";
uint32_t display_refresh_tick = 0;  /* 显示刷新定时器，每50ms刷新 */
uint8_t pa8_prev = 1;               /* PA8上次状态，用于EMI检测后重配MAX7219 */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* 初始化MAX7219 */
  MAX7219_Init();
  
  /* 初始化AHT20温湿度传感器 */
  HAL_Delay(100);
  AHT20_Init();
  HAL_Delay(100);
  
  /* 初始化BMP280气压温度传感器 */
  BMP280_Init();
  
  /* ESP8266初始化时间 */
  esp_init_time = HAL_GetTick();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* EC11编码器处理 - 简单可靠的边沿检测 */
    uint8_t ec11_sw = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
    
    /* 高频率采样CLK信号 */
    uint8_t curr_clk = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);
    
    /* 只在CLK下降沿检测方向，最稳定的检测方式 */
    if (curr_clk == 0 && ec11_stable_clk == 1)
    {
      /* CLK下降沿：读取DT的值判断方向 */
      uint8_t curr_dt = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);
      
      /* 计算旋转加速度 - 时间越短加速越大 */
      uint32_t rotate_interval = HAL_GetTick() - ec11_last_rotate_time;
      ec11_last_rotate_time = HAL_GetTick();
      
      /* 适度加速：旋转越快步进越大 */
      if (rotate_interval < 15)
        ec11_accel = 15;    /* 极快旋转，大步进 */
      else if (rotate_interval < 30)
        ec11_accel = 12;
      else if (rotate_interval < 60)
        ec11_accel = 8;
      else if (rotate_interval < 100)
        ec11_accel = 4;
      else if (rotate_interval < 150)
        ec11_accel = 2;
      else
        ec11_accel = 1;
      
      /* DT=0 → 顺时针(加数值), DT=1 → 逆时针(减数值) */
      if (curr_dt == 0)
      {
        /* 顺时针旋转 - 增加数值 */
        if (temp_threshold_mode)
        {
          if (temp_threshold_edit < 100)
            temp_threshold_edit += 1;
        }
        else if (!ec11_countdown_mode && ec11_value < 9999)
        {
          if (ec11_value + ec11_accel <= 9999)
            ec11_value += ec11_accel;
          else
            ec11_value = 9999;
        }
      }
      else
      {
        /* 逆时针旋转 - 减少数值 */
        if (temp_threshold_mode)
        {
          if (temp_threshold_edit > 30)
            temp_threshold_edit -= 1;
        }
        else if (!ec11_countdown_mode && ec11_value > 0)
        {
          if (ec11_value >= ec11_accel)
            ec11_value -= ec11_accel;
          else
            ec11_value = 0;
        }
      }
    }
    ec11_stable_clk = curr_clk;
    
    /* 按键按下检测 - 非阻塞软件滤波，长按检测 */
    if (ec11_sw == 0 && ec11_prev_sw == 1)
    {
      /* 50ms防抖 - 不阻塞，只记录时间 */
      if (HAL_GetTick() - ec11_sw_press_time > 50)
      {
        ec11_sw_press_time = HAL_GetTick();
        ec11_sw_press_start_time = HAL_GetTick();  /* 记录按下开始时间 */
        ec11_sw_long_press_handled = 0;  /* 重置长按处理标志 */
      }
      ec11_prev_sw = 0;  /* 立即更新prev_sw，避免重复触发 */
    }
    
    /* 按键保持按下状态，检测长按 */
    if (ec11_sw == 0 && ec11_sw_long_press_handled == 0)
    {
      /* 长按1秒检测 */
      if (HAL_GetTick() - ec11_sw_press_start_time >= 1000)
      {
        ec11_sw_long_press_handled = 1;  /* 标记长按已处理 */
        
        /* 长按：进入/退出温度阈值配置模式 */
        if (!temp_threshold_mode)
        {
          /* 进入配置模式：停止倒计时和加热，加载当前阈值 */
          temp_threshold_mode = 1;
          temp_threshold_edit = temp_threshold_high;  /* 加载当前高温阈值 */
          
          /* 进入配置模式时暂停倒计时和加热 */
          if (ec11_countdown_mode)
          {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);  /* 关闭电热丝 */
          }
        }
        else
        {
          /* 退出配置模式：保存设置 */
          temp_threshold_high = temp_threshold_edit;
          temp_threshold_low = temp_threshold_high - 10;  /* 低温=高温-10 */
          temp_threshold_mode = 0;
          
          /* 如果之前在倒计时模式，恢复加热（根据当前温度） */
          if (ec11_countdown_mode && ec11_value > 0)
          {
            /* 回温开启：使用新阈值判断是否需要加热 */
            if (aht20_data.temperature < temp_threshold_low)
            {
              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);  /* 打开电热丝 */
            }
          }
        }
      }
    }
    
    /* 按键释放检测 - 短按处理 */
    if (ec11_sw == 1 && ec11_prev_sw == 0)
    {
      ec11_prev_sw = 1;  /* 立即更新prev_sw */
      
      /* 如果长按已处理过，不处理短按 */
      if (ec11_sw_long_press_handled == 0)
      {
        /* 短按：正常操作（必须在非配置模式下） */
        if (!temp_threshold_mode)
        {
          /* 正常模式：切换倒计时模式 */
          uint8_t old_mode = ec11_countdown_mode;
          ec11_countdown_mode = !ec11_countdown_mode;
          
          /* 如果是启动倒计时，先检查值是否为0 */
          if (old_mode == 0 && ec11_countdown_mode == 1)
          {
            if (ec11_value == 0)
            {
              /* 值为0时不能启动倒计时，恢复为正常模式 */
              ec11_countdown_mode = 0;
            }
            else
            {
              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);  /* 低电平开电热丝 */
            }
          }
          
          /* 如果是停止倒计时，关闭电热丝 */
          if (old_mode == 1 && ec11_countdown_mode == 0)
          {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);  /* 高电平关电热丝 */
          }
          
           /* 按下按键同时停止蜂鸣器循环 */
           if (beep_mode)
           {
             beep_mode = 0;
             beep_state = 0;
             HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
             /* 停止蜂鸣后重置默认时间为5分钟(300秒) */
             ec11_value = 300;
           }
        }
      }
    }
    /* 如果按键状态未变化（持续按下或持续释放），不更新prev_sw */
    /* prev_sw已在按下/释放检测时立即更新 */
    
    /* 蜂鸣器循环模式: 2秒周期，响0.5秒停1.5秒，持续10分钟 */
    if (beep_mode)
    {
       /* 10分钟超时自动停止 */
       if (HAL_GetTick() - beep_start_time >= 600000)  /* 10分钟 = 600000ms */
       {
         beep_mode = 0;
         beep_state = 0;
         HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
         /* 超时停止蜂鸣后重置默认时间为5分钟(300秒) */
         ec11_value = 300;
       }
      else
      {
        uint32_t cycle_pos = (HAL_GetTick() - beep_cycle_time) % 2000;
        if (cycle_pos < 500 && beep_state == 0)
        {
          /* 开始响 */
          beep_state = 1;
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
        }
        else if (cycle_pos >= 500 && beep_state == 1)
        {
          /* 停止响 */
          beep_state = 0;
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
        }
      }
    }
    
    /* ESP8266接收处理 - 检测OK和解析IP */
    uint8_t rx_byte;
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE))
    {
      rx_byte = (uint8_t)(huart1.Instance->DR & (uint8_t)0x00FF);
      
      /* 存入接收缓冲区，换行时解析 */
      if (esp_rx_len < 511)
      {
        esp_rx_buf[esp_rx_len++] = rx_byte;
        esp_rx_buf[esp_rx_len] = 0;
      }
      
      /* 检测>提示符 - 必须在换行检测前，因为>后没有换行 */
      if (http_send_state == 1 && rx_byte == '>')
      {
        /* 构建完整的HTTP响应，填充空格到512字节 */
        char http_response[513];  /* 512 + null */
        memset(http_response, ' ', 512);  /* 先全部填空格 */
        http_response[512] = 0;
        
        /* 准备响应数据 */
        int temp_int = (int)aht20_data.temperature;
        int temp_dec = (int)((aht20_data.temperature - temp_int) * 10);
        int hum_int = (int)aht20_data.humidity;
        int hum_dec = (int)((aht20_data.humidity - hum_int) * 10);
        int bmp_temp_int = (int)bmp280_data.temperature;
        int bmp_temp_dec = (int)((bmp280_data.temperature - bmp_temp_int) * 10);
        int pres_kpa = (int)(bmp280_data.pressure / 1000);
        
        /* 倒计时 HH:MM:SS */
        int countdown_hours = ec11_value / 3600;
        int countdown_mins = (ec11_value % 3600) / 60;
        int countdown_secs = ec11_value % 60;
        
        /* 加热状态 */
        const char *heat_status = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_RESET ? "ON" : "OFF";
        
        /* 构建响应体 */
        char body[256];
        int body_len = snprintf(body, sizeof(body),
          "AHT20: %d.%d C, %d.%d %%RH\r\n"
          "BMP280: %d.%d C, %d kPa\r\n"
          "Countdown: %02d:%02d:%02d\r\n"
          "Temp Threshold: %d-%d C\r\n"
          "Heating: %s\r\n",
          temp_int, temp_dec, hum_int, hum_dec,
          bmp_temp_int, bmp_temp_dec, pres_kpa,
          countdown_hours, countdown_mins, countdown_secs,
          temp_threshold_low, temp_threshold_high,
          heat_status);
        
        /* 构建HTTP头 */
        char header[128];
        int header_len = snprintf(header, sizeof(header),
          "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", body_len);
        
        /* 安全复制：确保不超过512字节缓冲区 */
        int copy_len = header_len + body_len;
        if (copy_len > 511) copy_len = 511;
        memcpy(http_response, header, header_len);
        memcpy(http_response + header_len, body, copy_len - header_len);
        http_response[copy_len] = '\0';
        
        /* 发送完整的512字节响应 */
        HAL_UART_Transmit(&huart1, (uint8_t*)http_response, 512, 10000);
        
        /* 不立即关闭连接，延迟200ms后在状态2关闭 */
        http_send_time = HAL_GetTick();
        http_send_state = 2;  /* 状态2: 等待延迟后关闭 */
      }
      
      /* 换行时解析IP地址和检测ERROR/OK */
      if (rx_byte == '\n' || rx_byte == '\r')
      {
        if (esp_rx_len > 0)
        {
          /* 检测ERROR - 收到响应，清除等待标志 */
          if (strstr((char*)esp_rx_buf, "ERROR") != NULL)
          {
            esp_error = 1;      /* 标记出错，显示9999 */
            esp_waiting_response = 0;  /* 收到响应，清除等待标志 */
          }
          
          /* 检测OK表示连接成功，清除等待标志 */
          if (strstr((char*)esp_rx_buf, "OK") != NULL)
          {
            esp_connected = 1;
            esp_waiting_response = 0;  /* 收到响应，清除等待标志 */
            esp_error = 0;             /* 清除错误标记 */
          }
          
          /* 检测WIFI GOT IP - 只标记连接，查询IP在步骤3执行 */
          if (strstr((char*)esp_rx_buf, "GOT IP") != NULL)
          {
            esp_connected = 1;
            esp_wifi_disconnected = 0;
            /* 只有在步骤2/3等待WiFi连接时才清除等待标志 */
            if (esp_init_step == 2 || esp_init_step == 3)
            {
              esp_waiting_response = 0;
            }
          }
          
          /* 检测WIFI DISCONNECTED - WiFi断开（先检测，避免被CONNECTED误匹配） */
          if (strstr((char*)esp_rx_buf, "DISCONNECTED") != NULL)
          {
            esp_wifi_disconnected = 1;  /* 标记WiFi断开 */
            esp_connected = 0;          /* 清除连接标记 */
            esp_waiting_response = 0;   /* 清除等待标志 */
            esp_ip_query_done = 0;      /* 重置IP查询标志，重连后再次查询 */
            esp_init_step = 2;          /* 回退到步骤2，重新连接WiFi */
          }
          /* 检测WIFI CONNECTED - 只标记连接，不查询IP（不包含DIS的才是真正CONNECTED） */
          else if (strstr((char*)esp_rx_buf, "CONNECTED") != NULL)
          {
            esp_connected = 1;
            esp_wifi_disconnected = 0;  /* 清除断开标记 */
            /* 只有在步骤2/3等待WiFi连接时才清除等待标志，其他情况不干扰 */
            if (esp_init_step == 2 || esp_init_step == 3)
            {
              esp_waiting_response = 0;  /* 清除等待标志，继续下一步 */
            }
          }
          
          /* 检测busy也表示收到响应，清除等待标志，不标记错误 */
          if (strstr((char*)esp_rx_buf, "busy") != NULL)
          {
            esp_waiting_response = 0;  /* busy也是响应，清除等待标志重试 */
            esp_error = 0;
          }
          
          /* 检测SEND OK: HTTP响应发送成功，立即复位状态机 */
          if (strstr((char*)esp_rx_buf, "SEND OK") != NULL && http_send_state == 2)
          {
            http_send_state = 0;
            esp_rx_len = 0;
            memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
          }
          
          /* 检测CLOSED: 连接关闭，强制复位HTTP状态机 */
          if (strstr((char*)esp_rx_buf, "CLOSED") != NULL && http_send_state != 0)
          {
            http_send_state = 0;
            esp_rx_len = 0;
            memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
          }
          
          /* 检测CONNECT FAIL: 强制复位HTTP状态机 */
          if (strstr((char*)esp_rx_buf, "FAIL") != NULL && http_send_state != 0)
          {
            http_send_state = 0;
            esp_rx_len = 0;
            memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
          }
          
          /* 匹配AT+CIPSTA?返回的IP格式: ip:"x.x.x.x" */
          char *ip_pos = strstr((char*)esp_rx_buf, "ip:\"");
          if (ip_pos != NULL)
          {
            int ip1, ip2, ip3, ip4;
            /* ip:\" 共4个字符(i p : "), 跳过到数字 */
            if (sscanf(ip_pos + 4, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4) == 4)
            {
              esp_ip[0] = (uint8_t)ip1;
              esp_ip[1] = (uint8_t)ip2;
              esp_ip[2] = (uint8_t)ip3;
              esp_ip[3] = (uint8_t)ip4;
            }
          }
          
          /* 也匹配+CIFSR格式: STAIP,"192.168.1.100" */
          char *ip_pos2 = strstr((char*)esp_rx_buf, "STAIP,\"");
          if (ip_pos2 != NULL)
          {
            int ip1, ip2, ip3, ip4;
            if (sscanf(ip_pos2 + 7, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4) == 4)
            {
              esp_ip[0] = (uint8_t)ip1;
              esp_ip[1] = (uint8_t)ip2;
              esp_ip[2] = (uint8_t)ip3;
              esp_ip[3] = (uint8_t)ip4;
            }
          }
          
          /* 检测HTTP请求 +IPD,x,y:GET */
          char *ipd_pos = strstr((char*)esp_rx_buf, "+IPD,");
          if (ipd_pos != NULL && http_send_state == 0)
          {
            int link_id = 0;
            if (sscanf(ipd_pos, "+IPD,%d,", &link_id) == 1)
            {
              /* 步骤1: 发送CIPSEND指令 - 固定长度512，确保足够大 */
              char http_cmd[64];
              snprintf(http_cmd, sizeof(http_cmd), "AT+CIPSEND=%d,512\r\n", link_id);
              HAL_UART_Transmit(&huart1, (uint8_t*)http_cmd, strlen(http_cmd), 100);
              http_send_state = 1;  /* 等待> */
              http_send_time = HAL_GetTick();
              http_request_start = HAL_GetTick();
              http_pending_link = link_id;
            }
          }
        }
        esp_rx_len = 0;  /* 清空缓冲区 */
      }
    }
    
    /* HTTP发送状态机处理 */
    if (http_send_state == 2)
    {
      /* 状态2: HTTP响应已发送，延迟200ms后关闭连接（确保数据发送完成） */
      if (HAL_GetTick() - http_send_time > 200)
      {
        char close_cmd[32];
        snprintf(close_cmd, sizeof(close_cmd), "AT+CIPCLOSE=%d\r\n", http_pending_link);
        HAL_UART_Transmit(&huart1, (uint8_t*)close_cmd, strlen(close_cmd), 1000);
        
        http_send_state = 0;  /* 准备下一次请求 */
        esp_rx_len = 0;
        memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
      }
    }
    else if (http_send_state != 0 && (HAL_GetTick() - http_request_start > 3000))
    {
      /* 超时3秒强制关闭连接 */
      char close_cmd[32];
      snprintf(close_cmd, sizeof(close_cmd), "AT+CIPCLOSE=%d\r\n", http_pending_link);
      HAL_UART_Transmit(&huart1, (uint8_t*)close_cmd, strlen(close_cmd), 1000);
      
      http_send_state = 0;
      esp_rx_len = 0;
      memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
    }
    
    /* ========== 每30秒查询一次IP地址，查询结果更新到数码管 ========== */
    /* HTTP请求处理中跳过查询，避免AT命令冲突 */
    if (esp_connected && !esp_wifi_disconnected && !esp_waiting_response && http_send_state == 0)
    {
      if (HAL_GetTick() - esp_ip_last_query > 30000)  /* 30秒一次 */
      {
        uint8_t cmd[] = "AT+CIPSTA?\r\n";
        HAL_UART_Transmit(&huart1, cmd, sizeof(cmd)-1, 1000);
        esp_waiting_response = 1;
        esp_ip_last_query = HAL_GetTick();
      }
    }
    
      /* 每秒读取一次传感器数据并更新数码管显示 */
      if ((HAL_GetTick() - displayUpdateTick) >= 1000)
      {
        displayUpdateTick = HAL_GetTick();
        system_run_time++;  /* 系统运行时间秒数+1 */
        AHT20_Read(&aht20_data);
        BMP280_Read(&bmp280_data);
        
        /* 每秒切换一次IP显示段：0=1.xxx+2.xxx, 1=3.xxx+4.xxx */
        esp_ip_idx = (esp_ip_idx + 1) % 2;
      
      /* 倒计时模式下每秒-1 + 温控功能（配置模式下不计时） */
      if (ec11_countdown_mode && ec11_value > 0 && !temp_threshold_mode)
      {
        ec11_value--;
        
        /* 温控: 温度>配置的高温阈值关闭加热，温度<配置的低温阈值重新打开 */
        if (aht20_data.temperature > (float)temp_threshold_high)
        {
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);  /* 关闭电热丝 */
        }
        else if (aht20_data.temperature < (float)temp_threshold_low)
        {
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);  /* 打开电热丝 */
        }
        
        if (ec11_value == 0)
        {
          ec11_countdown_mode = 0;  /* 倒计时结束自动退出 */
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);  /* 关闭电热丝 */
          beep_mode = 1;  /* 启动循环蜂鸣 */
          beep_start_time = HAL_GetTick();  /* 记录蜂鸣开始时间 */
          beep_cycle_time = HAL_GetTick();
          beep_state = 1;
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);  /* 低电平触发 */
        }
      }
      
      /* 状态字符: '0'=正常, '1'=正在加热, '9'=ESP报错, 'd'=WiFi断开 */
      char status_char;
      if (esp_wifi_disconnected)
        status_char = 'd';  /* WiFi断开 */
      else if (esp_error)
        status_char = '9';  /* ESP报错 */
      else if (ec11_countdown_mode && ec11_value > 0 && HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_RESET)
        status_char = '1';  /* 正在加热 */
      else
        status_char = '0';  /* 正常状态 */
      
      /* 显示倒计时时间 HH.MM.SS */
      int countdown_hours = ec11_value / 3600;
      int countdown_mins = (ec11_value % 3600) / 60;
      int countdown_secs = ec11_value % 60;
      
      /* BMP280气压: 101325Pa = 101.3 kPa，取3位整数千帕 */
      int pres_kpa = (int)(bmp280_data.pressure / 1000);  /* Pa -> kPa */
      if (pres_kpa < 0) pres_kpa = 0;
      if (pres_kpa > 999) pres_kpa = 999;
      
      /* BMP280温度 */
      int bmp_temp_int = (int)bmp280_data.temperature;
      int bmp_temp_dec = (int)((bmp280_data.temperature - bmp_temp_int) * 10);
      if (bmp_temp_int < 0) bmp_temp_int = 0;
      if (bmp_temp_int > 99) bmp_temp_int = 99;
      
      /* AHT20温湿度 - 写入全局display2_buf */
      int temp_int = (int)aht20_data.temperature;
      int temp_dec = (int)((aht20_data.temperature - temp_int) * 10);
      int hum_int = (int)aht20_data.humidity;
      int hum_dec = (int)((aht20_data.humidity - hum_int) * 10);
      
      /* 范围限制 */
      if (temp_int < 0) temp_int = 0;
      if (temp_int > 999) temp_int = 999;
      if (hum_int < 0) hum_int = 0;
      if (hum_int > 999) hum_int = 999;
      
      /* 屏幕1: 状态码 + - + 倒计时HH.MM.SS 或 温度阈值配置 */
      if (temp_threshold_mode)
      {
        /* 温度阈值配置模式: C-HXX.LXX 显示高温和低温 */
        int low_temp = temp_threshold_edit - 10;
        if (low_temp < 20) low_temp = 20;
        sprintf(display1_buf, "C-H%02d.L%02d", temp_threshold_edit, low_temp);
        /* 配置模式下屏幕2保持正常显示 */
        sprintf(display2_buf, "T%02d.%1dH%02d.%1d", temp_int, temp_dec, hum_int, hum_dec);
      }
      else
      {
        /* 正常模式: 状态码 + 倒计时 */
        sprintf(display1_buf, "%c-%02d.%02d.%02d", status_char, countdown_hours, countdown_mins, countdown_secs);
      
        /* 第二个MAX7219轮播显示 */
        if (display_mode == 0)
        {
          /* 模式0: AHT20温度 + 湿度 */
          sprintf(display2_buf, "T%02d.%1dH%02d.%1d", temp_int, temp_dec, hum_int, hum_dec);
        }
        else if (display_mode == 1)
        {
          /* 模式1: BMP280温度 + 气压 - TXX.XPXXX 共8位正好 */
          sprintf(display2_buf, "T%02d.%1dP%03d", bmp_temp_int, bmp_temp_dec, pres_kpa);
        }
        else
        {
          /* 模式2: IP地址分两段显示 */
           if (esp_ip_idx == 0) {
               /* 组合1: -XXX.XXX + 空格 → 如 -192.168  */
               sprintf(display2_buf, "-%03d.%03d ", esp_ip[0], esp_ip[1]);
           } else {
               /* 组合2: 空格 + XXX.XXX- → 如  050.002- */
               sprintf(display2_buf, " %03d.%03d-", esp_ip[2], esp_ip[3]);
           }
        }
      }
      
      /* 每3秒切换轮播模式 */
      if (HAL_GetTick() - display_switch_time >= 3000)
      {
        display_switch_time = HAL_GetTick();
        display_mode = (display_mode + 1) % 3;  /* 0->1->2->0 循环 */
      }
    }
    
    /* ========== ESP8266状态机 - 命令发送逻辑（移到while循环顶层） ========== */
    if (esp_init_step <= 7)
    {
      /* busy延迟中，不发送命令 */
      if (HAL_GetTick() - esp_busy_delay < 800)
      {
      }
      /* 如果正在等待响应，不发送新命令 */
      else if (esp_waiting_response)
      {
        /* 超时8秒自动重置等待状态，防止死锁 */
        if (HAL_GetTick() - esp_init_time > 8000)
        {
          esp_waiting_response = 0;
          esp_error = 0;
        }
      }
      else
      {
        /* 空闲状态，可以发送命令 */
        if (esp_init_step == 0)
        {
          /* 步骤0: 先测试AT通信是否正常 */
          uint8_t cmd[] = "AT\r\n";
          HAL_UART_Transmit(&huart1, cmd, sizeof(cmd)-1, 1000);
          esp_waiting_response = 1;
          esp_init_time = HAL_GetTick();
          esp_init_step = 1;
        }
        else if (esp_init_step == 1)
        {
          /* 步骤1: 复位ESP8266 */
          uint8_t cmd[] = "AT+RST\r\n";
          HAL_UART_Transmit(&huart1, cmd, sizeof(cmd)-1, 1000);
          esp_waiting_response = 1;
          esp_init_time = HAL_GetTick();
          esp_init_step = 2;
        }
        else if (esp_init_step == 2)
        {
          /* 步骤2: 等待复位完成，延迟2.5秒后发送下一条命令 */
          if (HAL_GetTick() - esp_step_delay_time > 2500)
          {
            esp_init_step = 3;
          }
        }
        else if (esp_init_step == 3)
        {
          /* 步骤3: 设置为Station模式 */
          uint8_t cmd[] = "AT+CWMODE=1\r\n";
          HAL_UART_Transmit(&huart1, cmd, sizeof(cmd)-1, 1000);
          esp_waiting_response = 1;
          esp_init_time = HAL_GetTick();
          esp_init_step = 4;
        }
        else if (esp_init_step == 4)
        {
          /* 步骤4: 延迟500ms后连接WiFi */
          if (HAL_GetTick() - esp_step_delay_time > 500)
          {
            uint8_t cmd[] = "AT+CWJAP=\"Plasma\",\"3.1415926535898\"\r\n";
            HAL_UART_Transmit(&huart1, cmd, sizeof(cmd)-1, 15000);
            esp_waiting_response = 1;
            esp_init_time = HAL_GetTick();
            esp_init_step = 5;
          }
        }
        else if (esp_init_step == 5)
        {
          /* 步骤5: 等待WIFI GOT IP，然后查询IP确认 */
          if (esp_connected && !esp_wifi_disconnected)
          {
            /* 已有IP，直接进入下一步 */
            if (esp_ip[0] != 0 || esp_ip[1] != 0 || esp_ip[2] != 0 || esp_ip[3] != 0)
            {
              esp_init_step = 6;
              esp_step_delay_time = HAL_GetTick();
            }
            /* 没有IP，每隔2秒查询一次 */
            else if (HAL_GetTick() - esp_ip_last_query > 2000)
            {
              uint8_t cmd[] = "AT+CIPSTA?\r\n";
              HAL_UART_Transmit(&huart1, cmd, sizeof(cmd)-1, 1000);
              esp_waiting_response = 1;
              esp_ip_last_query = HAL_GetTick();
            }
          }
        }
        else if (esp_init_step == 6)
        {
          /* 步骤6: 开启多连接模式（延迟500ms确保WiFi稳定） */
          if (HAL_GetTick() - esp_step_delay_time > 500)
          {
            uint8_t cmd[] = "AT+CIPMUX=1\r\n";
            HAL_UART_Transmit(&huart1, cmd, sizeof(cmd)-1, 1000);
            esp_waiting_response = 1;
            esp_init_time = HAL_GetTick();
            esp_init_step = 7;
          }
        }
        else if (esp_init_step == 7)
        {
          /* 步骤7: 开启TCP服务器，端口80 */
          if (HAL_GetTick() - esp_step_delay_time > 500)
          {
            uint8_t cmd[] = "AT+CIPSERVER=1,80\r\n";
            HAL_UART_Transmit(&huart1, cmd, sizeof(cmd)-1, 1000);
            esp_waiting_response = 1;
            esp_init_time = HAL_GetTick();
            esp_init_step = 8;
          }
        }
      }
    }
    
    /* ========== 显示刷新独立进行，每50ms刷新一次，防止阻塞导致丢显示 ========== */
    if ((HAL_GetTick() - display_refresh_tick) >= 50)
    {
      display_refresh_tick = HAL_GetTick();
      
      /* 检测PA8继电器状态变化：EMI干扰后立即重配MAX7219寄存器 */
      uint8_t pa8_now = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8);
      if (pa8_now != pa8_prev)
      {
        pa8_prev = pa8_now;
        /* 继电器状态变化后立即重配MAX7219，消除EMI导致的88888888故障 */
        MAX7219_WriteReg_Dual(MAX7219_DISPLAYTEST, 0x00, MAX7219_DISPLAYTEST, 0x00);
        MAX7219_WriteReg_Dual(MAX7219_SHUTDOWN, 0x01, MAX7219_SHUTDOWN, 0x01);
        MAX7219_WriteReg_Dual(MAX7219_SCANLIMIT, 0x07, MAX7219_SCANLIMIT, 0x07);
        MAX7219_WriteReg_Dual(MAX7219_DECODEMODE, 0x00, MAX7219_DECODEMODE, 0x00);
        MAX7219_WriteReg_Dual(MAX7219_INTENSITY, 0x01, MAX7219_INTENSITY, 0x01);
      }
      
      MAX7219_DisplayStr_Dual(display1_buf, display2_buf);
    }
    
    /* PC13 LED 0.5s闪烁 */
    if ((HAL_GetTick() - ledTickMs) >= 250)
    {
      ledTickMs = HAL_GetTick();
      ledState = !ledState;
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, (GPIO_PinState)ledState);
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level: PA8高电平关闭电热丝 */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level: PB9高电平关闭蜂鸣器 */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);  /* MAX7219 CS */

  /*Configure GPIO pins : PB12 PB13 PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */