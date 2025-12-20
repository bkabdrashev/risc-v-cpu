module miniRV (
  input logic reset,
  input logic clk,
  input logic rom_wen,
  input logic [31:0] rom_wdata,
  input logic [31:0] rom_addr,
  output logic [31:0] reg0,
  output logic [31:0] reg1,
  output logic [31:0] reg2,
  output logic [31:0] reg3,
  output logic [31:0] reg4,
  output logic [31:0] reg5,
  output logic [31:0] reg6,
  output logic [31:0] reg7,
  output logic [31:0] pc,
  output logic [31:0] inst
);

  logic [4:0] dec_rd;
  logic [4:0] dec_rs1;
  logic [4:0] dec_rs2;
  logic [31:0] dec_imm;
  logic        dec_wen;
  logic [6:0]  dec_opcode;

  logic is_pc_jump;
  logic [31:0]  pc_addr;

  logic [31:0] rdata1;
  logic [31:0] rdata2;
  logic [31:0] alu_res;

  logic [31:0] rom_addr_or_pc;

  logic [31:0] wdata;

  logic        ram_wen;
  logic [31:0] ram_addr;
  logic [31:0] ram_wdata;
  logic [31:0] ram_rdata;

  pc u_pc(
    .clk(clk),
    .reset(reset),
    .in_addr(pc_addr),
    .is_addr(is_pc_jump),
    .out_addr(pc)
  );

  // TODO: write to ram/rom
  ram64k ram(.clk(clk), .wen(ram_wen), .wdata(ram_wdata), .addr(ram_addr), .read_data(ram_rdata));
  ram64k rom(.clk(clk), .wen(rom_wen), .wdata(rom_wdata), .addr(rom_addr_or_pc), .read_data(inst));

  dec u_dec(
    .inst(inst),

    .rd(dec_rd),
    .rs1(dec_rs1),
    .rs2(dec_rs2),
    .imm(dec_imm),
    .wen(dec_wen),
    .opcode(dec_opcode)
  );

  alu u_alu(
    .opcode(dec_opcode),
    .rdata1(rdata1),
    .rdata2(rdata2),
    .imm(dec_imm),

    .rout(alu_res)
  );

  rf u_rf(
    .clk(clk),
    .reset(reset),
    .wen(dec_wen),
    .rd(dec_rd),
    .wdata(wdata),
    .rs1(dec_rs1),
    .rs2(dec_rs2),

    .rdata1(rdata1),
    .rdata2(rdata2),
    .reg_out0(reg0),
    .reg_out1(reg1),
    .reg_out2(reg2),
    .reg_out3(reg3),
    .reg_out4(reg4),
    .reg_out5(reg5),
    .reg_out6(reg6),
    .reg_out7(reg7) 
  );

  always_comb begin
    if (rom_wen) begin
      rom_addr_or_pc = rom_addr;
      wdata = 0;
      pc_addr = 0;
      is_pc_jump = 0;
      ram_wen = 0;
      ram_addr = 0;
      ram_wdata = 0;
    end else begin
      rom_addr_or_pc = pc;
      if (dec_opcode == 7'b0010011) begin
        // ADDI
        ram_wen = 0;
        ram_addr = 0;
        ram_wdata = 0;
        wdata = alu_res;
        pc_addr = 0;
        is_pc_jump = 0;
      end else if (dec_opcode == 7'b1100111) begin
        // JALR
        ram_wen = 0;
        ram_addr = 0;
        ram_wdata = 0;
        pc_addr = (rdata1 + dec_imm) & ~1;
        wdata = pc+4;
        is_pc_jump = 1;
      end else if (dec_opcode == 7'b0110011) begin
        // ADD
        ram_wen = 0;
        ram_addr = 0;
        ram_wdata = 0;
        wdata = alu_res;
        pc_addr = 0;
        is_pc_jump = 0;
      end else if (dec_opcode == 7'b0110111) begin
        // LUI
        ram_wen = 0;
        ram_addr = 0;
        ram_wdata = 0;
        wdata = dec_imm;
        pc_addr = 0;
        is_pc_jump = 0;
      end else if (dec_opcode == 7'b0000011) begin
        // LW
        ram_wen = 0;
        ram_addr = rdata1 + dec_imm;
        ram_wdata = 0;
        wdata = ram_rdata;
        pc_addr = 0;
        is_pc_jump = 0;
      end else if (dec_opcode == 7'b0100011) begin
        // SW
        ram_wen = 1;
        ram_addr = rdata1 + dec_imm;
        ram_wdata = rdata2;
        wdata = 0;
        pc_addr = 0;
        is_pc_jump = 0;
      end else begin
        // NOT IMPLEMENTED
        ram_wen = 0;
        ram_addr = 0;
        ram_wdata = 0;
        wdata = 0;
        pc_addr = 0;
        wdata = 0;
        is_pc_jump = 0;
      end
    end
  end

endmodule;


