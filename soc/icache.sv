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
  localparam WAY_W  = $clog2(WAYS);

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

  typedef line_t [0:WAYS-1] set_t;
  set_t sets [0:SETS-1];

  integer unsigned victim_way;
  if (WAYS > 1) begin : gen_ways
    logic invalid_found;
    integer unsigned invalid_way;

    always_comb begin
      invalid_found = 1'b0;
      invalid_way   = 0;

      for (integer w = 0; w < WAYS; w++) begin
        if (!invalid_found && !sets[index][w].valid) begin
          invalid_found = 1'b1;
          invalid_way   = w;
        end
      end

    end

    logic [WAY_W-1:0] lru_stack [0:SETS-1][0:WAYS-1];
    always_comb begin
      if (invalid_found) begin
        victim_way = invalid_way;
      end else begin
        victim_way = lru_stack[index][WAYS-1]; // LRU
      end
    end
    always_ff @(posedge clock or posedge reset) begin
      if (reset) begin
        for (integer s = 0; s < SETS; s++) begin : sets_reset_lru_stack
          for (integer w = 0; w < WAYS; w++) begin : ways_reset_lru_stack
            lru_stack[s][w]  <= logic'(w[WAY_W-1:0]);
          end
        end
      end
      else if (wen) begin
        integer pos;
        pos = 0;

        for (integer w = 0; w < WAYS; w++) begin
         if (lru_stack[index][w] == logic'(victim_way[WAY_W-1:0])) 
          pos = w;
        end

        // shift [0..pos-1] down by 1, place victim at [0]
        for (int i = pos; i > 0; i--) begin
         lru_stack[index][i] <= lru_stack[index][i-1];
        end
        lru_stack[index][0] <= logic'(victim_way[WAY_W-1:0]);
      end
    end
  end
  else begin : gen_no_ways
    assign victim_way = 0;
  end
  logic hit_found;
  integer unsigned hit_way;

  logic  [TAG_W-1:0]  tag;
  logic  [    n-1:0]  index;
  assign {tag, index} = addr[31:m];
  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      for (integer s = 0; s < SETS; s++) begin : sets_reset_valid_ff
        for (integer w = 0; w < WAYS; w++) begin : ways_reset_valid_ff
          sets[s][w].valid <= 1'b0;
        end
      end
    end
    else if (wen) begin
      sets[index][victim_way].valid <= 1'b1;
      sets[index][victim_way].tag   <= tag;
      sets[index][victim_way].data  <= wdata;
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
    hit_found = 1'b0;
    hit_way   = 0;
    if (reqValid && !wen) begin
      for (integer w = 0; w < WAYS; w++) begin : gen_ways
        if (sets[index][w].valid && sets[index][w].tag == tag) begin
          hit_found = 1'b1;
          hit_way   = w;
        end
      end
    end
  end

  always_comb begin
    is_hit    = 1'b0;
    readValid = 1'b0;
    rdata     = 32'b0;

    if (reqValid && !wen) begin
      readValid = 1'b1;
      if (hit_found) begin
        is_hit = 1'b1;
        rdata  = sets[index][hit_way].data;
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

