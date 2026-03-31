/**
  ******************************************************************************
  * @file    asic_spi.h
  * @brief   ASIC SPI communication driver header
  *          Frame format: {R/~W (1 bit), Address[6:0] (7 bits), Data[7:0] (8 bits)}
  *          16-bit frame, MSB first, SPI Mode 0 (CPOL=0, CPHA=0)
  ******************************************************************************
  */

#ifndef ASIC_SPI_H
#define ASIC_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Exported defines ----------------------------------------------------------*/

/* Frame bit positions */
#define ASIC_RW_BIT_POS         15U      /* Bit 15: R/~W  (1=Read, 0=Write) */
#define ASIC_ADDR_BIT_POS       8U       /* Bits 14:8: Address[6:0]         */
#define ASIC_DATA_BIT_POS       0U       /* Bits 7:0:  Data[7:0]            */

/* Frame masks */
#define ASIC_RW_MASK            (0x01U << ASIC_RW_BIT_POS)   /* 0x8000 */
#define ASIC_ADDR_MASK          (0x7FU << ASIC_ADDR_BIT_POS)  /* 0x7F00 */
#define ASIC_DATA_MASK          (0xFFU << ASIC_DATA_BIT_POS)  /* 0x00FF */

/* R/W bit values */
#define ASIC_READ               0U
#define ASIC_WRITE              1U

/* SPI timeout in milliseconds */
#define ASIC_SPI_TIMEOUT        100U

/*
 * --------------------------------------------------------------------------
 *  ASIC Register Map (example — update to match your ASIC datasheet)
 * --------------------------------------------------------------------------
 *  Address range: 0x00 to 0x7F (7-bit addressing = 128 registers max)
 */
#define ASIC_REG_CHIP_ID        0x00U
#define ASIC_REG_STATUS         0x01U
#define ASIC_REG_CTRL           0x02U
#define ASIC_REG_CONFIG         0x03U
#define ASIC_REG_FIFO_THRESH    0x04U    /* <-- add this line */
#define ASIC_REG_DATA0          0x10U
#define ASIC_REG_DATA1          0x11U
/* Add more registers as needed for your ASIC */

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  ASIC driver status codes
  */
typedef enum
{
  ASIC_OK       = 0x00U,
  ASIC_ERROR    = 0x01U,
  ASIC_BUSY     = 0x02U,
  ASIC_TIMEOUT  = 0x03U
} ASIC_StatusTypeDef;

/**
  * @brief  ASIC driver handle structure
  */
typedef struct
{
  SPI_HandleTypeDef   *hspi;          /* Pointer to HAL SPI handle        */
  GPIO_TypeDef        *cs_port;       /* CS pin GPIO port                 */
  uint16_t             cs_pin;        /* CS pin number                    */
} ASIC_HandleTypeDef;

/* Exported function prototypes ----------------------------------------------*/

/**
  * @brief  Initialize the ASIC driver handle (does NOT init SPI/GPIO hardware)
  * @param  hasic   Pointer to ASIC handle
  * @param  hspi    Pointer to an already-initialized HAL SPI handle
  * @param  cs_port GPIO port for chip select (e.g., GPIOB)
  * @param  cs_pin  GPIO pin for chip select  (e.g., GPIO_PIN_6)
  * @retval ASIC_StatusTypeDef
  */
ASIC_StatusTypeDef ASIC_Init(ASIC_HandleTypeDef *hasic,
                             SPI_HandleTypeDef  *hspi,
                             GPIO_TypeDef       *cs_port,
                             uint16_t            cs_pin);

/**
  * @brief  Write one byte to an ASIC register
  * @param  hasic   Pointer to ASIC handle
  * @param  reg     7-bit register address (0x00–0x7F)
  * @param  data    Byte to write
  * @retval ASIC_StatusTypeDef
  */
ASIC_StatusTypeDef ASIC_WriteReg(ASIC_HandleTypeDef *hasic,
                                 uint8_t reg,
                                 uint8_t data);

/**
  * @brief  Read one byte from an ASIC register
  * @param  hasic   Pointer to ASIC handle
  * @param  reg     7-bit register address (0x00–0x7F)
  * @param  pData   Pointer to store the read byte
  * @retval ASIC_StatusTypeDef
  */
ASIC_StatusTypeDef ASIC_ReadReg(ASIC_HandleTypeDef *hasic,
                                uint8_t  reg,
                                uint8_t *pData);

/**
  * @brief  Write multiple consecutive registers (burst write)
  * @param  hasic   Pointer to ASIC handle
  * @param  reg     Starting 7-bit register address
  * @param  pData   Pointer to data buffer
  * @param  len     Number of bytes to write
  * @retval ASIC_StatusTypeDef
  */
ASIC_StatusTypeDef ASIC_WriteBurst(ASIC_HandleTypeDef *hasic,
                                   uint8_t  reg,
                                   uint8_t *pData,
                                   uint16_t len);

/**
  * @brief  Read multiple consecutive registers (burst read)
  * @param  hasic   Pointer to ASIC handle
  * @param  reg     Starting 7-bit register address
  * @param  pData   Pointer to receive buffer
  * @param  len     Number of bytes to read
  * @retval ASIC_StatusTypeDef
  */
ASIC_StatusTypeDef ASIC_ReadBurst(ASIC_HandleTypeDef *hasic,
                                  uint8_t  reg,
                                  uint8_t *pData,
                                  uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* ASIC_SPI_H */