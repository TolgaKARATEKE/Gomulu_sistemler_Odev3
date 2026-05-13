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
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// F103C8T6 icin son sayfalardan birinin adresi (Flash islemleri icin)
#define FLASH_USER_START_ADDR   0x0800FC00
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t blink_count = 4;
uint32_t current_blink = 0;
uint8_t is_waiting = 0;
uint32_t wait_seconds = 0;
uint8_t led_state = 0; 

uint32_t button_press_time = 0;
uint8_t button_pressed = 0;
uint8_t long_press_handled = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Save_Blink_Count_To_Flash(uint32_t count);
uint32_t Read_Blink_Count_From_Flash(void);
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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  
  // Flash memory'den elde ettiginiz deger 7'den buyukse (veya bos olan 0xFFFFFFFF ise) [cite: 48]
  uint32_t flash_val = Read_Blink_Count_From_Flash();

  if (flash_val == 0xFFFFFFFF || flash_val > 7) {
      blink_count = 4; // degerini 4 yapin [cite: 48]
      Save_Blink_Count_To_Flash(blink_count);
  } else {
      blink_count = flash_val;
  }

  // TIM2'yi kesme (interrupt) modunda baslat [cite: 36]
  HAL_TIM_Base_Start_IT(&htim2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // Butona basildiysa (A0 pini Lojik 0)
      if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
          if (!button_pressed) {
              button_pressed = 1;
              button_press_time = HAL_GetTick();
              long_press_handled = 0;
          } else {
              // Buton en az 3 saniye basili tutulursa sistem fabrika ayarlarina donsun [cite: 50]
              if (!long_press_handled && (HAL_GetTick() - button_press_time) >= 3000) {
                  blink_count = 4; // blink_count degeri 4 olsun [cite: 51]
                  Save_Blink_Count_To_Flash(blink_count);
                  long_press_handled = 1; 
              }
          }
      } else {
          // Buton birakildiysa
          if (button_pressed) {
              uint32_t press_duration = HAL_GetTick() - button_press_time;
              
              // 50ms debouncing ile kisa basim algilamasi
              if (press_duration > 50 && press_duration < 3000 && !long_press_handled) {
                  blink_count++; // butona her basildiginda blink count degeri 1 artsin [cite: 43]
                  if (blink_count > 7) {
                      blink_count = 4; // degeri 7 iken butona basilirsa blink count degeri 4 olsun [cite: 44]
                  }
                  Save_Blink_Count_To_Flash(blink_count);
              }
              button_pressed = 0;
          }
      }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// TIM2 Kesme Fonksiyonu
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        if (is_waiting) {
            // 5sn. sonuk dursun [cite: 40]
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); 
            wait_seconds++;
            if (wait_seconds >= 5) {
                is_waiting = 0;
                wait_seconds = 0;
                current_blink = 0;
                led_state = 0;
            }
        } else {
            // LED, blink_count degiskeninin degeri kadar 1'er saniye yanip sonsun [cite: 40]
            if (led_state == 0) {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); 
                led_state = 1;
            } else {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); 
                led_state = 0;
                current_blink++; 
                
                if (current_blink >= blink_count) {
                    is_waiting = 1;
                    wait_seconds = 0;
                }
            }
        }
    }
}

// Flash Yazma
void Save_Blink_Count_To_Flash(uint32_t count) {
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_USER_START_ADDR;
    EraseInitStruct.NbPages = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) == HAL_OK) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_USER_START_ADDR, count);
    }
    HAL_FLASH_Lock();
}

// Flash Okuma
uint32_t Read_Blink_Count_From_Flash(void) {
    return *(__IO uint32_t *)FLASH_USER_START_ADDR;
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