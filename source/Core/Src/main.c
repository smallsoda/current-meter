/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "usbd_cdc_if.h"

#include "eeprom.h"
#include "status_led.h" // TODO

#define K_ADC_TO_UA 805.860806 // k = 1000000000 / 4095 * 3.3
#define OA_MAX_ADC_VALUE 2000
#define OA0_GAIN 1
#define OA1_GAIN 40
#define OA2_GAIN 391

#define DATA_BUFFER_SIZE 256
#define DATA_STRING_SIZE 1024

#define DATA_TX_MAX_RECORDS 50
#define DATA_TX_MAX_RETRIES 10

#define BUTTON_DELAY     200 // ms
#define BUTTON_DELAY_LP 1000 // ms

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
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */

struct eeprom eeprom;
settings_t settings;

uint8_t save_settings_flag = 0;

// ADC
uint16_t adc_data[3];

// Data buffer
uint16_t data_buffer_raw[DATA_BUFFER_SIZE][3];
uint32_t data_buffer[DATA_BUFFER_SIZE];
uint16_t data_buffer_set = 0;
uint16_t data_buffer_get = 0;
char data_string[DATA_STRING_SIZE];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//
//
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	// Button
	static uint16_t btn_counter = 0;
	static uint16_t btn_counter_press = 0;
	static uint8_t btn_state = 1;

	// General counter
	static uint16_t counter = 0;
	counter = (counter + 1) % 1000;

	// Period: 1ms
	// ADC
	//
	HAL_ADC_Start_DMA(&hadc1, (uint32_t *) adc_data, 3);

	// Period: 1ms
	// Button
	//
	if(btn_counter_press)
		btn_counter_press--;
	//
	if(btn_counter)
	{
		btn_counter--;
	}
	else
	{
		if((btn_state == 1) && (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET))
		{
			btn_state = 0;
			btn_counter = BUTTON_DELAY;
			btn_counter_press = BUTTON_DELAY_LP;
		}
		else if((btn_state == 0) && (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_SET))
		{
			btn_state = 1;
			btn_counter = BUTTON_DELAY;

			// Short press
			if(btn_counter_press)
			{

				settings.period *= 10;
				if (settings.period > 100)
					settings.period = 1;
				status_led(0, settings.period == 1 ? 1 :
						(settings.period == 10 ? 2 : 3));
				save_settings_flag = 1;
			}
			// Long press
			else
			{
				;
			}
		}
	}

	// Period: 125ms
	// LEDs
	//
	if((counter % 125) == 0)
		status_led_callback();
}

//
//
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	static uint16_t counter = 0;
	uint32_t value = 0;

	// Raw data
	data_buffer_raw[data_buffer_set][0] = adc_data[1];
	data_buffer_raw[data_buffer_set][1] = adc_data[2];
	data_buffer_raw[data_buffer_set][2] = adc_data[0];

	// OA2
	if(adc_data[0] < OA_MAX_ADC_VALUE)
		value = adc_data[0] * K_ADC_TO_UA / OA2_GAIN * settings.oa2 / 1000;
	// OA1
	else if(adc_data[2] < OA_MAX_ADC_VALUE)
		value = adc_data[2] * K_ADC_TO_UA / OA1_GAIN * settings.oa1 / 1000;
	// OA0
	else
		value = adc_data[1] * K_ADC_TO_UA / OA0_GAIN * settings.oa0 / 1000;

	// Clear buffer
	if(counter == 0)
		data_buffer[data_buffer_set] = 0;

	// Period: 1ms
	if(settings.period == 1)
	{
		data_buffer[data_buffer_set] = value;
		counter = 0; // ?
		data_buffer_set = (data_buffer_set + 1) % DATA_BUFFER_SIZE;
	}
	// Period: 10ms
	else if(settings.period == 10)
	{
		data_buffer[data_buffer_set] += value;
		counter = (counter + 1) % 10;
		if(counter == 0)
		{
			data_buffer[data_buffer_set] /= 10;
			data_buffer_set = (data_buffer_set + 1) % DATA_BUFFER_SIZE;
		}
	}
	// Period: 100ms
	else if(settings.period == 100)
	{
		data_buffer[data_buffer_set] += value;
		counter = (counter + 1) % 100;
		if(counter == 0)
		{
			data_buffer[data_buffer_set] /= 100;
			data_buffer_set = (data_buffer_set + 1) % DATA_BUFFER_SIZE;
		}
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  int ret;

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
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USB_DEVICE_Init();
  MX_ADC1_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  ret = eeprom_init(&eeprom, &hi2c1, 0xA0, 0x00);
  if (ret)
	  for (;;); // TODO: Notify user

  ret = eeprom_get(&eeprom, &settings);
  if (ret)
	  for (;;); // TODO: Notify user

  // LEDs
  // 1ms, 10ms, 100ms
  status_led(0, settings.period == 1 ? 1 : (settings.period == 10 ? 2 : 3));
  status_led(1, 0);

  // ADC
  HAL_ADCEx_Calibration_Start(&hadc1);

  // TIM4
  HAL_TIM_Base_Start_IT(&htim4);

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	    // Saving settings to storage
	    if(save_settings_flag)
	    {
	    	save_settings_flag = 0;
	    	ret = eeprom_set(&eeprom, &settings);
	    	if (ret)
	    	{
	    		; // TODO
	    	}
	    }

		// Data transfer
	    if(data_buffer_get != data_buffer_set)
	    {
	    	// Create message
			uint16_t string_cnt = 0;
			uint16_t records = DATA_TX_MAX_RECORDS;

			while((data_buffer_get != data_buffer_set) && (records))
			{
				string_cnt += sprintf(&data_string[string_cnt], "%lu\r\n", data_buffer[data_buffer_get]);
				data_buffer_get = (data_buffer_get + 1) % DATA_BUFFER_SIZE;
				records--;
			}

			// Transfer message
			uint16_t retries = DATA_TX_MAX_RETRIES;
			while((CDC_Transmit_FS((uint8_t*) data_string, string_cnt) == USBD_BUSY) && (retries))
			{
				HAL_Delay(1);
				retries--;
			}
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
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 3;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 719;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 99;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB13 PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
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
