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

  typedef enum logic {
    IFU_IDLE, IFU_WAIT
  } ifu_state;

  ifu_state next_state;
  ifu_state curr_state;

  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      curr_state <= IFU_IDLE;
    end else begin
      curr_state <= next_state;
    end
  end

  assign inst = io_rdata;
  always_comb begin
    io_reqValid = reqValid;
    respValid   = io_respValid;
    next_state  = curr_state;
    io_addr     = pc;
    case (curr_state)
      IFU_IDLE: begin
        if (reqValid) begin
          io_reqValid = 1'b1;
          next_state  = IFU_WAIT;
          if (io_respValid) begin
            respValid  = 1'b1;
            next_state = IFU_IDLE;
          end
        end
        else begin
          next_state = IFU_IDLE;
        end
      end
      IFU_WAIT: begin
        if (io_respValid) begin
          respValid  = 1'b1;
          next_state = IFU_IDLE;
        end
        else begin
          next_state = IFU_WAIT;
        end
      end
    endcase
  end

`ifdef verilator
/* verilator lint_off UNUSEDSIGNAL */
reg [63:0]  dbg_ifu;

always @ * begin
  case (curr_state)
    IFU_IDLE   : dbg_ifu = "IFU_IDLE";
    IFU_WAIT   : dbg_ifu = "IFU_WAIT";
  endcase
end
/* verilator lint_on UNUSEDSIGNAL */
`endif

endmodule

