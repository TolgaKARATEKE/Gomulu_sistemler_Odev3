#include "main.h"
#include "tim.h"
#include "gpio.h"

#define FLASH_USER_START_ADDR   0x0800FC00

uint32_t blink_count = 4;
uint32_t current_blink = 0;
uint8_t is_waiting = 0;
uint32_t wait_seconds = 0;
uint8_t led_state = 0;

uint32_t button_press_time = 0;
uint8_t button_pressed = 0;

void SystemClock_Config(void);
void Save_Blink_Count_To_Flash(uint32_t count);
uint32_t Read_Blink_Count_From_Flash(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM2_Init();

  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
      HAL_Delay(3000);
      blink_count = 4;
      Save_Blink_Count_To_Flash(blink_count);
  } else {
      uint32_t flash_val = Read_Blink_Count_From_Flash();
      if (flash_val == 0xFFFFFFFF || flash_val > 7 || flash_val < 4) {
          blink_count = 4;
          Save_Blink_Count_To_Flash(blink_count);
      } else {
          blink_count = flash_val;
      }
  }

  HAL_TIM_Base_Start_IT(&htim2);

  while (1)
  {
      if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
          if (!button_pressed) {
              button_pressed = 1;
              button_press_time = HAL_GetTick();
          }
      } else {
          if (button_pressed) {
              uint32_t press_duration = HAL_GetTick() - button_press_time;

              if (press_duration > 50) {
                  blink_count++;
                  if (blink_count > 7) {
                      blink_count = 4;
                  }
                  Save_Blink_Count_To_Flash(blink_count);
              }
              button_pressed = 0;
          }
      }
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        if (is_waiting) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            wait_seconds++;
            if (wait_seconds >= 5) {
                is_waiting = 0;
                wait_seconds = 0;
                current_blink = 0;
                led_state = 0;
            }
        } else {
            if (led_state == 0) {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
                led_state = 1;
            } else {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
                led_state = 0;
                current_blink++;
                if (current_blink >= blink_count) {
                    is_waiting = 1;
                }
            }
        }
    }
}

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

uint32_t Read_Blink_Count_From_Flash(void) {
    return *(__IO uint32_t *)FLASH_USER_START_ADDR;
}