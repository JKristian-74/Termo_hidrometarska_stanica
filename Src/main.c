/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dht22.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
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

/* USER CODE BEGIN PV */
volatile uint8_t measure_flag  = 0;
volatile uint8_t set_mode      = 0;
volatile uint8_t reset_minmax  = 0;

volatile float temperature = 0.0f;
volatile float humidity    = 0.0f;
float threshold = 28.0f;
float min_temp  = 100.0f;
float max_temp  = -100.0f;

uint16_t adc_value = 0;
char     msg[120];
extern UART_HandleTypeDef huart4;

volatile uint8_t bt_rx_byte;
volatile char    bt_cmd[32];
volatile uint8_t bt_cmd_ready = 0;
static   char    bt_line[32];
static   uint8_t bt_idx = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Read_ADC(void);
static void Update_Threshold(void);
static void LED_Control(void);
static void Update_MinMax(void);
static void Send_Data(void);
static void OLED_Update(void);
static void Process_BT_Command(void);
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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start(&htim3);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 10);
  ssd1306_WriteString("Termo-Higro", Font_7x10, White);
  ssd1306_SetCursor(10, 30);
  ssd1306_WriteString("Stanica v1.0", Font_7x10, White);
  ssd1306_UpdateScreen();
  HAL_Delay(1500);

  DHT22_Init(&htim3, GPIOA, GPIO_PIN_1);

  HAL_UART_Receive_IT(&huart4, (uint8_t*)&bt_rx_byte, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* USER CODE END WHILE */

      /* USER CODE BEGIN 3 */
      if (set_mode)
      {
          Read_ADC();
          Update_Threshold();
          OLED_Update();
      }

      if (reset_minmax)
      {
          reset_minmax = 0;
          min_temp =  100.0f;
          max_temp = -100.0f;
      }

      Process_BT_Command();

      if (measure_flag)
      {
          measure_flag = 0;

          HAL_StatusTypeDef rez = DHT22_Read((float*)&temperature, (float*)&humidity);

          if (rez == HAL_OK)
          {
              Update_MinMax();
              LED_Control();
              Send_Data();

              if (!set_mode)
              {
                  OLED_Update();
              }
          }
      }

      HAL_Delay(100);
  }
  /* USER CODE END 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void Read_ADC(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    adc_value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
}

static void Update_Threshold(void)
{
    threshold = 20.0f + (adc_value / 4095.0f) * 15.0f;
}

static void LED_Control(void)
{
    if (temperature > threshold)
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}

static void Update_MinMax(void)
{
    if (temperature < min_temp) min_temp = temperature;
    if (temperature > max_temp) max_temp = temperature;
}

static void Send_Data(void)
{
    snprintf(msg, sizeof(msg),
             "T:%.1f;H:%.1f;P:%.1f;M:%u;MN:%.1f;MX:%.1f\r\n",
             temperature, humidity, threshold,
             (unsigned int)set_mode,
             min_temp, max_temp);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 200); // PuTTY
    HAL_UART_Transmit(&huart4, (uint8_t*)msg, strlen(msg), 200); // HC-05
}

static void OLED_Update(void)
{
    char buf[32];
    ssd1306_Fill(Black);

    if (set_mode)
    {
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString("** SET MODE **", Font_11x18, White);

        snprintf(buf, sizeof(buf), "Prag: %.1f C", threshold);
        ssd1306_SetCursor(0, 20);
        ssd1306_WriteString(buf, Font_11x18, White);
    }
    else
    {
        snprintf(buf, sizeof(buf), "T: %.1f C", temperature);
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString(buf, Font_11x18, White);

        snprintf(buf, sizeof(buf), "H: %.1f %%", humidity);
        ssd1306_SetCursor(0, 22);
        ssd1306_WriteString(buf, Font_11x18, White);

        snprintf(buf, sizeof(buf), "P:%.1f C", threshold);
        ssd1306_SetCursor(0, 44);
        ssd1306_WriteString(buf, Font_11x18, White);
    }

    ssd1306_UpdateScreen();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART4)
    {
        char c = (char)bt_rx_byte;

        if (c == '\n' || c == '\r')
        {
            if (bt_idx > 0 && !bt_cmd_ready)
            {
                bt_line[bt_idx] = '\0';
                strcpy((char*)bt_cmd, bt_line);
                bt_cmd_ready = 1;
            }
            bt_idx = 0;
        }
        else if (bt_idx < sizeof(bt_line) - 1)
        {
            bt_line[bt_idx++] = c;
        }

        HAL_UART_Receive_IT(&huart4, (uint8_t*)&bt_rx_byte, 1);
    }
}

static void Process_BT_Command(void)
{
    if (!bt_cmd_ready) return;

    if (strcmp((char*)bt_cmd, "R") == 0 ||
        strcmp((char*)bt_cmd, "RESET") == 0)         /* reset min/max */
    {
        reset_minmax = 1;
        HAL_UART_Transmit(&huart4, (uint8_t*)"OK:RESET\r\n", 10, 100);
    }
    else if (strncmp((char*)bt_cmd, "P:", 2) == 0)   /* novi prag, npr. "P:30.5" */
    {
        float p = atof((char*)bt_cmd + 2);
        if (p >= 20.0f && p <= 35.0f)
        {
            threshold = p;
            LED_Control();
            if (!set_mode) OLED_Update();
            HAL_UART_Transmit(&huart4, (uint8_t*)"OK:PRAG\r\n", 9, 100);
        }
    }
    else if (strcmp((char*)bt_cmd, "S") == 0)        /* mjerenje odmah */
    {
        measure_flag = 1;
    }

    bt_cmd_ready = 0;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
        measure_flag = 1;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last1 = 0;
    static uint32_t last2 = 0;

    uint32_t now = HAL_GetTick();

    if (GPIO_Pin == GPIO_PIN_5)
    {
        if (now - last1 > 2000 && !measure_flag)
        {
            measure_flag = 1;
            last1 = now;
        }
    }

    if (GPIO_Pin == GPIO_PIN_12)
    {
        if (now - last2 > 500)
        {
            set_mode = !set_mode;
            last2 = now;
        }
    }
}
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

#ifdef  USE_FULL_ASSERT
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
