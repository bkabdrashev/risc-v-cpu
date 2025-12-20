module rf (
  input logic       clk,
  input logic       reset,
  input logic       wen,
  input logic [4:0]  rd,
  input logic [4:0] rs1,
  input logic [4:0]  rs2,
  input logic [31:0] wdata,
  output logic [31:0] reg_out0,
  output logic [31:0] reg_out1,
  output logic [31:0] reg_out2,
  output logic [31:0] reg_out3,
  output logic [31:0] reg_out4,
  output logic [31:0] reg_out5,
  output logic [31:0] reg_out6,
  output logic [31:0] reg_out7,
  output logic [31:0] rdata1,
  output logic [31:0] rdata2
);

  logic [31:0] regs [0:15];
  logic [3:0] trunc_rd;
  logic [3:0] trunc_rs1;
  logic [3:0] trunc_rs2;
  integer i;

  always_ff @(posedge clk or posedge reset) begin
    if (reset) begin
      for (i = 0; i < 16; i++) begin
        regs[i] <= 32'h0;
      end
    end else if (wen && (rd != 0) && (rd < 16)) begin
      regs[trunc_rd] <= wdata;
    end
  end

  always_comb begin
    trunc_rd = rd[3:0];
    trunc_rs1 = rs1[3:0];
    trunc_rs2 = rs2[3:0];
    rdata1 = (rs1 == 0 && rs1 >= 16) ? 32'h0 : regs[trunc_rs1];
    rdata2 = (rs2 == 0 && rs2 >= 16) ? 32'h0 : regs[trunc_rs2];
    reg_out0 = regs[0];
    reg_out1 = regs[1];
    reg_out2 = regs[2];
    reg_out3 = regs[3];
    reg_out4 = regs[4];
    reg_out5 = regs[5];
    reg_out6 = regs[6];
    reg_out7 = regs[7];
  end

endmodule;


