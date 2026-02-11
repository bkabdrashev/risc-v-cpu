module core (
  input         clock,
  input         reset,
  input         io_interrupt,
  input         io_master_awready,
  output        io_master_awvalid,
  output [31:0] io_master_awaddr,
  output [ 3:0] io_master_awid,
  output [ 7:0] io_master_awlen,
  output [ 2:0] io_master_awsize,
  output [ 1:0] io_master_awburst,
  input         io_master_wready,
  output        io_master_wvalid,
  output [31:0] io_master_wdata,
  output [ 3:0] io_master_wstrb,
  output        io_master_wlast,
  output        io_master_bready,
  input         io_master_bvalid,
  input  [ 1:0] io_master_bresp,
  input  [ 3:0] io_master_bid,
  input         io_master_arready,
  output        io_master_arvalid,
  output [31:0] io_master_araddr,
  output [ 3:0] io_master_arid,
  output [ 7:0] io_master_arlen,
  output [ 2:0] io_master_arsize,
  output [ 1:0]  io_master_arburst,
  output        io_master_rready,
  input         io_master_rvalid,
  input  [ 1:0] io_master_rresp,
  input  [31:0] io_master_rdata,
  input         io_master_rlast,
  input  [ 3:0] io_master_rid,
  output        io_master_awlock,
  output [ 3:0] io_master_awcache,
  output [ 2:0] io_master_awprot,
  output [ 3:0] io_master_awqos,
  output        io_master_arlock,
  output [ 3:0] io_master_arcache,
  output [ 2:0] io_master_arprot,
  output [ 3:0] io_master_arqos,

  output        io_slave_awready,
  input         io_slave_awvalid,
  input  [31:0] io_slave_awaddr,
  input  [ 3:0] io_slave_awid,
  input  [ 7:0] io_slave_awlen,
  input  [ 2:0] io_slave_awsize,
  input  [ 1:0] io_slave_awburst,
  output        io_slave_wready,
  input         io_slave_wvalid,
  input  [31:0] io_slave_wdata,
  input  [ 3:0] io_slave_wstrb,
  input         io_slave_wlast,
  input         io_slave_bready,
  output        io_slave_bvalid,
  output [ 1:0] io_slave_bid,
  output [ 3:0] io_slave_bresp,
  output        io_slave_arready,
  input         io_slave_arvalid,
  input  [31:0] io_slave_arid,
  input  [ 3:0] io_slave_araddr,
  input  [ 7:0] io_slave_arlen,
  input  [ 2:0] io_slave_arsize,
  input  [ 1:0] io_slave_arburst,
  input         io_slave_rready,
  output        io_slave_rvalid,
  output [ 1:0] io_slave_rid,
  output [31:0] io_slave_rdata,
  output        io_slave_rresp,
  output [ 3:0] io_slave_rlast,
  input         io_slave_awlock, 
  input  [ 3:0] io_slave_awcache, 
  input  [ 2:0] io_slave_awprot, 
  input  [ 3:0] io_slave_awqos, 
  input         io_slave_arlock,
  input  [ 3:0] io_slave_arcache,
  input  [ 2:0] io_slave_arprot,
  input  [ 3:0] io_slave_arqos);

/* verilator lint_off UNUSEDPARAM */
`include "com_defines.vh"
`include "reg_defines.vh"
`include "alu_defines.vh"
`include "inst_defines.vh"
/* verilator lint_on UNUSEDPARAM */
              
/*
              +---+        +---+          +---+
start ------->|IFU|------->|IDU| -------> |LSU|
              +---+<---+   +---+          +---+
                       |      |             |
                     +---+    |             |
                     |EXU|<---+             |
                     +---+<-----------------+
*/

  logic [REG_A_END:0]     idu_rd;
  logic [REG_A_END:0]     idu_rs1;
  logic [REG_A_END:0]     idu_rs2;
  logic [INST_TYPE_END:0] idu_inst_type;
  logic [REG_W_END:0]     idu_imm;
  logic [ALU_OP_END:0]    idu_alu_op;
  logic [COM_OP_END:0]    idu_com_op;

  logic [REG_W_END:0] pc;
  logic               pc_wen;
  logic [REG_W_END:0] pc_jump;
  logic               is_pc_jump;
  logic [REG_W_END:0] pc_next;
  logic [REG_W_END:0] pc_inc;

  logic               rf_wen;

  logic [REG_W_END:0] rf_wdata;
  logic [REG_W_END:0] rf_rdata1;
  logic [REG_W_END:0] rf_rdata2;

  logic [REG_W_END:0] ifu_inst;
  logic               ifu_respValid;
  logic               ifu_reqValid;

  logic               idu_respValid;
  logic               idu_reqValid;
  logic               exu_respValid;
  logic               exu_reqValid;

  logic               is_lsu_inst;
  logic               is_store;

  logic [REG_W_END:0] lsu_rdata;
  logic [REG_W_END:0] lsu_wdata;
  logic [REG_W_END:0] lsu_addr;
  logic               lsu_respValid;
  logic               lsu_reqValid;

  logic [REG_W_END:0] csr_rdata;

  pc u_pc(
    .clock(clock),
    .reset(reset),
    .wen  (pc_wen),
    .wdata(pc_next),
    .rdata(pc));

  ifu u_ifu(
    .clock(clock),
    .reset(reset),
    .respValid   (ifu_respValid),
    .reqValid    (ifu_reqValid),

    .io_respValid(io_ifu_respValid),
    .io_reqValid (io_ifu_reqValid),
    .io_addr     (io_ifu_addr),
    .io_rdata    (io_ifu_rdata),

    .pc          (pc),
    .inst        (ifu_inst));

  assign idu_reqValid = ifu_respValid;
  idu u_idu(
    .clock(clock),
    .reset(reset),
    .respValid(idu_respValid),
    .reqValid (idu_reqValid),

    .inst_in  (ifu_inst),

    .rd       (idu_rd),
    .rs1      (idu_rs1),
    .rs2      (idu_rs2),
    .alu_op   (idu_alu_op),
    .com_op   (idu_com_op),
    .imm      (idu_imm),
    .inst_type(idu_inst_type));

  rf u_rf(
    .clock(clock),
    .reset(reset),

    .wen   (rf_wen),
    .wdata (rf_wdata),
    .rd    (idu_rd),
    .rs1   (idu_rs1),
    .rs2   (idu_rs2),
    .rdata1(rf_rdata1),
    .rdata2(rf_rdata2));

  csr u_csr(
    .clock(clock),
    .reset(reset),
    .addr (idu_imm[11:0]),
    .rdata(csr_rdata));

  assign is_lsu_inst = idu_inst_type[4];
  assign is_store    = idu_inst_type[5:3] == INST_STORE;

  assign lsu_reqValid = idu_respValid && is_lsu_inst;
  lsu u_lsu(
    .clock(clock),
    .reset(reset),

    .reqValid     (lsu_reqValid),
    .respValid    (lsu_respValid),
    .is_write     (is_store),
    .wdata        (lsu_wdata),
    .rdata        (lsu_rdata),
    .addr         (lsu_addr),
    .data_size    (idu_inst_type[1:0]),
    .is_mem_sign  (!idu_inst_type[2]),

    .io_respValid (io_lsu_respValid),
    .io_reqValid  (io_lsu_reqValid),
    .io_wdata     (io_lsu_wdata),
    .io_rdata     (io_lsu_rdata),
    .io_addr      (io_lsu_addr),
    .io_size      (io_lsu_size),
    .io_wen       (io_lsu_wen),
    .io_wmask     (io_lsu_wmask));

  assign pc_wen = exu_respValid;
  assign pc_inc = pc + 4;
  always_comb begin
    pc_next = pc_inc;
    if (is_pc_jump) pc_next = pc_jump;
  end

  assign exu_reqValid = idu_respValid;
  exu u_exu(
    .clock(clock),
    .reset(reset),
    .reqValid       (exu_reqValid),
    .respValid      (exu_respValid),

    .lsu_respValid  (lsu_respValid),
    .is_lsu_inst    (is_lsu_inst),

    .lsu_rdata(lsu_rdata),
    .csr_rdata(csr_rdata),
    .rdata1   (rf_rdata1),
    .rdata2   (rf_rdata2),
    .pc       (pc),
    .pc_inc   (pc_inc),

    .is_pc_jump(is_pc_jump),
    .pc_jump   (pc_jump),
    .rf_wdata  (rf_wdata),
    .rf_wen    (rf_wen),
    .lsu_wdata (lsu_wdata),
    .lsu_addr  (lsu_addr),

    .alu_op   (idu_alu_op),
    .com_op   (idu_com_op),
    .imm      (idu_imm),
    .inst_type(idu_inst_type));

  logic is_start;
  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      ifu_reqValid <= 1'b0;
      is_start     <= 1'b1;
    end else begin
      is_start     <= 1'b0;
      ifu_reqValid <= exu_respValid || is_start;
    end
  end

`ifdef verilator
/* verilator lint_off UNUSEDSIGNAL */
reg [119:0] dbg_inst;
always @ * begin
  case (idu_inst_type)
    INST_EBREAK   : dbg_inst = "INST_EBREAK";
    INST_CSR      : dbg_inst = "INST_CSR";
    INST_CSRI     : dbg_inst = "INST_CSRI";

    INST_LOAD_B  : dbg_inst = "INST_LOAD_B";
    INST_LOAD_H  : dbg_inst = "INST_LOAD_H";
    INST_LOAD_W  : dbg_inst = "INST_LOAD_W";
    INST_LOAD_BU : dbg_inst = "INST_LOAD_BU";
    INST_LOAD_HU : dbg_inst = "INST_LOAD_HU";
    INST_STORE_B : dbg_inst = "INST_STORE_B";
    INST_STORE_H : dbg_inst = "INST_STORE_H";
    INST_STORE_W : dbg_inst = "INST_STORE_W";

    INST_BRANCH     : dbg_inst = "INST_BRANCH";
    INST_IMM        : dbg_inst = "INST_IMM";
    INST_REG        : dbg_inst = "INST_REG";
    INST_UPP        : dbg_inst = "INST_UPP";
    INST_JUMP       : dbg_inst = "INST_JUMP";
    INST_JUMPR      : dbg_inst = "INST_JUMPR";
    INST_AUIPC      : dbg_inst = "INST_AUIPC";
    default         : dbg_inst = "INST_UNDEFINED";
  endcase
end

/* verilator lint_on UNUSEDSIGNAL */
`endif

endmodule

