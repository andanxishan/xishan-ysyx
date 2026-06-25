/* verilator lint_off DECLFILENAME */
/* verilator lint_off UNUSEDSIGNAL */
/* verilator lint_off WIDTHTRUNC */   // <--- 新增的免死金牌
`timescale 1ns / 1ps

module seg_decoder(
    input      [3:0] bin,
    output reg [7:0] seg
);
    // 映射顺序: [7]=A, [6]=B, [5]=C, [4]=D, [3]=E, [2]=F, [1]=G, [0]=DP
    // 共阳极: 0 = 亮, 1 = 灭
    always @(*) begin
        case(bin)
            4'h0: seg = 8'b0000_0011; // 显示 0
            4'h1: seg = 8'b1001_1111; // 显示 1
            4'h2: seg = 8'b0010_0101; // 显示 2
            4'h3: seg = 8'b0000_1101; // 显示 3
            4'h4: seg = 8'b1001_1001; // 显示 4
            4'h5: seg = 8'b0100_1001; // 显示 5
            4'h6: seg = 8'b0100_0001; // 显示 6
            4'h7: seg = 8'b0001_1111; // 显示 7
            4'h8: seg = 8'b0000_0001; // 显示 8
            4'h9: seg = 8'b0000_1001; // 显示 9
            4'ha: seg = 8'b0001_0001; // 显示 A
            4'hb: seg = 8'b1100_0001; // 显示 b
            4'hc: seg = 8'b0110_0011; // 显示 C
            4'hd: seg = 8'b1000_0101; // 显示 d
            4'he: seg = 8'b0110_0001; // 显示 E
            4'hf: seg = 8'b0111_0001; // 显示 F
            default: seg = 8'b1111_1111; // 全灭
        endcase
    end
endmodule

module top(
    input  wire       clk,
    input  wire       rst,
    output wire [7:0] seg0,
    output wire [7:0] seg1
);

    wire [31:0] cpu_out_data;
    wire        cpu_out_valid;

    scpu u_scpu (
        .clk      (clk),
        .rst      (rst),
        .out_data (cpu_out_data),
        .out_valid(cpu_out_valid)
    );

    reg [7:0] display_reg;
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            display_reg <= 8'h00;
        end else if (cpu_out_valid) begin
            display_reg <= cpu_out_data[7:0];
        end
    end

    // === 二进制转十进制 (Binary to BCD) 组合逻辑 ===
    wire [3:0] dec_tens = display_reg / 10;
    wire [3:0] dec_ones = display_reg % 10;

    // 例化两个数码管译码器，传入十进制的十位和个位
    seg_decoder u_seg0 (.bin(dec_ones), .seg(seg0)); // 个位
    seg_decoder u_seg1 (.bin(dec_tens), .seg(seg1)); // 十位

endmodule
