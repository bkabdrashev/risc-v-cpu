module sm (
  input  logic clock,
  input  logic reset,
  input  logic top_mem_wen,

  input  logic [2:0] in,
  input  logic ifu_respValid,
  input  logic lsu_respValid,
  input  logic [3:0] inst_type,

  output logic ifu_reqValid,
  output logic lsu_reqValid,
  output logic finished,
  output logic [2:0] out
);
/* verilator lint_off UNUSEDPARAM */
  `include "defs.vh"
/* verilator lint_on UNUSEDPARAM */

  logic [2:0] next;

  always_ff @(posedge clock or posedge reset) begin
         if (reset)       out <= STATE_FETCH;
    else if (top_mem_wen) begin
      if (lsu_respValid) begin
        out <= STATE_STORE;
      end
      else begin
        out <= STATE_STORE;
      end
    end
    else out <= next;
  end

  always_comb begin
    finished = 0;
    ifu_reqValid = 0;
    lsu_reqValid = 0;
    unique case (in)
      STATE_FETCH: begin
        if (ifu_respValid)
          case (inst_type)
            INST_LOAD_BYTE: begin next = STATE_LOAD;  lsu_reqValid = 1; end
            INST_LOAD_HALF: begin next = STATE_LOAD;  lsu_reqValid = 1; end 
            INST_LOAD_WORD: begin next = STATE_LOAD;  lsu_reqValid = 1; end 
            INST_STORE:     begin next = STATE_STORE; lsu_reqValid = 1; end 
            default:        begin next = STATE_REG;   lsu_reqValid = 0; end
          endcase
        else begin
          ifu_reqValid = 1;
          next = STATE_FETCH;
        end
      end
      STATE_LOAD: begin
        if (lsu_respValid) next = STATE_REG;
        else next = STATE_LOAD;
      end
      STATE_STORE: begin
        if (lsu_respValid && !top_mem_wen) begin
          finished = 1;
          ifu_reqValid = 1;
          next = STATE_FETCH;
        end
        else if (lsu_respValid && top_mem_wen) begin
          finished = 1;
          lsu_reqValid = 1;
          next = STATE_STORE;
        end
        else begin
          finished = 0;
          lsu_reqValid = 1;
          next = STATE_STORE;
        end
      end
      STATE_REG: begin
        next = STATE_FINISH;
      end
      STATE_FINISH: begin
        finished = 1;
        ifu_reqValid = 1;
        next = STATE_FETCH;
      end
      default: begin
        finished = 1;
        ifu_reqValid = 1;
        next = STATE_FETCH;
      end
    endcase
  end

endmodule


