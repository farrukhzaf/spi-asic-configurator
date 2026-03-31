/**
  ******************************************************************************
  * @file    asic_spi.c
  * @brief   ASIC SPI communication driver implementation
  *
  *          Frame format: {R/~W (1 bit), Address[6:0] (7 bits), Data[7:0] (8 bits)}
  *          Total: 16 bits per transaction, transmitted MSB first.
  *
  *          SPI Mode 0 (CPOL=0, CPHA=0)
  *          CS_bar held LOW for the entire 16-bit transfer.
  *
  *          WRITE: Master sends {0, Addr[6:0], Data[7:0]} on MOSI.
  *                 MISO data from slave is ignored.
  *
  *          READ:  Master sends {1, Addr[6:0], 0x00} on MOSI.
  *                 Slave returns data on MISO during the Data[7:0] phase.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "asic_spi.h"

/* Private function prototypes -----------------------------------------------*/
static void ASIC_CS_Low(ASIC_HandleTypeDef *hasic);
static void ASIC_CS_High(ASIC_HandleTypeDef *hasic);
static uint16_t ASIC_BuildFrame(uint8_t rw, uint8_t reg, uint8_t data);
static ASIC_StatusTypeDef ASIC_TransmitReceive16(ASIC_HandleTypeDef *hasic,
                                                  uint16_t tx_frame,
                                                  uint16_t *rx_frame);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Assert chip select (drive LOW)
  */
static void ASIC_CS_Low(ASIC_HandleTypeDef *hasic)
{
  HAL_GPIO_WritePin(hasic->cs_port, hasic->cs_pin, GPIO_PIN_RESET);
}

/**
  * @brief  De-assert chip select (drive HIGH)
  */
static void ASIC_CS_High(ASIC_HandleTypeDef *hasic)
{
  HAL_GPIO_WritePin(hasic->cs_port, hasic->cs_pin, GPIO_PIN_SET);
}

/**
  * @brief  Build a 16-bit SPI frame
  * @param  rw   ASIC_READ (1) or ASIC_WRITE (0)
  * @param  reg  7-bit register address
  * @param  data 8-bit data (0x00 for reads)
  * @retval 16-bit frame ready to transmit
  */
static uint16_t ASIC_BuildFrame(uint8_t rw, uint8_t reg, uint8_t data)
{
  uint16_t frame = 0;

  frame |= ((uint16_t)(rw & 0x01U) << ASIC_RW_BIT_POS);     /* Bit 15      */
  frame |= ((uint16_t)(reg & 0x7FU) << ASIC_ADDR_BIT_POS);   /* Bits 14:8   */
  frame |= ((uint16_t)(data & 0xFFU) << ASIC_DATA_BIT_POS);  /* Bits 7:0    */

  return frame;
}

/**
  * @brief  Transmit and receive a 16-bit frame over SPI
  * @note   CS is held low for the entire 16-bit exchange.
  *         The 16-bit value is sent as two bytes (MSB first).
  * @param  hasic     Pointer to ASIC handle
  * @param  tx_frame  16-bit frame to transmit
  * @param  rx_frame  Pointer to store 16-bit received frame (can be NULL)
  * @retval ASIC_StatusTypeDef
  */
static ASIC_StatusTypeDef ASIC_TransmitReceive16(ASIC_HandleTypeDef *hasic,
                                                  uint16_t tx_frame,
                                                  uint16_t *rx_frame)
{
  HAL_StatusTypeDef hal_status;
  uint8_t tx_buf[2];
  uint8_t rx_buf[2] = {0};

  /* Split 16-bit frame into two bytes, MSB first */
  tx_buf[0] = (uint8_t)((tx_frame >> 8) & 0xFF);   /* High byte: R/W + Addr */
  tx_buf[1] = (uint8_t)(tx_frame & 0xFF);           /* Low byte:  Data      */

  /* CS low — start of transaction */
  ASIC_CS_Low(hasic);

  /* Full-duplex 2-byte transfer: CS stays low throughout */
  hal_status = HAL_SPI_TransmitReceive(hasic->hspi, tx_buf, rx_buf, 2,
                                       ASIC_SPI_TIMEOUT);

  /* CS high — end of transaction */
  ASIC_CS_High(hasic);

  if (hal_status != HAL_OK)
  {
    return ASIC_ERROR;
  }

  /* Reassemble 16-bit received data if caller wants it */
  if (rx_frame != NULL)
  {
    *rx_frame = ((uint16_t)rx_buf[0] << 8) | (uint16_t)rx_buf[1];
  }

  return ASIC_OK;
}

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize the ASIC driver handle
  */
ASIC_StatusTypeDef ASIC_Init(ASIC_HandleTypeDef *hasic,
                             SPI_HandleTypeDef  *hspi,
                             GPIO_TypeDef       *cs_port,
                             uint16_t            cs_pin)
{
  if (hasic == NULL || hspi == NULL || cs_port == NULL)
  {
    return ASIC_ERROR;
  }

  hasic->hspi    = hspi;
  hasic->cs_port = cs_port;
  hasic->cs_pin  = cs_pin;

  /* Ensure CS starts HIGH (de-asserted) */
  ASIC_CS_High(hasic);

  return ASIC_OK;
}

/**
  * @brief  Write one byte to an ASIC register
  */
ASIC_StatusTypeDef ASIC_WriteReg(ASIC_HandleTypeDef *hasic,
                                 uint8_t reg,
                                 uint8_t data)
{
  uint16_t tx_frame;

  if (hasic == NULL)
    return ASIC_ERROR;

  /* Validate 7-bit address */
  if (reg > 0x7FU)
    return ASIC_ERROR;

  /* Build WRITE frame: R/W=0, Addr, Data */
  tx_frame = ASIC_BuildFrame(ASIC_WRITE, reg, data);

  return ASIC_TransmitReceive16(hasic, tx_frame, NULL);
}

/**
  * @brief  Read one byte from an ASIC register
  */
ASIC_StatusTypeDef ASIC_ReadReg(ASIC_HandleTypeDef *hasic,
                                uint8_t  reg,
                                uint8_t *pData)
{
  uint16_t tx_frame;
  uint16_t rx_frame = 0;
  ASIC_StatusTypeDef status;

  if (hasic == NULL || pData == NULL)
    return ASIC_ERROR;

  if (reg > 0x7FU)
    return ASIC_ERROR;

  /* Build READ frame: R/W=1, Addr, Data=0x00 (dummy) */
  tx_frame = ASIC_BuildFrame(ASIC_READ, reg, 0x00);

  status = ASIC_TransmitReceive16(hasic, tx_frame, &rx_frame);

  if (status == ASIC_OK)
  {
    /* Extract the lower 8 bits — the slave's data response */
    *pData = (uint8_t)(rx_frame & ASIC_DATA_MASK);
  }

  return status;
}

/**
  * @brief  Write multiple consecutive registers (burst write)
  * @note   Each register is a separate 16-bit transaction.
  *         Address auto-increments from the starting register.
  */
ASIC_StatusTypeDef ASIC_WriteBurst(ASIC_HandleTypeDef *hasic,
                                   uint8_t  reg,
                                   uint8_t *pData,
                                   uint16_t len)
{
  uint16_t i;
  ASIC_StatusTypeDef status;

  if (hasic == NULL || pData == NULL || len == 0)
    return ASIC_ERROR;

  for (i = 0; i < len; i++)
  {
    uint8_t current_reg = reg + (uint8_t)i;

    if (current_reg > 0x7FU)
      return ASIC_ERROR;

    status = ASIC_WriteReg(hasic, current_reg, pData[i]);

    if (status != ASIC_OK)
      return status;
  }

  return ASIC_OK;
}

/**
  * @brief  Read multiple consecutive registers (burst read)
  * @note   Each register is a separate 16-bit transaction.
  *         Address auto-increments from the starting register.
  */
ASIC_StatusTypeDef ASIC_ReadBurst(ASIC_HandleTypeDef *hasic,
                                  uint8_t  reg,
                                  uint8_t *pData,
                                  uint16_t len)
{
  uint16_t i;
  ASIC_StatusTypeDef status;

  if (hasic == NULL || pData == NULL || len == 0)
    return ASIC_ERROR;

  for (i = 0; i < len; i++)
  {
    uint8_t current_reg = reg + (uint8_t)i;

    if (current_reg > 0x7FU)
      return ASIC_ERROR;

    status = ASIC_ReadReg(hasic, current_reg, &pData[i]);

    if (status != ASIC_OK)
      return status;
  }

  return ASIC_OK;
}