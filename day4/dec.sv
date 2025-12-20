module dec (
  input logic [31:0] inst,

  output logic [4:0] rd,
  output logic [4:0] rs1,
  output logic [4:0] rs2,
  output logic [6:0] opcode,
  output logic       wen,
  output logic [31:0] imm
);

  logic sign; 

  always_comb begin
    opcode = inst[6:0];
    rd = inst[11:7];
    rs1 = inst[19:15];
    rs2 = inst[24:20];

    sign = inst[31];
    if (opcode == 7'b0010011) begin
      // ADDI
      if (sign) imm = { 20'sd-1, inst[31:20] };
      else imm = { 20'd0, inst[31:20] };
      wen = 1;
    end else if (opcode == 7'b1100111) begin
      // JALR
      if (sign) imm = { 20'sd-1, inst[31:20] };
      else imm = { 20'd0, inst[31:20] };
      wen = 1;
    end else if (opcode == 7'b0110011) begin
      // ADD
      if (sign) imm = { 12'sd-1, inst[31:12] };
      else imm = { 12'd0, inst[31:12] };
      wen = 1;
    end else  if (opcode == 7'b0110111) begin
      // LUI
      imm = { inst[31:12], 12'd0 };
      wen = 1;
    end else begin
      imm = 0;
      wen = 0;
    end
  end

endmodule;

