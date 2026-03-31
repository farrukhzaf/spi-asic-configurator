# SPI Slave Register Interface — ASIC-Ready RTL

A 16-bit SPI Mode 0 slave that provides read/write access to five 8-bit configuration registers. Designed for ASIC register programming with minimal pin count (4 wires). Validated on FPGA hardware with an STM32G474 SPI master, then hardened for ASIC tape-out.

## Architecture

```
STM32G474 (Master)          Basys 3 / ASIC (Slave)
┌──────────┐                ┌──────────────────────┐
│          │──── SCLK ────▶│                      │
│   SPI1   │──── MOSI ────▶│   spi_slave_memory   │──▶ reg0_out [7:0]
│          │◀─── MISO ────│                      │──▶ reg1_out [7:0]
│          │──── CS_N ────▶│                      │──▶ reg2_out [7:0]
│   PB0  ──│──── RST_N ──▶│                      │──▶ reg3_out [7:0]
└──────────┘                └──────────────────────┘──▶ reg4_out [7:0]
```

## Frame Format

```
┌─────────┬───────────────┬──────────────┐
│  Bit 15 │  Bits [14:8]  │  Bits [7:0]  │
│   R/W   │  Addr [6:0]   │  Data [7:0]  │
└─────────┴───────────────┴──────────────┘
  1=Write     7-bit addr     8-bit data
  0=Read
```

- **SPI Mode 0**: CPOL=0, CPHA=0 — SCLK idles low, data sampled on rising edge
- **16 bits per transaction**, MSB first, CS held low for entire frame
- **No system clock** — all logic clocked by SPI master's SCLK

## Repository Structure

```
├── rtl/
│   ├── spi_slave_memory.v    # SPI slave core (ASIC-ready)
│   └── top.v                 # Basys 3 FPGA top-level wrapper
│
├── tb/
│   └── tb_spi_slave_memory.v # Verilog testbench (Icarus/Vivado)
│
├── firmware/
│   ├── main.c                # STM32 test harness (7 test categories)
│   ├── asic_spi.c            # SPI protocol driver
│   ├── asic_spi.h            # Driver header + register map
│   ├── spi_config.c          # SPI1 & GPIO hardware init
│   ├── spi_config.h          # Pin definitions (Nucleo-G474RE)
│   ├── uart_config.c         # LPUART1 for test output
│   └── uart_config.h         # UART header
│
├── constraints/
│   └── constraints.xdc       # Basys 3 XDC (PMOD JA pinout)
│
└── docs/
    └── SPI_slave_design_report.pdf
```

## RTL Design

Five always blocks, each with a single responsibility:

| Block | Function | Clock Edge | Async |
|-------|----------|-----------|-------|
| 1 | Bit counter | posedge sclk | rst_n, cs (clear only) |
| 2 | Shift register | posedge sclk | rst_n |
| 3 | TX buffer (predictive load) | posedge sclk | rst_n |
| 4 | MISO output | negedge sclk | rst_n |
| 5 | Memory write | posedge sclk | rst_n |

### ASIC Design Rules

- **Single clock source**: All logic on `posedge sclk`; only the MISO output flop uses `negedge sclk`, constrained as a generated clock
- **Targeted async clear**: `cs` clears `bit_count` only — safe because SPI protocol guarantees CS transitions while SCLK is idle
- **No combinational outputs**: MISO is fully registered
- **DFT-ready**: One scan-mode mux on the negedge MISO flop

### SDC Constraints

```tcl
create_clock -name sclk -period <T> [get_ports sclk]

create_generated_clock -name sclk_inv \
    -source [get_ports sclk] \
    -divide_by 1 -invert \
    [get_pins miso_reg/CK]

set_false_path -from [get_ports cs] \
    -to [get_pins bit_count_reg*/CLR]
```

## Simulation

Run with Icarus Verilog:

```bash
iverilog -o spi_tb tb/tb_spi_slave_memory.v rtl/spi_slave_memory.v
vvp spi_tb
```

Or open a waveform:

```bash
gtkwave spi_slave_memory.vcd
```

All 35 checks pass (MISO read-back + output port verification + async reset).

## Hardware Test Results

STM32G474 firmware runs 7 test categories over UART at 115200 baud:

| Test | Category | Result |
|------|----------|--------|
| 1 | Write all 5 registers | PASS |
| 2 | Read back all 5 registers | PASS |
| 3 | Overwrite addr 0, verify others | PASS |
| 4 | Read out-of-range addresses | PASS |
| 5 | Write out-of-range, no corruption | PASS |
| 6 | Write and read 0x00 | PASS |
| 7 | Hardware reset clears all | PASS |
| | **Total** | **17/17** |

## Pin Mapping

### Basys 3 FPGA (PMOD JA)

| PMOD Pin | Package Pin | Signal | Direction |
|----------|-------------|--------|-----------|
| JA1 | J1 | CS_N | Input |
| JA2 | L2 | MOSI | Input |
| JA3 | J2 | MISO | Output |
| JA4 | G2 | SCLK | Input |

### STM32 Nucleo-G474RE

| STM32 Pin | Function |
|-----------|----------|
| PA5 | SPI1_SCK |
| PA6 | SPI1_MISO |
| PA7 | SPI1_MOSI |
| PB6 | CS (GPIO) |
| PB0 | RST_N (GPIO) |

## Design History

| Rev | Milestone | Key Change |
|-----|-----------|-----------|
| 0 | Initial code | 5 bugs identified |
| 1 | Simulation pass | Fixed R/W polarity, MISO timing, predictive tx_buffer, registered MISO, single counter |
| 2 | Signal rename | cs_n → cs; R/W: 1=Write, 0=Read |
| 3 | FPGA race fix | Memory write moved from posedge cs to posedge sclk |
| 4 | ASIC hardening | cs removed as clock/reset except targeted bit_count clear; SDC constraints |
| 5 | Output ports | reg_file with reg[0..4]_out ports for layout integration |
