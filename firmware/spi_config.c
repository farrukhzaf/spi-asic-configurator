/**
  ******************************************************************************
  * @file    spi_config.c
  * @brief   SPI1 and GPIO hardware initialization for ASIC interface
  *          on STM32 Nucleo-G474RE
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "spi_config.h"

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize SPI1 and GPIO for ASIC communication
  *
  *         SPI1 configuration:
  *           - Master mode
  *           - Mode 0 (CPOL=0, CPHA=0)
  *           - 8-bit data size (we send 2 bytes per transaction)
  *           - MSB first
  *           - Software NSS management (CS controlled via GPIO)
  *           - Full-duplex
  *
  *         GPIO configuration:
  *           - PA5 (SCK),  PA6 (MISO), PA7 (MOSI) -> AF5 (SPI1)
  *           - PB6 (CS)   -> Output push-pull, default HIGH
  */
HAL_StatusTypeDef SPI_Config_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  HAL_StatusTypeDef status;

  /* ---- Enable peripheral clocks ---- */
  __HAL_RCC_SPI1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* ---- Configure SPI1 GPIO pins (PA5, PA6, PA7) ---- */
  GPIO_InitStruct.Pin       = ASIC_SCK_PIN | ASIC_MISO_PIN | ASIC_MOSI_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* ---- Configure CS pin (PB6) as GPIO output push-pull ---- */
  GPIO_InitStruct.Pin   = ASIC_CS_PIN;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ASIC_CS_PORT, &GPIO_InitStruct);

  /* CS de-asserted (HIGH) by default */
  HAL_GPIO_WritePin(ASIC_CS_PORT, ASIC_CS_PIN, GPIO_PIN_SET);

  /* ---- Configure SPI1 peripheral ---- */
  hspi1.Instance               = ASIC_SPI_INSTANCE;
  hspi1.Init.Mode              = SPI_MODE_MASTER;
  hspi1.Init.Direction         = SPI_DIRECTION_2LINES;        /* Full-duplex      */
  hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;           /* 8-bit transfers  */
  hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;            /* CPOL = 0         */
  hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;             /* CPHA = 0         */
  hspi1.Init.NSS               = SPI_NSS_SOFT;                /* Software CS      */
  hspi1.Init.BaudRatePrescaler = ASIC_SPI_PRESCALER;
  hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;            /* MSB first        */
  hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;

  status = HAL_SPI_Init(&hspi1);

  return status;
}