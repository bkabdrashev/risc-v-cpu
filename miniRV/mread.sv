module mread (
  input logic                  clock,
  input logic [REG_END_WORD:0] addr,    

  output logic [REG_END_WORD:0] rdata
);
/* verilator lint_off UNUSEDPARAM */
  `include "defs.vh"
/* verilator lint_on UNUSEDPARAM */

  import "DPI-C" context function int unsigned mem_read(input int unsigned address);
  always_ff @(posedge clock) begin
    rdata <= mem_read(addr);
    // $display("addr: %h, inst: %d", addr, rdata);
  end
endmodule


