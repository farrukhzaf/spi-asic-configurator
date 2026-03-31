/**
  ******************************************************************************
  * @file    spi_config.h
  * @brief   SPI1 and GPIO hardware configuration for ASIC interface
  *          Nucleo-G474RE pin mapping:
  *            PA5 -> SPI1_SCK   (Arduino D13 / CN5 pin 6)
  *            PA6 -> SPI1_MISO  (Arduino D12 / CN5 pin 5)
  *            PA7 -> SPI1_MOSI  (Arduino D11 / CN5 pin 4)
  *            PB6 -> CS_bar     (Arduino D10 / CN5 pin 3) — GPIO software CS
  ******************************************************************************
  */

#ifndef SPI_CONFIG_H
#define SPI_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Exported defines ----------------------------------------------------------*/

/* SPI1 pins (directly from Nucleo-G474RE user manual UM2505) */
#define ASIC_SPI_INSTANCE       SPI1

#define ASIC_SCK_PORT           GPIOA
#define ASIC_SCK_PIN            GPIO_PIN_5       /* PA5 */
#define ASIC_SCK_AF             GPIO_AF5_SPI1

#define ASIC_MISO_PORT          GPIOA
#define ASIC_MISO_PIN           GPIO_PIN_6       /* PA6 */
#define ASIC_MISO_AF            GPIO_AF5_SPI1

#define ASIC_MOSI_PORT          GPIOA
#define ASIC_MOSI_PIN           GPIO_PIN_7       /* PA7 */
#define ASIC_MOSI_AF            GPIO_AF5_SPI1

#define ASIC_CS_PORT            GPIOB
#define ASIC_CS_PIN             GPIO_PIN_6       /* PB6 */

/*
 * SPI clock prescaler selection.
 * STM32G474 runs SPI1 from APB2 clock (up to 170 MHz).
 *
 * Prescaler    SPI Clock (@ 170 MHz APB2)
 * ----------------------------------------
 *   2           85.00 MHz
 *   4           42.50 MHz
 *   8           21.25 MHz
 *  16           10.63 MHz
 *  32            5.31 MHz
 *  64            2.66 MHz
 * 128            1.33 MHz
 * 256          664.06 kHz
 *
 * Start with a conservative prescaler (e.g. 256 or 128) for bring-up,
 * then decrease once communication is verified.
 */
#define ASIC_SPI_PRESCALER      SPI_BAUDRATEPRESCALER_32

/* Exported variables --------------------------------------------------------*/
extern SPI_HandleTypeDef hspi1;

/* Exported function prototypes ----------------------------------------------*/

/**
  * @brief  Initialize SPI1 peripheral and associated GPIO pins
  * @note   Configures SPI1 as master, Mode 0, 8-bit, MSB first, software CS.
  *         Also configures PB6 as output push-pull for chip select.
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef SPI_Config_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* SPI_CONFIG_H */