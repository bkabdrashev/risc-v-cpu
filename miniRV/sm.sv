module sm (
  input  logic clock,
  input  logic reset,
  input  logic is_load,
  input  logic [1:0] in,
  output logic [1:0] out
);
/* verilator lint_off UNUSEDPARAM */
  `include "defs.vh"
/* verilator lint_on UNUSEDPARAM */

  always_ff @(posedge clock, posedge reset, posedge is_load) begin
    if (reset) out <= BUS_IDLE;
    else unique case ({in, is_load})
      {BUS_IDLE,      1'b0}: out <= BUS_WAIT_INST;
      {BUS_IDLE,      1'b1}: out <= BUS_WAIT_INST;
      {BUS_WAIT_INST, 1'b0}: out <= BUS_IDLE;
      {BUS_WAIT_INST, 1'b1}: out <= BUS_WAIT_LOAD;
      {BUS_WAIT_LOAD, 1'b0}: out <= BUS_IDLE;
      {BUS_WAIT_LOAD, 1'b1}: out <= BUS_IDLE;
      default:               out <= BUS_IDLE;
    endcase
    // $display("state out: %d", out);
  end

  always_comb begin
  end
endmodule


