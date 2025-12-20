module alu (
  input logic [6:0] opcode,
  input logic [31:0] rdata1,
  input logic [31:0] rdata2,
  input logic [31:0] imm,
  output logic [31:0] rout
);

  always_comb begin
    if (opcode == 7'b0010011) begin
    // ADDI
      rout = rdata1 + imm;
    end else if (opcode == 7'b0110011) begin
    // ADD
      rout = rdata1 + rdata2;
    end else begin 
      rout = 32'hEFEF_EFEF;
    end
  end

endmodule;




