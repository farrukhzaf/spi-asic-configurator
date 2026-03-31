`timescale 1ns / 1ps

module spi_slave_memory_tb;

    reg        sclk;
    reg        cs;
    reg        mosi;
    reg        rst_n;
    wire       miso;

    // Register output ports
    wire [7:0] reg0_out, reg1_out, reg2_out, reg3_out, reg4_out;

    spi_slave_memory dut (
        .sclk     (sclk),
        .cs       (cs),
        .mosi     (mosi),
        .rst_n    (rst_n),
        .miso     (miso),
        .reg0_out (reg0_out),
        .reg1_out (reg1_out),
        .reg2_out (reg2_out),
        .reg3_out (reg3_out),
        .reg4_out (reg4_out)
    );

    localparam SCLK_HALF   = 50;
    localparam SCLK_PERIOD = SCLK_HALF * 2;

    integer tests_passed = 0;
    integer tests_failed = 0;

    // ----------------------------------------------------------------
    // Task : spi_write - R/W = 1 → Write
    // ----------------------------------------------------------------
    task spi_write;
        input [6:0] address;
        input [7:0] data;
        reg   [15:0] frame;
        integer i;
        begin
            frame = {1'b1, address, data};
            #(SCLK_HALF/2);
            cs = 1'b0;
            #(SCLK_HALF/2);

            for (i = 15; i >= 0; i = i - 1) begin
                mosi = frame[i];
                #(SCLK_HALF);
                sclk = 1'b1;
                #(SCLK_HALF);
                sclk = 1'b0;
            end

            #(SCLK_HALF);
            mosi = 1'b0;
            cs = 1'b1;
            #(SCLK_PERIOD);
        end
    endtask

    // ----------------------------------------------------------------
    // Task : spi_read - R/W = 0 → Read
    // ----------------------------------------------------------------
    task spi_read;
        input  [6:0] address;
        output [7:0] data;
        reg    [15:0] frame;
        integer i;
        begin
            frame = {1'b0, address, 8'h00};
            data  = 8'h00;
            #(SCLK_HALF/2);
            cs = 1'b0;
            #(SCLK_HALF/2);

            for (i = 15; i >= 0; i = i - 1) begin
                mosi = frame[i];
                #(SCLK_HALF);
                sclk = 1'b1;
                if (i < 8) begin
                    #1;
                    data[i] = miso;
                end
                #(SCLK_HALF);
                sclk = 1'b0;
            end

            #(SCLK_HALF);
            mosi = 1'b0;
            cs = 1'b1;
            #(SCLK_PERIOD);
        end
    endtask

    // ----------------------------------------------------------------
    // Task : check - MISO read-back compare
    // ----------------------------------------------------------------
    task check;
        input [7:0] got;
        input [7:0] expected;
        input [6:0] address;
        begin
            if (got === expected) begin
                $display("[PASS] addr %0d : got 0x%02h (expected 0x%02h)",
                         address, got, expected);
                tests_passed = tests_passed + 1;
            end else begin
                $display("[FAIL] addr %0d : got 0x%02h (expected 0x%02h)",
                         address, got, expected);
                tests_failed = tests_failed + 1;
            end
        end
    endtask

    // ----------------------------------------------------------------
    // Task : check_output - verify reg_out port value
    // ----------------------------------------------------------------
    task check_output;
        input [7:0] got;
        input [7:0] expected;
        input [31:0] reg_num;    // just for display
        begin
            if (got === expected) begin
                $display("[PASS] reg%0d_out : got 0x%02h (expected 0x%02h)",
                         reg_num, got, expected);
                tests_passed = tests_passed + 1;
            end else begin
                $display("[FAIL] reg%0d_out : got 0x%02h (expected 0x%02h)",
                         reg_num, got, expected);
                tests_failed = tests_failed + 1;
            end
        end
    endtask

    initial sclk = 1'b0;

    reg [7:0] rd_data;

    initial begin
        cs    = 1'b1;
        mosi  = 1'b0;
        rst_n = 1'b0;
        #(SCLK_PERIOD * 3);
        rst_n = 1'b1;
        #(SCLK_PERIOD * 2);

        // =============================================================
        // TEST 1 - Write all 5 registers
        // =============================================================
        $display("\n===== TEST 1 : Write all 5 registers =====");
        spi_write(7'd0, 8'hA5);
        spi_write(7'd1, 8'h3C);
        spi_write(7'd2, 8'h0F);
        spi_write(7'd3, 8'h81);
        spi_write(7'd4, 8'hFF);

        // =============================================================
        // TEST 2 - Read back via MISO and verify
        // =============================================================
        $display("\n===== TEST 2 : Read back all 5 registers (MISO) =====");
        spi_read(7'd0, rd_data);  check(rd_data, 8'hA5, 7'd0);
        spi_read(7'd1, rd_data);  check(rd_data, 8'h3C, 7'd1);
        spi_read(7'd2, rd_data);  check(rd_data, 8'h0F, 7'd2);
        spi_read(7'd3, rd_data);  check(rd_data, 8'h81, 7'd3);
        spi_read(7'd4, rd_data);  check(rd_data, 8'hFF, 7'd4);

        // =============================================================
        // TEST 3 - Verify register output ports match
        // =============================================================
        $display("\n===== TEST 3 : Verify reg_out output ports =====");
        check_output(reg0_out, 8'hA5, 0);
        check_output(reg1_out, 8'h3C, 1);
        check_output(reg2_out, 8'h0F, 2);
        check_output(reg3_out, 8'h81, 3);
        check_output(reg4_out, 8'hFF, 4);

        // =============================================================
        // TEST 4 - Overwrite addr 0, verify port updates immediately
        // =============================================================
        $display("\n===== TEST 4 : Overwrite addr 0, check port updates =====");
        spi_write(7'd0, 8'h42);
        check_output(reg0_out, 8'h42, 0);
        check_output(reg1_out, 8'h3C, 1);   // must be untouched
        spi_read(7'd0, rd_data);  check(rd_data, 8'h42, 7'd0);
        spi_read(7'd1, rd_data);  check(rd_data, 8'h3C, 7'd1);

        // =============================================================
        // TEST 5 - Read out-of-range addresses
        // =============================================================
        $display("\n===== TEST 5 : Read out-of-range address =====");
        spi_read(7'd10, rd_data);  check(rd_data, 8'hFF, 7'd10);
        spi_read(7'd127, rd_data); check(rd_data, 8'hFF, 7'd127);

        // =============================================================
        // TEST 6 - Write to out-of-range, verify no corruption
        // =============================================================
        $display("\n===== TEST 6 : Write out-of-range, check no corruption =====");
        spi_write(7'd10, 8'hBE);
        spi_read(7'd0, rd_data);  check(rd_data, 8'h42, 7'd0);
        spi_read(7'd4, rd_data);  check(rd_data, 8'hFF, 7'd4);
        check_output(reg0_out, 8'h42, 0);
        check_output(reg4_out, 8'hFF, 4);

        // =============================================================
        // TEST 7 - Write and read 0x00
        // =============================================================
        $display("\n===== TEST 7 : Write and read 0x00 =====");
        spi_write(7'd2, 8'h00);
        spi_read(7'd2, rd_data);  check(rd_data, 8'h00, 7'd2);
        check_output(reg2_out, 8'h00, 2);

        // =============================================================
        // TEST 8 - Async reset clears all registers and outputs
        // =============================================================
        $display("\n===== TEST 8 : Async reset clears memory and outputs =====");
        rst_n = 1'b0;
        #(SCLK_PERIOD * 2);
        // Check outputs are 0 DURING reset
        check_output(reg0_out, 8'h00, 0);
        check_output(reg4_out, 8'h00, 4);
        rst_n = 1'b1;
        #(SCLK_PERIOD * 2);
        // Check outputs stay 0 AFTER reset
        spi_read(7'd0, rd_data);  check(rd_data, 8'h00, 7'd0);
        spi_read(7'd3, rd_data);  check(rd_data, 8'h00, 7'd3);
        check_output(reg0_out, 8'h00, 0);
        check_output(reg3_out, 8'h00, 3);

        // =============================================================
        // Summary
        // =============================================================
        $display("\n============================================");
        $display("  PASSED : %0d", tests_passed);
        $display("  FAILED : %0d", tests_failed);
        $display("============================================\n");
        if (tests_failed == 0)
            $display(">>> ALL TESTS PASSED <<<");
        else
            $display(">>> SOME TESTS FAILED <<<");

        #(SCLK_PERIOD * 2);
        $finish;
    end

    initial begin
        $dumpfile("spi_slave_memory.vcd");
        $dumpvars(0, spi_slave_memory_tb);
    end

endmodule
