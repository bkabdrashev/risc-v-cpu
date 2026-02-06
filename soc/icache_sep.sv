module icache_sep (
  input  logic        clock,
  input  logic        reset,
  input  logic        wen,
  input  logic [31:0] wdata,
  input  logic [31:2] addr,
  input  logic        reqValid,
  output logic        is_hit,
  output logic        respValid,
  output logic [31:0] rdata);

  localparam SETS   = 256;
  localparam DATA_B = 4;
  localparam DATA_W = 8 * DATA_B;
  localparam m      = $clog2(DATA_B);
  localparam n      = $clog2(SETS);
  localparam TAG_W  = 32-m-n;

/*
      ICACHE
  +---+-----+------+
  | 1 |TAG_W|DATA_W|
  +---+-----+------+
  | v | tag | data |
  +---+-----+------+
  |   |     |      |
  +---+-----+------+
*/

  logic              valids [0:SETS-1];
  logic [ TAG_W-1:0] tags   [0:SETS-1];
  logic [DATA_W-1:0] data   [0:SETS-1];

  logic  [TAG_W-1:0]  tag;
  logic  [    n-1:0]  index;
  assign {tag, index} = addr[31:m];
  assign rdata  = data[index];

  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      for (integer s = 0; s < SETS; s++) begin : sets_reset_valid_ff
        valids[s] <= 1'b0;
      end
    end
    else if (wen) begin
      valids[index] <= 1'b1;
      tags  [index] <= tag;
      data  [index] <= wdata;
    end
  end

  logic writeValid;
  logic readValid;
  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      writeValid <= 1'b0;
    end
    else begin
      writeValid <= wen;
    end
  end

  assign respValid = writeValid | readValid;

  always_comb begin
    is_hit    = 1'b0;
    readValid = 1'b0;
    if (reqValid && !wen) begin
      readValid = 1'b1;
      if (valids[index] && tags[index] == tag) begin
        is_hit = 1'b1;
      end
    end
  end

`ifdef verilator
import "DPI-C" context task icache_perf_measure(input bit is_hit);
import "DPI-C" context task icache_perf_reset();

always_ff @(posedge clock or posedge reset) begin
  if (reset) begin
    icache_perf_reset();
  end
  else begin
    icache_perf_measure(is_hit);
  end
end
`endif

endmodule

