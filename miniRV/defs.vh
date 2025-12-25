localparam INITIAL_PC = 32'h8000_0000;
localparam N_REGS    = 16;
localparam REG_END_WORD = 31;
localparam REG_END_HALF = 15;
localparam REG_END_BYTE = 7;

localparam REG_END_ID = 4;

localparam OP_ADD  = 4'b0000; 
localparam OP_SUB  = 4'b1000; 
localparam OP_SLL  = 4'b0001; 
localparam OP_SLT  = 4'b0010; 
localparam OP_SLTU = 4'b0011; 
localparam OP_XOR  = 4'b0100; 
localparam OP_SRL  = 4'b0101; 
localparam OP_SRA  = 4'b1101; 
localparam OP_OR   = 4'b0110; 
localparam OP_AND  = 4'b0111; 
localparam FUNCT3_SR      = 3'b101;

localparam INST_LOAD_BYTE = 3'b000;
localparam INST_LOAD_HALF = 3'b001;
localparam INST_LOAD_WORD = 3'b010;
localparam INST_STORE     = 3'b011;
localparam INST_IMM       = 3'b100;
localparam INST_REG       = 3'b101;
localparam INST_UPP       = 3'b110;
localparam INST_JUMP      = 3'b111;


localparam FUNCT3_BYTE        = 3'b000;
localparam FUNCT3_HALF        = 3'b001;
localparam FUNCT3_WORD        = 3'b010;
localparam FUNCT3_BYTE_UNSIGN = 3'b100;
localparam FUNCT3_HALF_UNSIGN = 3'b101;

localparam OPCODE_LUI   = 7'b0110111;
localparam OPCODE_AUIPC = 7'b0010111;
localparam OPCODE_JAL   = 7'b1101111;
localparam OPCODE_JALR  = 7'b1100111;
localparam OPCODE_LOAD  = 7'b0000011;
localparam OPCODE_STORE = 7'b0100011;
localparam OPCODE_CALC_IMM  = 7'b0010011;
localparam OPCODE_CALC_REG  = 7'b0110011;
localparam OPCODE_ENV       = 7'b1110011;
