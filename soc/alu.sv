module alu (
  input  logic [3:0]            op,
  input  logic [REG_END_WORD:0] lhs,
  input  logic [REG_END_WORD:0] rhs,
  output logic [REG_END_WORD:0] res
);
/* verilator lint_off UNUSEDPARAM */
  `include "./soc/defs.vh"
/* verilator lint_on UNUSEDPARAM */

  logic [4:0] shamt;

  always_comb begin
    shamt = rhs[4:0];
    case (op)
      OP_ADD: res = lhs + rhs;
      OP_SUB: res = lhs - rhs;
      OP_XOR: res = lhs ^ rhs;
      OP_AND: res = lhs & rhs;
      OP_OR:  res = lhs | rhs;
      OP_SLL: res = lhs << shamt;
      OP_SRL: res = lhs >> shamt;
      OP_SRA: res = $signed(lhs) >>> shamt;
      OP_SLT: res = { 31'b0, $signed(lhs) < $signed(rhs) };
      OP_SLTU: res = { 31'b0, lhs < rhs };
      default: res = 'b1010; // not implemented
    endcase
  end

endmodule;




