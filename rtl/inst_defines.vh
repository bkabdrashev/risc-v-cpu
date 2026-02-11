// 0-2 define the inst base type
// 3   define reg_wen
// 4   define lsu on 
// 5   define system
localparam INST_TYPE_END   = 5;

localparam INST_SYSTEM     = 3'b101;
localparam INST_EBREAK     = 6'b101000;
localparam INST_CSR        = 6'b101001;
localparam INST_CSRI       = 6'b101101;

localparam INST_LOAD    = 3'b011;
localparam INST_LOAD_B  = 6'b011000;
localparam INST_LOAD_H  = 6'b011001;
localparam INST_LOAD_W  = 6'b011010;
localparam INST_LOAD_BU = 6'b011100;
localparam INST_LOAD_HU = 6'b011101;
localparam INST_STORE   = 3'b010;
localparam INST_STORE_B = 6'b010000;
localparam INST_STORE_H = 6'b010001;
localparam INST_STORE_W = 6'b010010;

localparam INST_EXEC    = 2'b00;
localparam INST_BRANCH  = 6'b000101;
localparam INST_JUMPR   = 6'b001001;
localparam INST_JUMP    = 6'b001101;
localparam INST_REG     = 6'b001000;
localparam INST_IMM     = 6'b001010;
localparam INST_AUIPC   = 6'b001100;
localparam INST_UPP     = 6'b001110;
localparam INST_CALC    = 1'b0;     // if exec then inst_type[0] == 0 is CALC
