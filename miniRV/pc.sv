module pc (
  input logic clock,
  input logic reset,
  input logic [REG_END_WORD:0] in_addr,
  output logic [REG_END_WORD:0] out_addr
);
/* verilator lint_off UNUSEDPARAM */
  `include "defs.vh"
/* verilator lint_on UNUSEDPARAM */

  always_ff @(posedge clock or posedge reset) begin
    /**/ if (reset)  out_addr <= INITIAL_PC;
    else if (!reset) out_addr <= in_addr;
  end

endmodule;


