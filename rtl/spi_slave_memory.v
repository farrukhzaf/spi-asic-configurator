// ============================================================================
// SPI Slave with 5-Register Memory - ASIC-safe (Mode 0, No System Clock)
// ----------------------------------------------------------------------------
// Protocol : {R/W, A[6:0], D[7:0]}  - 16 bits per transaction
//   R/W = 1 → WRITE (master sends D[7:0] on MOSI during clocks 9-16)
//   R/W = 0 → READ  (slave drives D[7:0] on MISO during clocks 9-16)
// SPI Mode 0 : CPOL=0, CPHA=0
//
// ASIC design rules:
//   1. Single clock: posedge sclk for all logic, negedge sclk for MISO
//      output only (standard SPI pattern - one inverted clock flop,
//      handled by SDC generated clock + DFT scan-mode mux)
//   2. rst_n: global async reset for all flops
//   3. cs: async clear for bit_count ONLY - safe because SPI protocol
//      guarantees cs transitions only while sclk is idle (CPOL=0 →
//      sclk=0), so recovery/removal timing is inherently satisfied.
//      cs is NOT used as a clock anywhere.
//   4. All other logic samples cs as synchronous data input on posedge sclk
//   5. No combinational outputs - MISO is fully registered
//   6. Synchronous memory read/write
//
// SDC constraints:
//   create_clock -name sclk -period <T> [get_ports sclk]
//   create_generated_clock -name sclk_inv -source [get_ports sclk] \
//       -divide_by 1 -invert [get_pins miso_reg/CK]
//   set_false_path -from [get_ports cs] -to [get_pins bit_count_reg*/CLR]
// ============================================================================

module spi_slave_memory (
    input  wire sclk,       // SPI Clock (idle low, Mode 0)
    input  wire cs,         // Chip Select (active low)
    input  wire mosi,       // Master Out Slave In
    input  wire rst_n,      // Asynchronous reset (active low)
    output reg  miso,       // Master In Slave Out (negedge sclk registered)

    // --- Register outputs (directly wire to your ASIC logic) ---
    output wire [7:0] reg0_out,   // addr 0x00
    output wire [7:0] reg1_out,   // addr 0x01
    output wire [7:0] reg2_out,   // addr 0x02
    output wire [7:0] reg3_out,   // addr 0x03
    output wire [7:0] reg4_out    // addr 0x04
);

    localparam MEM_DEPTH = 5;

    reg [15:0] shift_reg;
    reg [4:0]  bit_count;
    reg [7:0]  tx_buffer;

    // Named registers instead of memory array - clean synthesis names,
    // easy to trace in layout/LVS, and directly exposed as outputs.
    reg [7:0]  reg_file [0:MEM_DEPTH-1];

    assign reg0_out = reg_file[0];
    assign reg1_out = reg_file[1];
    assign reg2_out = reg_file[2];
    assign reg3_out = reg_file[3];
    assign reg4_out = reg_file[4];

    // ---------------------------------------------------------------
    // 1. Bit Counter  (posedge sclk, async clear from cs HIGH + rst_n)
    // ---------------------------------------------------------------
    //  cs HIGH asynchronously clears bit_count to 0.
    //  Safe because SPI protocol guarantees cs transitions only
    //  while sclk is idle (low in Mode 0) - no setup/hold race.
    // ---------------------------------------------------------------
    always @(posedge sclk or posedge cs or negedge rst_n) begin
        if (!rst_n)
            bit_count <= 5'b0;
        else if (cs)
            bit_count <= 5'b0;
        else if (bit_count < 5'd16)
            bit_count <= bit_count + 1'b1;
    end

    // ---------------------------------------------------------------
    // 2. MOSI Shift Register  (posedge sclk, async reset from rst_n)
    // ---------------------------------------------------------------
    //  Shifts in MOSI when cs is low and frame not complete.
    //  cs sampled as synchronous enable - no async dependency.
    // ---------------------------------------------------------------
    always @(posedge sclk or negedge rst_n) begin
        if (!rst_n)
            shift_reg <= 16'b0;
        else if (!cs && bit_count < 5'd16)
            shift_reg <= {shift_reg[14:0], mosi};
    end

    // ---------------------------------------------------------------
    // 3. Predictive tx_buffer Load  (posedge sclk)
    // ---------------------------------------------------------------
    //  At bit_count == 7 (pre-NBA), 7 bits are in shift_reg[6:0]:
    //      R/W  = shift_reg[6]
    //      addr = {shift_reg[5:0], mosi}   (mosi provides addr[0])
    //  R/W = 0 → READ: pre-load tx_buffer from memory.
    //  tx_buffer is valid after this posedge, ready for negedge MISO.
    // ---------------------------------------------------------------
    always @(posedge sclk or negedge rst_n) begin
        if (!rst_n)
            tx_buffer <= 8'h00;
        else if (!cs && bit_count == 5'd7 && !shift_reg[6])  begin
            if ({shift_reg[5:0], mosi} < MEM_DEPTH)
                tx_buffer <= reg_file[{shift_reg[5:0], mosi}];
            else
                tx_buffer <= 8'hFF;
        end
    end

    // ---------------------------------------------------------------
    // 4. MISO Output  (negedge sclk - single output flop)
    // ---------------------------------------------------------------
    //  After posedge 8: bit_count = 8, tx_buffer valid.
    //  negedge after posedge 8 → drives tx_buffer[7].
    //  Master samples at posedge 9 → captures tx_buffer[7]. Correct.
    //
    //  This is the ONE negedge flop. In DFT, a scan-mode mux
    //  switches it to posedge for shift. Standard practice.
    // ---------------------------------------------------------------
    always @(negedge sclk or negedge rst_n) begin
        if (!rst_n)
            miso <= 1'b0;
        else if (cs)
            miso <= 1'b0;
        else if (bit_count >= 5'd8 && bit_count < 5'd16)
            miso <= tx_buffer[5'd15 - bit_count];
        else
            miso <= 1'b0;
    end

    // ---------------------------------------------------------------
    // 5. Memory Write  (posedge sclk - 16th clock)
    // ---------------------------------------------------------------
    //  At bit_count == 15 (pre-NBA), 15 bits are in shift_reg[14:0]:
    //      R/W  = shift_reg[14]
    //      addr = shift_reg[13:7]
    //      data = {shift_reg[6:0], mosi}   (mosi provides data[0])
    // ---------------------------------------------------------------
    integer i;
    always @(posedge sclk or negedge rst_n) begin
        if (!rst_n) begin
            for (i = 0; i < MEM_DEPTH; i = i + 1)
                reg_file[i] <= 8'h00;
        end else if (!cs && bit_count == 5'd15) begin
            if (shift_reg[14] && shift_reg[13:7] < MEM_DEPTH)
                reg_file[shift_reg[13:7]] <= {shift_reg[6:0], mosi};
        end
    end

endmodule
