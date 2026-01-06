module csr (
  input  logic        clock,
  input  logic        reset,
  input  logic        wen,
  input  logic        is_done_inst,
  input  logic [11:0] addr,
  input  logic [REG_END_WORD:0] wdata,
  output logic [REG_END_WORD:0] rdata);
/* verilator lint_off UNUSEDPARAM */
  `include "defs.vh"
/* verilator lint_on UNUSEDPARAM */
  localparam MCYCLE_ADDR    = 12'hB00; 
  localparam MCYCLEH_ADDR   = 12'hB80; 
  localparam MINSTRET_ADDR  = 12'hB02; 
  localparam MINSTRETH_ADDR = 12'hB82; 
  localparam MVENDORID_ADDR = 12'hf11; 
  localparam MARCHID_ADDR   = 12'hf12; 
  localparam MISA_ADDR      = 12'h301; 

  // extensions:             MXL    ZY XWVUTSRQ PONMLKJI HGFEDCBA
  localparam MISA      = 32'b01_000000_00000000_00000000_00010000;
  localparam MVENDORID = "beka";
  localparam MARCHID   = 32'h05318008; 

  logic [63:0] mcycle;
  logic [63:0] minstret;

  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      mcycle   <= 64'h0;
      minstret <= 64'h0;
    end
    else if (wen) begin
           if (addr == MCYCLE_ADDR)  mcycle <= {mcycle[63:32], wdata};
      else if (addr == MCYCLEH_ADDR) mcycle <= {wdata, mcycle[31: 0]};
      else                           mcycle <= mcycle + 1;

           if (addr == MINSTRET_ADDR)  minstret <= {minstret[63:32], wdata};
      else if (addr == MINSTRETH_ADDR) minstret <= {wdata, minstret[31: 0]};
      else if (is_done_inst)           minstret <= minstret + 1;
    end
    else begin
      mcycle <= mcycle + 1;
      if (is_done_inst) minstret <= minstret + 1;
    end
  end

  always_comb begin
    case (addr) 
      MISA_ADDR:      rdata = MISA;
      MARCHID_ADDR:   rdata = MARCHID;
      MVENDORID_ADDR: rdata = MVENDORID;
      MCYCLE_ADDR:    rdata = mcycle[31: 0];
      MCYCLEH_ADDR:   rdata = mcycle[63:32];
      MINSTRET_ADDR:  rdata = minstret[31: 0];
      MINSTRETH_ADDR: rdata = minstret[63:32];
      default:        rdata = 32'h0;
    endcase
  end

endmodule

