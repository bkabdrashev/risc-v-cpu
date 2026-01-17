module icache (
  input  logic         clock,
  input  logic         reset,

  output logic        rdata,
  input  logic        wen,
  input  logic [31:0] wdata,
  input  logic [31:0] addr);

  localparam m = 2;
  localparam n = 4;
  localparam BLOCK_BYTES = 2**m; // b
  localparam NUM_BLOCKS  = 2**n; // k
  localparam TAG_SIZE    = 32-m-n;
  localparam BLOCK_SIZE  = TAG_SIZE+1+32;
  logic [TAG_SIZE-1:0] tag;
  logic [   n-1:0] index;
  logic [   m-1:0] offset;
  assign tag    = addr[   31:m+n];
  assign index  = addr[m+n-1:  m];
  assign offset = addr[  m-1:  0];

  logic [BLOCK_SIZE-1:0] blocks [0:NUM_BLOCKS];

/*
          31     0
+---+-----+------+
| v | tag | data |
+---+-----+------+
*/

  assign block = blocks[offset];
  assign valid = block[BLOCK_SIZE-1];
  assign rtag  = block[TAG_SIZE+32-1:32];
  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      blocks[offset][BLOCK_SIZE-1] <= 1'b0;
    end
    else begin
      if (wen) begin
        if (valid) begin
          if (rtag == tag) begin
            blocks[offset] <= {1'b1, rtag, wdata};
          end
        end
      end
    end
  end

endmodule
