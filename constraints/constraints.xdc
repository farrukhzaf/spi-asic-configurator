## ============================================================================
## Basys3 Constraints - SPI Slave on PMOD JA
## ============================================================================

## --- PMOD JA (directly wire your SPI master to these pins) ---
## JA1  →  CS_N
set_property -dict { PACKAGE_PIN J1  IOSTANDARD LVCMOS33 } [get_ports {cs_n}]
## JA2  →  MOSI
set_property -dict { PACKAGE_PIN L2  IOSTANDARD LVCMOS33 } [get_ports {mosi}]
## JA3  ←  MISO
set_property -dict { PACKAGE_PIN J2  IOSTANDARD LVCMOS33 } [get_ports {miso}]
## JA4  →  SCLK
set_property -dict { PACKAGE_PIN G2  IOSTANDARD LVCMOS33 } [get_ports {sclk}]

## --- Center Button - Reset ---
set_property -dict { PACKAGE_PIN H1 IOSTANDARD LVCMOS33 } [get_ports {rst_n}]

set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets cs_n_IBUF]
set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets sclk_IBUF]

## --- Clock Constraint on SCLK (entering as a clock via PMOD) ---
## Adjust period to match your SPI master frequency.
## 100 ns = 10 MHz. Change if your master runs faster/slower.
create_clock -name spi_clk -period 100.000 [get_ports {sclk}]

## --- No system clock used - mark config voltage ---
set_property CFGBVS VCCO [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]
