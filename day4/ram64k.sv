module ram64k #(
  parameter int unsigned WORDS = 16384,             
  parameter string HEXFILE     = ""                 
) (
  input  logic        clk,
  input  logic        wen,
  input  logic [31:0] wdata,
  input  logic [31:0] addr,      // byte address
  output logic [31:0] read_data
);

  logic [31:0] mem [0:WORDS-1];
  initial begin
    if (HEXFILE != "") $readmemh(HEXFILE, mem);
  end

  always_ff @(posedge clk) begin
    if (wen) begin
      if (addr < WORDS*4) begin
        mem[addr] <= wdata;
      end
    end
  end

  always_comb begin
    if (addr < WORDS*4) begin
      read_data = mem[addr];
    end else begin
      read_data = 32'h0000_0000;
    end
  end

endmodule

