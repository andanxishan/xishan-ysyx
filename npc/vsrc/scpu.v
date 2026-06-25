/* verilator lint_off UNUSEDSIGNAL */
`timescale 1ns / 1ps

module scpu(
    input  wire        clk,
    input  wire        rst,
    output reg  [31:0] out_data,
    output reg         out_valid
);

    reg [31:0] pc;
    wire [31:0] next_pc;

    always @(posedge clk or posedge rst) begin
        if (rst) pc <= 32'h0;
        else     pc <= next_pc;
    end

    reg [31:0] rom [0:63]; 

    initial begin
    	$readmemh("/home/xishan/ysyx-workbench/npc/vsrc/inst.txt", rom);
    end

    wire [31:0] instr;
    // 修复位宽警告: 只取需要的 6 位地址
    assign instr = rom[pc[7:2]]; 

    wire [6:0]  opcode = instr[6:0];
    wire [4:0]  rd     = instr[11:7];
    wire [4:0]  rs1    = instr[19:15];
    wire [4:0]  rs2    = instr[24:20];
    
    wire [31:0] imm_i = {{20{instr[31]}}, instr[31:20]};
    // 修复硬件 Bug: 将 instr[11] 修正为 instr[11:8] 补齐 32 位
    wire [31:0] imm_b = {{20{instr[31]}}, instr[7], instr[30:25], instr[11:8], 1'b0};

    reg [31:0] regs [0:31];
    wire [31:0] rdata1 = (rs1 == 0) ? 32'b0 : regs[rs1];
    wire [31:0] rdata2 = (rs2 == 0) ? 32'b0 : regs[rs2];

    wire [31:0] alu_op2 = (opcode == 7'b0010011) ? imm_i : rdata2; 
    wire [31:0] alu_result = rdata1 + alu_op2;

    wire reg_write = (opcode == 7'b0110011) || (opcode == 7'b0010011); 

    always @(posedge clk) begin
        if (reg_write && rd != 0) begin
            regs[rd] <= alu_result;
        end
    end

    wire branch_en = (opcode == 7'b1100011) && (rdata1 != rdata2); 
    assign next_pc = branch_en ? (pc + imm_b) : (pc + 4);

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            out_data  <= 32'b0;
            out_valid <= 1'b0;
        end else if (opcode == 7'b1111011) begin
            out_data  <= rdata1;
            out_valid <= 1'b1;
        end else begin
            out_valid <= 1'b0;
        end
    end

endmodule
