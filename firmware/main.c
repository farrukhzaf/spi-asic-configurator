/**
  ******************************************************************************
  * @file    main.c
  * @brief   Full SPI slave register test — mirrors the Verilog testbench
  *
  *          Tests:
  *            1. Write all 5 registers with distinct patterns
  *            2. Read back and verify each register
  *            3. Overwrite one register, verify others untouched
  *            4. Read out-of-range addresses (expect 0xFF)
  *            5. Write to out-of-range, verify no corruption
  *            6. Write and read 0x00 (zero distinguishable from default)
  *            7. Hardware reset via ASIC_RST_N pin, verify memory cleared
  *
  *          Open a serial terminal at 115200 baud to see the output.
  ******************************************************************************
  */

#include "stm32g4xx_hal.h"
#include "spi_config.h"
#include "asic_spi.h"
#include "uart_config.h"
#include <stdio.h>

/* ---------- ASIC register addresses (0..4 valid, 5 registers) ---------- */
#ifndef ASIC_REG_0
#define ASIC_REG_0            0x00
#define ASIC_REG_1            0x01
#define ASIC_REG_2            0x02
#define ASIC_REG_3            0x03
#endif
/* ASIC_REG_FIFO_THRESH (0x04) is already defined in asic_spi.h */

/* ---------- ASIC hardware-reset pin (directly drives rst_n on FPGA) ---- */
/* Adjust port/pin to match your board wiring                              */
#ifndef ASIC_RST_PORT
#define ASIC_RST_PORT   GPIOB
#define ASIC_RST_PIN    GPIO_PIN_0
#endif

/* Private variables */
ASIC_HandleTypeDef hasic;
char uart_buf[128];

static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

/* Private function prototypes */
void SystemClock_Config(void);
void Error_Handler(void);

/* ---------------------------------------------------------------------- */
/*  Helper: assert / de-assert hardware reset                              */
/* ---------------------------------------------------------------------- */
static void ASIC_Reset_Init(void)
{
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {0};
  gpio.Pin   = ASIC_RST_PIN;
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ASIC_RST_PORT, &gpio);

  /* Start with reset de-asserted (rst_n = 1 ? normal operation) */
  HAL_GPIO_WritePin(ASIC_RST_PORT, ASIC_RST_PIN, GPIO_PIN_SET);
}

static void ASIC_Reset_Assert(void)
{
  /* rst_n = 0 ? hold ASIC in reset */
  HAL_GPIO_WritePin(ASIC_RST_PORT, ASIC_RST_PIN, GPIO_PIN_RESET);
  HAL_Delay(2);
}

static void ASIC_Reset_Deassert(void)
{
  /* rst_n = 1 ? release reset */
  HAL_GPIO_WritePin(ASIC_RST_PORT, ASIC_RST_PIN, GPIO_PIN_SET);
  HAL_Delay(2);
}

/* ---------------------------------------------------------------------- */
/*  Helper: write + error check                                            */
/* ---------------------------------------------------------------------- */
static int asic_write(uint8_t addr, uint8_t val)
{
  if (ASIC_WriteReg(&hasic, addr, val) != ASIC_OK)
  {
    sprintf(uart_buf, "  ERROR: SPI write failed (addr 0x%02X)\r\n", addr);
    UART_Print(uart_buf);
    return -1;
  }
  return 0;
}

/* ---------------------------------------------------------------------- */
/*  Helper: read + error check                                             */
/* ---------------------------------------------------------------------- */
static int asic_read(uint8_t addr, uint8_t *val)
{
  if (ASIC_ReadReg(&hasic, addr, val) != ASIC_OK)
  {
    sprintf(uart_buf, "  ERROR: SPI read failed (addr 0x%02X)\r\n", addr);
    UART_Print(uart_buf);
    return -1;
  }
  return 0;
}

/* ---------------------------------------------------------------------- */
/*  Helper: read-and-compare with PASS / FAIL logging                      */
/* ---------------------------------------------------------------------- */
static void check(uint8_t addr, uint8_t expected)
{
  uint8_t got = 0x00;
  if (asic_read(addr, &got) != 0)
  {
    tests_failed++;
    return;
  }

  if (got == expected)
  {
    sprintf(uart_buf, "  [PASS] addr 0x%02X : got 0x%02X (expected 0x%02X)\r\n",
            addr, got, expected);
    tests_passed++;
  }
  else
  {
    sprintf(uart_buf, "  [FAIL] addr 0x%02X : got 0x%02X (expected 0x%02X)\r\n",
            addr, got, expected);
    tests_failed++;
  }
  UART_Print(uart_buf);
}

/* ====================================================================== */
/*  main                                                                   */
/* ====================================================================== */
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  if (SPI_Config_Init() != HAL_OK)
    Error_Handler();
  if (UART_Config_Init() != HAL_OK)
    Error_Handler();

  ASIC_Reset_Init();

  if (ASIC_Init(&hasic, &hspi1, ASIC_CS_PORT, ASIC_CS_PIN) != ASIC_OK)
    Error_Handler();

  UART_Print("\r\n========================================\r\n");
  UART_Print("       ASIC SPI Register Test\r\n");
  UART_Print("========================================\r\n");
	
	ASIC_Reset_Assert();
  ASIC_Reset_Deassert();

  /* ================================================================== */
  /* TEST 1 — Write all 5 registers with distinct patterns               */
  /* ================================================================== */
  UART_Print("\r\n--- TEST 1 : Write all 5 registers ---\r\n");
  if (asic_write(0x00, 0xA5) != 0) Error_Handler();
  if (asic_write(0x01, 0x3C) != 0) Error_Handler();
  if (asic_write(0x02, 0x0F) != 0) Error_Handler();
  if (asic_write(0x03, 0x81) != 0) Error_Handler();
  if (asic_write(0x04, 0xFF) != 0) Error_Handler();
  UART_Print("  Writes complete.\r\n");

  /* ================================================================== */
  /* TEST 2 — Read back and verify each register                         */
  /* ================================================================== */
  UART_Print("\r\n--- TEST 2 : Read back all 5 registers ---\r\n");
  check(0x00, 0xA5);
  check(0x01, 0x3C);
  check(0x02, 0x0F);
  check(0x03, 0x81);
  check(0x04, 0xFF);

  /* ================================================================== */
  /* TEST 3 — Overwrite register 0, verify it and others untouched       */
  /* ================================================================== */
  UART_Print("\r\n--- TEST 3 : Overwrite addr 0 ---\r\n");
  if (asic_write(0x00, 0x42) != 0) Error_Handler();
  check(0x00, 0x42);
  check(0x01, 0x3C);   /* must still be original value */

  /* ================================================================== */
  /* TEST 4 — Read out-of-range addresses (expect 0xFF from RTL)         */
  /* ================================================================== */
  UART_Print("\r\n--- TEST 4 : Read out-of-range addresses ---\r\n");
  check(0x0A, 0xFF);
  check(0x7F, 0xFF);

  /* ================================================================== */
  /* TEST 5 — Write to out-of-range, verify no corruption                */
  /* ================================================================== */
  UART_Print("\r\n--- TEST 5 : Write out-of-range, check no corruption ---\r\n");
  asic_write(0x0A, 0xBE);         /* should be silently ignored */
  check(0x00, 0x42);              /* still 0x42 from TEST 3 */
  check(0x04, 0xFF);              /* still 0xFF from TEST 1 */

  /* ================================================================== */
  /* TEST 6 — Write 0x00 and read back (zero distinguishable)            */
  /* ================================================================== */
  UART_Print("\r\n--- TEST 6 : Write and read 0x00 ---\r\n");
  if (asic_write(0x02, 0x00) != 0) Error_Handler();
  check(0x02, 0x00);

  /* ================================================================== */
  /* TEST 7 — Hardware reset clears all registers                        */
  /* ================================================================== */
  UART_Print("\r\n--- TEST 7 : Hardware reset clears memory ---\r\n");
  ASIC_Reset_Assert();
  ASIC_Reset_Deassert();
  check(0x00, 0x00);
  check(0x01, 0x00);
  check(0x02, 0x00);
  check(0x03, 0x00);
  check(0x04, 0x00);

  /* ================================================================== */
  /* Summary                                                              */
  /* ================================================================== */
  UART_Print("\r\n========================================\r\n");
  sprintf(uart_buf, "  PASSED : %u\r\n", tests_passed);
  UART_Print(uart_buf);
  sprintf(uart_buf, "  FAILED : %u\r\n", tests_failed);
  UART_Print(uart_buf);
  UART_Print("========================================\r\n");

  if (tests_failed == 0)
    UART_Print(">>> ALL TESTS PASSED <<<\r\n");
  else
    UART_Print(">>> SOME TESTS FAILED <<<\r\n");

  while (1)
  {
    /* Test complete — spin */
  }
}

/* ====================================================================== */
/*  System Clock Configuration (170 MHz from 24 MHz HSE)                   */
/* ====================================================================== */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM            = RCC_PLLM_DIV6;
  RCC_OscInitStruct.PLL.PLLN            = 85;
  RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    Error_Handler();

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK |
                                     RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_8) != HAL_OK)
    Error_Handler();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) { }
}
