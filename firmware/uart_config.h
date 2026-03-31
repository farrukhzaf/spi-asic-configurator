/**
  ******************************************************************************
  * @file    uart_config.h
  * @brief   LPUART1 configuration for Virtual COM Port (VCP) on Nucleo-G474RE
  *
  *          Default board config (solder bridges SB17, SB23 ON):
  *            PA2 -> LPUART1_TX -> STLINK VCP TX
  *            PA3 -> LPUART1_RX -> STLINK VCP RX
  *
  *          Open a serial terminal on your PC at 115200 baud, 8N1.
  ******************************************************************************
  */

#ifndef UART_CONFIG_H
#define UART_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"
#include <stdio.h>

extern UART_HandleTypeDef hlpuart1;

/**
  * @brief  Initialize LPUART1 for VCP communication (115200, 8N1)
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef UART_Config_Init(void);

/**
  * @brief  Send a string over LPUART1
  * @param  str  Null-terminated string
  */
void UART_Print(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* UART_CONFIG_H */