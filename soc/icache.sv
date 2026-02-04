module icache (
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
  localparam WAYS   = 1;
  localparam DATA_B = 4;
  localparam DATA_W = 8 * DATA_B;
  localparam LINES  = SETS * WAYS;
  localparam m      = $clog2(DATA_B);
  localparam n      = $clog2(LINES);
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

  typedef struct packed {
    logic              valid;
    logic [ TAG_W-1:0] tag;
    logic [DATA_W-1:0] data;
  } line_t;

  line_t lines [0:LINES-1];

  logic  [TAG_W-1:0]  tag;
  logic  [    n-1:0]  index;
  line_t              line;
  assign {tag, index} = addr[31:m];
  assign line  = lines[index];
  assign rdata = line.data;

  logic  writeValid;
  logic  readValid;
    always_ff @(posedge clock or posedge reset) begin
      if (reset) begin
        for (integer i = 0; i < LINES; i++) begin : gen_lines_ff
          lines[i].valid <= 1'b0;
        end
      end
      else if (wen) begin
        lines[index] <= {1'b1, tag, wdata};
      end
    end

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
      is_hit    = line.valid && line.tag == tag;
      readValid = 1'b1;
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

