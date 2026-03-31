/**
  ******************************************************************************
  * @file    uart_config.c
  * @brief   LPUART1 initialization and print function for VCP
  *          PA2 = TX, PA3 = RX, 115200 baud, 8N1
  ******************************************************************************
  */

#include "uart_config.h"
#include <string.h>

UART_HandleTypeDef hlpuart1;

HAL_StatusTypeDef UART_Config_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Enable clocks */
  __HAL_RCC_LPUART1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
	
	/* Use HSI (16 MHz) as LPUART1 clock — gives clean baud rate division */
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
	PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_HSI;
	HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

  /* PA2 = LPUART1_TX, PA3 = LPUART1_RX */
  GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_LPUART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* LPUART1 config: 115200 baud, 8N1 */
  hlpuart1.Instance               = LPUART1;
  hlpuart1.Init.BaudRate           = 115200;
  hlpuart1.Init.WordLength         = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits           = UART_STOPBITS_1;
  hlpuart1.Init.Parity             = UART_PARITY_NONE;
  hlpuart1.Init.Mode               = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl          = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling      = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler      = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  return HAL_UART_Init(&hlpuart1);
}

void UART_Print(const char *str)
{
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)str, (uint16_t)strlen(str), 1000);
}