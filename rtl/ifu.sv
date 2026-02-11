module ifu (
  input  logic  clock,
  input  logic  reset,

  input  logic        io_respValid,
  input  logic [31:0] io_rdata,
  output logic [31:0] io_addr,
  output logic        io_reqValid,

  input  logic [31:0] pc,
  input  logic        reqValid,
  output logic        respValid,
  output logic [31:0] inst);

  logic        icache_wen;
  logic [31:0] icache_wdata;
  logic [31:2] icache_addr;
  logic        icache_hit;
  logic [31:0] icache_rdata;
  logic        icache_reqValid;
  logic        icache_respValid;
 
  assign icache_wdata = inst;
  icache u_icache(
    .clock(clock),
    .reset(reset),
    .wen      (icache_wen),
    .wdata    (icache_wdata),
    .addr     (icache_addr[31:2]),
    .is_hit   (icache_hit),
    .reqValid (icache_reqValid),
    .respValid(icache_respValid),
    .rdata    (icache_rdata));

  typedef enum logic [1:0] {
    IFU_IDLE, IFU_WAIT_ICACHE, IFU_WAIT_IO, IFU_WRITE_ICACHE
  } ifu_state;

  ifu_state next_state;
  ifu_state curr_state;

  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      curr_state <= IFU_IDLE;
    end else begin
      if (reqValid) begin
        curr_state <= next_state;
      end
      else if (io_respValid || icache_respValid) begin
        curr_state <= next_state;
      end
    end
  end

  always_comb begin
    next_state      = curr_state;
    respValid       = 1'b0;
    io_reqValid     = 1'b0;
    icache_reqValid = 1'b0;
    io_addr         = pc;
    icache_addr     = pc[31:2];
    icache_wen      = 1'b0;
    inst            = 32'b0;
    case (curr_state)
      IFU_IDLE: begin
        if (reqValid) begin
          icache_reqValid = 1'b1;
          next_state      = IFU_WAIT_ICACHE;
          if (icache_respValid) begin
            if (icache_hit) begin
              inst       = icache_rdata;
              respValid  = 1'b1;
              next_state = IFU_IDLE;
            end
            else begin
              io_reqValid = 1'b1;
              next_state  = IFU_WAIT_IO;
            end
          end
        end
      end
      IFU_WAIT_ICACHE: begin
        if (icache_respValid) begin
          if (icache_hit) begin
            inst       = icache_rdata;
            respValid  = 1'b1;
            next_state = IFU_IDLE;
          end
          else begin
            io_reqValid = 1'b1;
            next_state  = IFU_WAIT_IO;
          end
        end
      end
      IFU_WAIT_IO: begin
        if (io_respValid) begin
          inst            = io_rdata;
          respValid       = 1'b1;
       // next_state      = IFU_WRITE_ICACHE; TODO: right now we switch to IDLE right away, but if icache write takes more than 1 cycle, it is a bug.
          next_state      = IFU_IDLE;
          icache_wen      = 1'b1;
          icache_reqValid = 1'b1;
        end
      end
      IFU_WRITE_ICACHE: begin
      // TODO: icache and io should be separate buses?
        if (icache_respValid) begin
          next_state   = IFU_IDLE;
        end
      end
    endcase
  end

`ifdef verilator
/* verilator lint_off UNUSEDSIGNAL */
reg [127:0]  dbg_ifu;

always @ * begin
  case (curr_state)
    IFU_IDLE         : dbg_ifu = "IFU_IDLE";
    IFU_WAIT_ICACHE  : dbg_ifu = "IFU_WAIT_ICACHE";
    IFU_WRITE_ICACHE : dbg_ifu = "IFU_WRITE_ICACHE";
    IFU_WAIT_IO      : dbg_ifu = "IFU_WAIT_IO";
  endcase
end
/* verilator lint_on UNUSEDSIGNAL */
`endif

endmodule

