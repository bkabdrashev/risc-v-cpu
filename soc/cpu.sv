module cpu (
  input         clock,
  input         reset,

  input  [31:0] io_ifu_rdata,
  input         io_ifu_respValid,
  output        io_ifu_reqValid,
  output [31:0] io_ifu_addr,

  input         io_lsu_respValid,
  input  [31:0] io_lsu_rdata,
  output        io_lsu_reqValid,
  output [31:0] io_lsu_addr,
  output [1:0]  io_lsu_size,
  output        io_lsu_wen,
  output [31:0] io_lsu_wdata,
  output [3:0]  io_lsu_wmask);

  import reg_defines::REG_A_END;
  import reg_defines::REG_W_END;
  import inst_defines::*;
  import alu_defines::ALU_OP_END;
  import com_defines::COM_OP_END;

  logic               is_inst_ret;
  logic               is_fetch_inst;
  logic               is_lsu_busy;
  logic               is_ifu_busy;
  logic               ebreak;

  logic [REG_A_END:0] rd;
  logic [REG_A_END:0] rs1;
  logic [REG_A_END:0] rs2;
  logic [REG_W_END:0] imm;

  logic [REG_W_END:0] pc;
  logic               pc_wen;
  logic               is_pc_jump;
  logic [REG_W_END:0] pc_next;
  logic [REG_W_END:0] pc_inc;

  logic [REG_W_END:0]  alu_lhs;
  logic [REG_W_END:0]  alu_rhs;
  logic [ALU_OP_END:0] alu_op;
  logic [REG_W_END:0]  alu_res;

  logic [COM_OP_END:0] com_op;
  logic                com_res;

  logic               reg_wen;
  logic [REG_W_END:0] reg_wdata;
  logic [REG_W_END:0] reg_rdata1;
  logic [REG_W_END:0] reg_rdata2;

  logic [INST_TYPE_END:0] inst_type;
  logic                   is_inst_ready;

  logic               is_lsu;
  logic [REG_W_END:0] lsu_rdata;

  logic [REG_W_END:0] csr_rdata;
  logic [REG_W_END:0] csr_wdata;
  logic               csr_wen;

  pc u_pc(
    .clock(clock),
    .reset(reset),
    .wen  (pc_wen),
    .wdata(pc_next),
    .rdata(pc));

  assign io_ifu_addr = pc_next;
  ifu u_ifu(
    .clock(clock),
    .reset(reset),
    .is_fetch_inst (is_fetch_inst),
    .is_busy       (is_ifu_busy),
    .is_inst_ready (is_inst_ready),
    .respValid     (io_ifu_respValid),
    .reqValid      (io_ifu_reqValid));

  dec u_dec(
    .inst(io_ifu_rdata),

    .rd(rd),
    .rs1(rs1),
    .rs2(rs2),

    .alu_op(alu_op),
    .com_op(com_op),
    .imm(imm),
    .inst_type(inst_type));

  always_comb begin
    case (inst_type)
      INST_CSRI:   alu_lhs = {27'b0,  rs1};
      INST_JUMP:   alu_lhs = pc;
      INST_AUIPC:  alu_lhs = pc;
      INST_BRANCH: alu_lhs = pc;
      default:     alu_lhs = reg_rdata1;
    endcase
  end

  always_comb begin
    case (inst_type)
      INST_REG:  alu_rhs = reg_rdata2;
      INST_CSR:  alu_rhs = csr_rdata;
      INST_CSRI: alu_rhs = csr_rdata;
      default:   alu_rhs = imm;
    endcase
  end

  alu u_alu(
    .op(alu_op),
    .lhs(alu_lhs),
    .rhs(alu_rhs),
    .res(alu_res));

  com u_com(
    .op(com_op),
    .lhs(reg_rdata1),
    .rhs(reg_rdata2),
    .res(com_res));

  rf u_rf(
    .clock(clock),
    .reset(reset),

    .wen(reg_wen),
    .wdata(reg_wdata),

    .rd(rd),
    .rs1(rs1),
    .rs2(rs2),

    .rdata1(reg_rdata1),
    .rdata2(reg_rdata2));

  assign csr_wen   = inst_type[5];
  assign csr_wdata = alu_res;
  csr u_csr(
    .clock(clock),
    .reset(reset),
    .wen(csr_wen),
    .addr(imm[11:0]),
    .is_inst_ret(is_inst_ret),
    .wdata(csr_wdata),
    .rdata(csr_rdata));

  assign is_lsu      = inst_type[4] & is_inst_ready;
  assign io_lsu_wen  = inst_type[5:3] == INST_STORE;
  assign io_lsu_size = inst_type[1:0];

  lsu u_lsu(
    .clock(clock),
    .reset(reset),

    .is_lsu     (is_lsu),
    .is_busy    (is_lsu_busy),
    .rdata      (io_lsu_rdata),
    .wdata      (reg_rdata2),
    .addr       (alu_res),
    .data_size  (inst_type[1:0]),
    .is_mem_sign(inst_type[2]),

    .respValid    (io_lsu_respValid),

    .reqValid     (io_lsu_reqValid),
    .lsu_wdata    (io_lsu_wdata),
    .lsu_rdata    (lsu_rdata),
    .lsu_addr     (io_lsu_addr),
    .lsu_wmask    (io_lsu_wmask));

  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      ebreak <= 1'b0;
    end else begin
      ebreak <= inst_type == INST_EBREAK;
    end
  end

  typedef enum logic [1:0] {
    START, STAL, EXEC, NONE
  } exu_state;

  exu_state next_state;
  exu_state curr_state;

  logic next_is_fetch_inst;
  assign is_pc_jump  = inst_type == INST_JUMP || inst_type == INST_JUMPR || (inst_type == INST_BRANCH && com_res);
  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      curr_state    <= START;
      is_inst_ret   <= 1'b0;
      is_fetch_inst <= 1'b0;
    end else begin

      curr_state    <= next_state;
      is_inst_ret   <= next_state == STAL && curr_state == EXEC;
      is_fetch_inst <= next_is_fetch_inst;
    end
  end

  always_comb begin
    next_is_fetch_inst = 1'b0;
    case (curr_state) 
      START: begin
        next_state = STAL;
        next_is_fetch_inst = 1'b1;
      end
      STAL: begin
        if (is_ifu_busy || is_lsu_busy || io_ifu_reqValid || io_lsu_reqValid) next_state = STAL;
        else begin
          next_state = EXEC;
          next_is_fetch_inst = 1'b1;
        end
      end
      EXEC: begin
        next_state = STAL;
      end
      NONE: begin
        next_state = STAL;
      end
    endcase
  end

  assign reg_wen = inst_type[3] && next_state == EXEC && !ebreak;
  assign pc_wen  = next_is_fetch_inst;
  
  assign pc_inc  = pc + 4;
  always_comb begin
    if (curr_state == START || next_state == STAL) pc_next = pc;
    else if (is_pc_jump)                           pc_next = alu_res;
    else                                           pc_next = pc_inc;
  end

  always_comb begin
    case (inst_type)
      INST_JUMP:    reg_wdata = pc_inc;
      INST_JUMPR:   reg_wdata = pc_inc;

      INST_UPP:     reg_wdata = alu_res;
      INST_AUIPC:   reg_wdata = alu_res;
      INST_REG:     reg_wdata = alu_res;
      INST_IMM:     reg_wdata = alu_res;

      INST_CSR:     reg_wdata = csr_rdata;
      INST_CSRI:    reg_wdata = csr_rdata;

      INST_LOAD_B:  reg_wdata = lsu_rdata;
      INST_LOAD_H:  reg_wdata = lsu_rdata;
      INST_LOAD_W:  reg_wdata = lsu_rdata;
      INST_LOAD_BU: reg_wdata = lsu_rdata;
      INST_LOAD_HU: reg_wdata = lsu_rdata;
      
      default:      reg_wdata = 0;
    endcase
  end

`ifdef verilator
/* verilator lint_off UNUSEDSIGNAL */
reg [119:0] dbg_inst_type;
reg [39:0]  dbg_state;

always @ * begin
  case (inst_type)
    INST_EBREAK   : dbg_inst_type = "INST_EBREAK";
    INST_CSR      : dbg_inst_type = "INST_CSR";
    INST_CSRI     : dbg_inst_type = "INST_CSRI";

    INST_LOAD_B  : dbg_inst_type = "INST_LOAD_B";
    INST_LOAD_H  : dbg_inst_type = "INST_LOAD_H";
    INST_LOAD_W  : dbg_inst_type = "INST_LOAD_W";
    INST_LOAD_BU : dbg_inst_type = "INST_LOAD_BU";
    INST_LOAD_HU : dbg_inst_type = "INST_LOAD_HU";
    INST_STORE_B : dbg_inst_type = "INST_STORE_B";
    INST_STORE_H : dbg_inst_type = "INST_STORE_H";
    INST_STORE_W : dbg_inst_type = "INST_STORE_W";

    INST_BRANCH     : dbg_inst_type = "INST_BRANCH";
    INST_IMM        : dbg_inst_type = "INST_IMM";
    INST_REG        : dbg_inst_type = "INST_REG";
    INST_UPP        : dbg_inst_type = "INST_UPP";
    INST_JUMP       : dbg_inst_type = "INST_JUMP";
    INST_JUMPR      : dbg_inst_type = "INST_JUMPR";
    INST_AUIPC      : dbg_inst_type = "INST_AUIPC";
    default         : dbg_inst_type = "INST_UNDEFINED";
  endcase
end
always @ * begin
  case (curr_state)
    START : dbg_state = "START";
    STAL  : dbg_state = "STAL" ;
    EXEC  : dbg_state = "EXEC" ;
    NONE  : dbg_state = "NONE" ;
  endcase
end
/* verilator lint_on UNUSEDSIGNAL */
`endif

endmodule

