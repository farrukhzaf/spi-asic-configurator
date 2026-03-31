// ============================================================================
// Basys3 Top-Level Wrapper for SPI Slave
// ----------------------------------------------------------------------------
// PMOD JA (directly connect your SPI master here):
//   JA[0]  →  SCLK   (input)
//   JA[1]  →  MOSI   (input)
//   JA[2]  →  CS_N   (input)
//   JA[3]  ←  MISO   (output)
//
// Reset  : Center push-button (BTNC) - active-high on board, inverted here
// ============================================================================

module top (
    input  wire sclk,
    input  wire mosi,
    input  wire cs_n,
    output wire miso,
    input  wire rst_n
);

//    wire rst_n = ~btnC;        // Basys3 buttons are active-high

    spi_slave_memory u_spi (
        .sclk  (sclk),
        .mosi  (mosi),
        .cs  (cs_n),
        .miso  (miso),
        .rst_n (rst_n)
    );

endmodule
