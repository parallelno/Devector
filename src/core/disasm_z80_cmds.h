#include "core/disasm.h"

static const dev::Cmd Z80_CMD_NOP = {
		{ "nop" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_LXI_B_NN = {
		{ "ld", " ", "bc", ", "},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_STAX_B = {
		{ "ld", " ", "(", "bc", ")", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_INX_B = {
		{ "inc", " ", "bc" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_INR_B = {
		{ "inc", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DCR_B = {
		{ "dec", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MVI_B_N = {
		{ "ld", " ", "b", ", "},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RLC = {
		{ "rlca" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DAD_B = {
		{ "add", " ", "hl", ", ", "bc" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_LDAX_B = {
		{ "ld", " ", "a", ", ", "(", "bc", ")",},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DCX_B = {
		{ "dec", " ", "bc" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_INR_C = {
		{ "inc", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DCR_C = {
		{ "dec", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MVI_C_N = {
		{ "ld", " ", "c", ", "},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RRC = {
		{ "rrca" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_LXI_D = {
		{ "ld", " ", "de", ", "},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_STAX_D = {
		{ "ld", " ", "(", "de", ")", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_INX_D = {
		{ "inc", " ", "de" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_INR_D = {
		{ "inc", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DCR_D = {
		{ "dec", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MVI_D_N = {
		{ "ld", " ", "d", ", "},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RAL = {
		{ "rla" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DAD_D = {
		{ "add", " ", "hl", ", ", "de" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_LDAX_D = {
		{ "ld", " ", "a", ", ", "(", "de", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DCX_D = {
		{ "dec", " ", "de" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_INR_E = {
		{ "inc", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DCR_E = {
		{ "dec", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MVI_E_N = {
		{ "ld", " ", "e", ", "},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RAR = {
		{ "rra" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_LXI_H = {
		{ "ld", " ", "hl", ", "},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_SHLD_NN = {
		{ "ld", " ", "(", "NN", ")", ", ", "hl" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_IMM, dev::CMD_TT_RPAREN, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_INX_H = {
		{ "inc", " ", "hl" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_INR_H = {
		{ "inc", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DCR_H = {
		{ "dec", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MVI_H_N = {
		{ "ld", " ", "h", ", "},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_DAA = {
		{ "daa" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DAD_H = {
		{ "add", " ", "hl", ", ", "hl" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_LHLD_NN = {
		{ "ld", " ", "hl", ", ", "(", "NN", ")", },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_IMM, dev::CMD_TT_RPAREN },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_DCX_H = {
		{ "dec", " ", "hl" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_INR_L = {
		{ "inc", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DCR_L = {
		{ "dec", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MVI_L_N = {
		{ "ld", " ", "l", ", ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_CMA = {
		{ "cpl" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_LXI_SP = {
		{ "ld", " ", "sp", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_STA_NN = {
		{ "ld", " ", "(", "NN", ")", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_IMM, dev::CMD_TT_RPAREN, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_INX_SP = {
		{ "inc", " ", "sp" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_INR_M = {
		{ "inc", " ", "(", "hl", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DCR_M = {
		{ "dec", " ", "(", "hl", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MVI_M_N = {
		{ "ld", " ", "(", "hl", ")", ", "},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_STC = {
		{ "scf" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DAD_SP = {
		{ "add", " ", "hl", ", ", "sp" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_LDA_NN = {
		{ "ld", " ", "a", ", ", "(", "NN", ")", },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_IMM, dev::CMD_TT_RPAREN },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_DCX_SP = {
		{ "dec", " ", "sp" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_INR_A = {
		{ "inc", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DCR_A = {
		{ "dec", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MVI_A_N = {
		{ "ld", " ", "a", ", "},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_CMC = {
		{ "ccf" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_B_B = {
		{ "ld", " ", "b", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_B_C = {
		{ "ld", " ", "b", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_B_D = {
		{ "ld", " ", "b", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_B_E = {
		{ "ld", " ", "b", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_B_H = {
		{ "ld", " ", "b", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_B_L = {
		{ "ld", " ", "b", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_B_M = {
		{ "ld", " ", "b", ", ", "(", "hl", ")", },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_B_A = {
		{ "ld", " ", "b", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_C_B = {
		{ "ld", " ", "c", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_C_C = {
		{ "ld", " ", "c", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_C_D = {
		{ "ld", " ", "c", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_C_E = {
		{ "ld", " ", "c", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_C_H = {
		{ "ld", " ", "c", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_C_L = {
		{ "ld", " ", "c", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_C_M = {
		{ "ld", " ", "c", ", ", "(", "hl", ")", },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_C_A = {
		{ "ld", " ", "c", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_D_B = {
		{ "ld", " ", "d", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_D_C = {
		{ "ld", " ", "d", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_D_D = {
		{ "ld", " ", "d", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_D_E = {
		{ "ld", " ", "d", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_D_H = {
		{ "ld", " ", "d", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_D_L = {
		{ "ld", " ", "d", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_D_M = {
		{ "ld", " ", "d", ", ", "(", "hl", ")", },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_D_A = {
		{ "ld", " ", "d", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_E_B = {
		{ "ld", " ", "e", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_E_C = {
		{ "ld", " ", "e", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_E_D = {
		{ "ld", " ", "e", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_E_E = {
		{ "ld", " ", "e", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_E_H = {
		{ "ld", " ", "e", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_E_L = {
		{ "ld", " ", "e", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_E_M = {
		{ "ld", " ", "e", ", ", "(", "hl", ")", },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_E_A = {
		{ "ld", " ", "e", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_H_B = {
		{ "ld", " ", "h", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_H_C = {
		{ "ld", " ", "h", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_H_D = {
		{ "ld", " ", "h", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_H_E = {
		{ "ld", " ", "h", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_H_H = {
		{ "ld", " ", "h", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_H_L = {
		{ "ld", " ", "h", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_H_M = {
		{ "ld", " ", "h", ", ", "(", "hl", ")", },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_H_A = {
		{ "ld", " ", "h", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_L_B = {
		{ "ld", " ", "l", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_L_C = {
		{ "ld", " ", "l", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_L_D = {
		{ "ld", " ", "l", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_L_E = {
		{ "ld", " ", "l", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_L_H = {
		{ "ld", " ", "l", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_L_L = {
		{ "ld", " ", "l", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_L_M = {
		{ "ld", " ", "l", ", ", "(", "hl", ")", },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_L_A = {
		{ "ld", " ", "l", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_M_B = {
		{ "ld", " ", "(", "hl", ")", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_M_C = {
		{ "ld", " ", "(", "hl", ")", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_M_D = {
		{ "ld", " ", "(", "hl", ")", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_M_E = {
		{ "ld", " ", "(", "hl", ")", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_M_H = {
		{ "ld", " ", "(", "hl", ")", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_M_L = {
		{ "ld", " ", "(", "hl", ")", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_HLT = {
		{ "hlt" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_M_A = {
		{ "ld", " ", "(", "hl", ")", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_A_B = {
		{ "ld", " ", "a", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_A_C = {
		{ "ld", " ", "a", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_A_D = {
		{ "ld", " ", "a", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_A_E = {
		{ "ld", " ", "a", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_A_H = {
		{ "ld", " ", "a", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_A_L = {
		{ "ld", " ", "a", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_A_M = {
		{ "ld", " ", "a", ", ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_MOV_A_A = {
		{ "ld", " ", "a", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADD_B = {
		{ "add", " ", "a", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADD_C = {
		{ "add", " ", "a", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADD_D = {
		{ "add", " ", "a", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADD_E = {
		{ "add", " ", "a", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADD_H = {
		{ "add", " ", "a", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADD_L = {
		{ "add", " ", "a", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADD_M = {
		{ "add", " ", "a", ", ", "(", "hl", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADD_A = {
		{ "add", " ", "a", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADC_B = {
		{ "adc", " ", "a", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADC_C = {
		{ "adc", " ", "a", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADC_D = {
		{ "adc", " ", "a", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADC_E = {
		{ "adc", " ", "a", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADC_H = {
		{ "adc", " ", "a", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADC_L = {
		{ "adc", " ", "a", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADC_M = {
		{ "adc", " ", "a", ", ", "(", "hl", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADC_A = {
		{ "adc", " ", "a", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SUB_B = {
		{ "sub", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SUB_C = {
		{ "sub", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SUB_D = {
		{ "sub", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SUB_E = {
		{ "sub", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SUB_H = {
		{ "sub", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SUB_L = {
		{ "sub", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SUB_M = {
		{ "sub", " ", "(", "hl", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SUB_A = {
		{ "sub", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SBB_B = {
		{ "sbc", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SBB_C = {
		{ "sbc", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SBB_D = {
		{ "sbc", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SBB_E = {
		{ "sbc", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SBB_H = {
		{ "sbc", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SBB_L = {
		{ "sbc", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SBB_M = {
		{ "sbc", " ", "(", "hl", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SBB_A = {
		{ "sbc", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ANA_B = {
		{ "and", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ANA_C = {
		{ "and", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ANA_D = {
		{ "and", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ANA_E = {
		{ "and", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ANA_H = {
		{ "and", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ANA_L = {
		{ "and", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ANA_M = {
		{ "and", " ", "(", "hl", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ANA_A = {
		{ "and", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_XRA_B = {
		{ "xor", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_XRA_C = {
		{ "xor", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_XRA_D = {
		{ "xor", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_XRA_E = {
		{ "xor", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_XRA_H = {
		{ "xor", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_XRA_L = {
		{ "xor", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_XRA_M = {
		{ "xor", " ", "(", "hl", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_XRA_A = {
		{ "xor", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ORA_B = {
		{ "or", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ORA_C = {
		{ "or", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ORA_D = {
		{ "or", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ORA_E = {
		{ "or", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ORA_H = {
		{ "or", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ORA_L = {
		{ "or", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ORA_M = {
		{ "or", " ", "(", "hl", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ORA_A = {
		{ "or", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_CMP_B = {
		{ "cp", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_CMP_C = {
		{ "cp", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_CMP_D = {
		{ "cp", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_CMP_E = {
		{ "cp", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_CMP_H = {
		{ "cp", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_CMP_L = {
		{ "cp", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_CMP_M = {
		{ "cp", " ", "(", "hl", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_CMP_A = {
		{ "cp", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_RNZ = {
		{ "ret", " ", "nz" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_POP_B = {
		{ "pop", " ", "bc" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_JNZ_NN = {
		{ "jp", " ", "nz", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_JMP_NN = {
		{ "jp", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};
static const dev::Cmd Z80_CMD_CNZ_NN = {
		{ "call", " ", "nz", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_PUSH_B = {
		{ "push", " ", "bc" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ADI_N = {
		{ "add", " ", "a", ", ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RST_0 = {
		{ "rst", " ", "0" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_RZ = {
		{ "ret", " ", "z" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_RET = {
		{ "ret" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};
static const dev::Cmd Z80_CMD_JZ_NN = {
		{ "jp", " ", "z", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_CZ_NN = {
		{ "call", " ", "z", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_CALL_NN = {
		{ "call", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_ACI_N = {
		{ "adc", " ", "a", ", ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RST_1 = {
		{ "rst", " ", "8" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_RNC = {
		{ "ret", " ", "nc" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_POP_D = {
		{ "pop", " ", "de" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};
static const dev::Cmd Z80_CMD_JNC_NN = {
		{ "jp", " ", "nc", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_OUT_N = {
		{ "out", " ", "(", "N", ")", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_IMM, dev::CMD_TT_RPAREN, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_CNC_NN = {
		{ "call", " ", "nc", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_PUSH_D = {
		{ "push", " ", "de" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SUI_N = {
		{ "sub", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RST_2 = {
		{ "rst", " ", "10h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_RC = {
		{ "ret", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};
static const dev::Cmd Z80_CMD_JC_NN = {
		{ "jp", " ", "c", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_IN_N = {
		{ "in", " ", "a", ", ", "(", "N", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_IMM, dev::CMD_TT_RPAREN },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_CC_NN = {
		{ "call", " ", "c", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_SBI_N = {
		{ "sbc", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RST_3 = {
		{ "rst", " ", "20h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_RPO = {
		{ "ret", " ", "po" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_POP_H = {
		{ "pop", " ", "hl" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_JPO_NN = {
		{ "jp", " ", "po", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_XTHL = {
		{ "ex", " ", "(", "sp", ")", ", ", "hl" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_CPO_NN = {
		{ "call", " ", "po", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_PUSH_H = {
		{ "push", " ", "hl" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ANI_N = {
		{ "and", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RST_4 = {
		{ "rst", " ", "20h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_RPE = {
		{ "ret", " ", "pe"},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG},
		dev::CMD_IT_NONE
};
static const dev::Cmd Z80_CMD_PCHL = {
		{ "jp", " ", "(", "hl", ")" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_LPAREN, dev::CMD_TT_REG, dev::CMD_TT_RPAREN },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_JPE_NN = {
		{ "jp", " ", "pe", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_XCHG = {
		{ "ex", " ", "de", ", ", "hl" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_CPE_NN = {
		{ "call", " ", "pe", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_XRI_N = {
		{ "xor", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RST_5 = {
		{ "rst", " ", "28h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_RP = {
		{ "ret", " ", "p"},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_POP_PSW = {
		{ "pop", " ", "psw" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_JP_NN = {
		{ "jp", " ", "p", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_DI = {
		{ "di" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};
static const dev::Cmd Z80_CMD_CP_NN = {
		{ "call", " ", "p", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_PUSH_PSW = {
		{ "push", " ", "psw" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_ORI_N = {
		{ "ora", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RST_6 = {
		{ "rst", " ", "30h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_RM = {
		{ "ret", " ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_SPHL = {
		{ "ld", " ", "sp", ", ", "hl" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_JM_NN = {
		{ "jp", " ", "m", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_EI = {
		{ "ei" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_CM_NN = {
		{ "call", " ", "m", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd Z80_CMD_CPI_N = {
		{ "cp", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd Z80_CMD_RST_7 = {
		{ "rst", " ", "38h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd Z80_CMD_DB_N = {
		{ "db", " "},
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B0
};

static dev::Cmds cmds_z80 = {
		&Z80_CMD_NOP, /* 0x00 */
		&Z80_CMD_LXI_B_NN, /* 0x01 */
		&Z80_CMD_STAX_B, /* 0x02 */
		&Z80_CMD_INX_B, /* 0x03 */
		&Z80_CMD_INR_B, /* 0x04 */
		&Z80_CMD_DCR_B, /* 0x05 */
		&Z80_CMD_MVI_B_N, /* 0x06 */
		&Z80_CMD_RLC, /* 0x07 */
	&Z80_CMD_DB_N, /* 0x08 */
		&Z80_CMD_DAD_B, /* 0x09 */
		&Z80_CMD_LDAX_B, /* 0x0A */
		&Z80_CMD_DCX_B, /* 0x0B */
		&Z80_CMD_INR_C, /* 0x0C */
		&Z80_CMD_DCR_C, /* 0x0D */
		&Z80_CMD_MVI_C_N, /* 0x0E */
		&Z80_CMD_RRC, /* 0x0F */

	&Z80_CMD_DB_N, /* 0x10 */
		&Z80_CMD_LXI_D, /* 0x11 */
		&Z80_CMD_STAX_D, /* 0x12 */
		&Z80_CMD_INX_D, /* 0x13 */
		&Z80_CMD_INR_D, /* 0x14 */
		&Z80_CMD_DCR_D, /* 0x15 */
		&Z80_CMD_MVI_D_N, /* 0x16 */
		&Z80_CMD_RAL, /* 0x17 */
	&Z80_CMD_DB_N, /* 0x18 */
		&Z80_CMD_DAD_D, /* 0x19 */
		&Z80_CMD_LDAX_D, /* 0x1A */
		&Z80_CMD_DCX_D, /* 0x1B */
		&Z80_CMD_INR_E, /* 0x1C */
		&Z80_CMD_DCR_E, /* 0x1D */
		&Z80_CMD_MVI_E_N, /* 0x1E */
		&Z80_CMD_RAR, /* 0x1F */

	&Z80_CMD_DB_N, /* 0x20 */
		&Z80_CMD_LXI_H, /* 0x21 */
		&Z80_CMD_SHLD_NN, /* 0x22 */
		&Z80_CMD_INX_H, /* 0x23 */
		&Z80_CMD_INR_H, /* 0x24 */
		&Z80_CMD_DCR_H, /* 0x25 */
		&Z80_CMD_MVI_H_N, /* 0x26 */
		&Z80_CMD_DAA, /* 0x27 */
	&Z80_CMD_DB_N, /* 0x28 */
		&Z80_CMD_DAD_H, /* 0x29 */
		&Z80_CMD_LHLD_NN, /* 0x2A */
		&Z80_CMD_DCX_H, /* 0x2B */
		&Z80_CMD_INR_L, /* 0x2C */
		&Z80_CMD_DCR_L, /* 0x2D */
		&Z80_CMD_MVI_L_N, /* 0x2E */
		&Z80_CMD_CMA, /* 0x2F */

	&Z80_CMD_DB_N, /* 0x30 */
		&Z80_CMD_LXI_SP, /* 0x31 */
		&Z80_CMD_STA_NN, /* 0x32 */
		&Z80_CMD_INX_SP, /* 0x33 */
		&Z80_CMD_INR_M, /* 0x34 */
		&Z80_CMD_DCR_M, /* 0x35 */
		&Z80_CMD_MVI_M_N, /* 0x36 */
		&Z80_CMD_STC, /* 0x37 */
	&Z80_CMD_DB_N, /* 0x38 */
		&Z80_CMD_DAD_SP, /* 0x39 */
		&Z80_CMD_LDA_NN, /* 0x3A */
		&Z80_CMD_DCX_SP, /* 0x3B */
		&Z80_CMD_INR_A, /* 0x3C */
		&Z80_CMD_DCR_A, /* 0x3D */
		&Z80_CMD_MVI_A_N, /* 0x3E */
		&Z80_CMD_CMC, /* 0x3F */

		&Z80_CMD_MOV_B_B, /* 0x40 */
		&Z80_CMD_MOV_B_C, /* 0x41 */
		&Z80_CMD_MOV_B_D, /* 0x42 */
		&Z80_CMD_MOV_B_E, /* 0x43 */
		&Z80_CMD_MOV_B_H, /* 0x44 */
		&Z80_CMD_MOV_B_L, /* 0x45 */
		&Z80_CMD_MOV_B_M, /* 0x46 */
		&Z80_CMD_MOV_B_A, /* 0x47 */
		&Z80_CMD_MOV_C_B, /* 0x48 */
		&Z80_CMD_MOV_C_C, /* 0x49 */
		&Z80_CMD_MOV_C_D, /* 0x4A */
		&Z80_CMD_MOV_C_E, /* 0x4B */
		&Z80_CMD_MOV_C_H, /* 0x4C */
		&Z80_CMD_MOV_C_L, /* 0x4D */
		&Z80_CMD_MOV_C_M, /* 0x4E */
		&Z80_CMD_MOV_C_A, /* 0x4F */

		&Z80_CMD_MOV_D_B, /* 0x50 */
		&Z80_CMD_MOV_D_C, /* 0x51 */
		&Z80_CMD_MOV_D_D, /* 0x52 */
		&Z80_CMD_MOV_D_E, /* 0x53 */
		&Z80_CMD_MOV_D_H, /* 0x54 */
		&Z80_CMD_MOV_D_L, /* 0x55 */
		&Z80_CMD_MOV_D_M, /* 0x56 */
		&Z80_CMD_MOV_D_A, /* 0x57 */
		&Z80_CMD_MOV_E_B, /* 0x58 */
		&Z80_CMD_MOV_E_C, /* 0x59 */
		&Z80_CMD_MOV_E_D, /* 0x5A */
		&Z80_CMD_MOV_E_E, /* 0x5B */
		&Z80_CMD_MOV_E_H, /* 0x5C */
		&Z80_CMD_MOV_E_L, /* 0x5D */
		&Z80_CMD_MOV_E_M, /* 0x5E */
		&Z80_CMD_MOV_E_A, /* 0x5F */

		&Z80_CMD_MOV_H_B, /* 0x60 */
		&Z80_CMD_MOV_H_C, /* 0x61 */
		&Z80_CMD_MOV_H_D, /* 0x62 */
		&Z80_CMD_MOV_H_E, /* 0x63 */
		&Z80_CMD_MOV_H_H, /* 0x64 */
		&Z80_CMD_MOV_H_L, /* 0x65 */
		&Z80_CMD_MOV_H_M, /* 0x66 */
		&Z80_CMD_MOV_H_A, /* 0x67 */
		&Z80_CMD_MOV_L_B, /* 0x68 */
		&Z80_CMD_MOV_L_C, /* 0x69 */
		&Z80_CMD_MOV_L_D, /* 0x6A */
		&Z80_CMD_MOV_L_E, /* 0x6B */
		&Z80_CMD_MOV_L_H, /* 0x6C */
		&Z80_CMD_MOV_L_L, /* 0x6D */
		&Z80_CMD_MOV_L_M, /* 0x6E */
		&Z80_CMD_MOV_L_A, /* 0x6F */

		&Z80_CMD_MOV_M_B, /* 0x70 */
		&Z80_CMD_MOV_M_C, /* 0x71 */
		&Z80_CMD_MOV_M_D, /* 0x72 */
		&Z80_CMD_MOV_M_E, /* 0x73 */
		&Z80_CMD_MOV_M_H, /* 0x74 */
		&Z80_CMD_MOV_M_L, /* 0x75 */
		&Z80_CMD_HLT, /* 0x76 */
		&Z80_CMD_MOV_M_A, /* 0x77 */
		&Z80_CMD_MOV_A_B, /* 0x78 */
		&Z80_CMD_MOV_A_C, /* 0x79 */
		&Z80_CMD_MOV_A_D, /* 0x7A */
		&Z80_CMD_MOV_A_E, /* 0x7B */
		&Z80_CMD_MOV_A_H, /* 0x7C */
		&Z80_CMD_MOV_A_L, /* 0x7D */
		&Z80_CMD_MOV_A_M, /* 0x7E */
		&Z80_CMD_MOV_A_A, /* 0x7F */

		&Z80_CMD_ADD_B, /* 0x80 */
		&Z80_CMD_ADD_C, /* 0x81 */
		&Z80_CMD_ADD_D, /* 0x82 */
		&Z80_CMD_ADD_E, /* 0x83 */
		&Z80_CMD_ADD_H, /* 0x84 */
		&Z80_CMD_ADD_L, /* 0x85 */
		&Z80_CMD_ADD_M, /* 0x86 */
		&Z80_CMD_ADD_A, /* 0x87 */
		&Z80_CMD_ADC_B, /* 0x88 */
		&Z80_CMD_ADC_C, /* 0x89 */
		&Z80_CMD_ADC_D, /* 0x8A */
		&Z80_CMD_ADC_E, /* 0x8B */
		&Z80_CMD_ADC_H, /* 0x8C */
		&Z80_CMD_ADC_L, /* 0x8D */
		&Z80_CMD_ADC_M, /* 0x8E */
		&Z80_CMD_ADC_A, /* 0x8F */

		&Z80_CMD_SUB_B, /* 0x90 */
		&Z80_CMD_SUB_C, /* 0x91 */
		&Z80_CMD_SUB_D, /* 0x92 */
		&Z80_CMD_SUB_E, /* 0x93 */
		&Z80_CMD_SUB_H, /* 0x94 */
		&Z80_CMD_SUB_L, /* 0x95 */
		&Z80_CMD_SUB_M, /* 0x96 */
		&Z80_CMD_SUB_A, /* 0x97 */
		&Z80_CMD_SBB_B, /* 0x98 */
		&Z80_CMD_SBB_C, /* 0x99 */
		&Z80_CMD_SBB_D, /* 0x9A */
		&Z80_CMD_SBB_E, /* 0x9B */
		&Z80_CMD_SBB_H, /* 0x9C */
		&Z80_CMD_SBB_L, /* 0x9D */
		&Z80_CMD_SBB_M, /* 0x9E */
		&Z80_CMD_SBB_A, /* 0x9F */

		&Z80_CMD_ANA_B, /* 0xA0 */
		&Z80_CMD_ANA_C, /* 0xA1 */
		&Z80_CMD_ANA_D, /* 0xA2 */
		&Z80_CMD_ANA_E, /* 0xA3 */
		&Z80_CMD_ANA_H, /* 0xA4 */
		&Z80_CMD_ANA_L, /* 0xA5 */
		&Z80_CMD_ANA_M, /* 0xA6 */
		&Z80_CMD_ANA_A, /* 0xA7 */
		&Z80_CMD_XRA_B, /* 0xA8 */
		&Z80_CMD_XRA_C, /* 0xA9 */
		&Z80_CMD_XRA_D, /* 0xAA */
		&Z80_CMD_XRA_E, /* 0xAB */
		&Z80_CMD_XRA_H, /* 0xAC */
		&Z80_CMD_XRA_L, /* 0xAD */
		&Z80_CMD_XRA_M, /* 0xAE */
		&Z80_CMD_XRA_A, /* 0xAF */

		&Z80_CMD_ORA_B, /* 0xB0 */
		&Z80_CMD_ORA_C, /* 0xB1 */
		&Z80_CMD_ORA_D, /* 0xB2 */
		&Z80_CMD_ORA_E, /* 0xB3 */
		&Z80_CMD_ORA_H, /* 0xB4 */
		&Z80_CMD_ORA_L, /* 0xB5 */
		&Z80_CMD_ORA_M, /* 0xB6 */
		&Z80_CMD_ORA_A, /* 0xB7 */
		&Z80_CMD_CMP_B, /* 0xB8 */
		&Z80_CMD_CMP_C, /* 0xB9 */
		&Z80_CMD_CMP_D, /* 0xBA */
		&Z80_CMD_CMP_E, /* 0xBB */
		&Z80_CMD_CMP_H, /* 0xBC */
		&Z80_CMD_CMP_L, /* 0xBD */
		&Z80_CMD_CMP_M, /* 0xBE */
		&Z80_CMD_CMP_A, /* 0xBF */

		&Z80_CMD_RNZ, /* 0xC0 */
		&Z80_CMD_POP_B, /* 0xC1 */
		&Z80_CMD_JNZ_NN, /* 0xC2 */
		&Z80_CMD_JMP_NN, /* 0xC3 */
		&Z80_CMD_CNZ_NN, /* 0xC4 */
		&Z80_CMD_PUSH_B, /* 0xC5 */
		&Z80_CMD_ADI_N, /* 0xC6 */
		&Z80_CMD_RST_0, /* 0xC7 */
		&Z80_CMD_RZ, /* 0xC8 */
		&Z80_CMD_RET, /* 0xC9 */
		&Z80_CMD_JZ_NN, /* 0xCA */
	&Z80_CMD_DB_N, /* 0xCB */
		&Z80_CMD_CZ_NN, /* 0xCC */
		&Z80_CMD_CALL_NN, /* 0xCD */
		&Z80_CMD_ACI_N, /* 0xCE */
		&Z80_CMD_RST_1, /* 0xCF */

		&Z80_CMD_RNC, /* 0xD0 */
		&Z80_CMD_POP_D, /* 0xD1 */
		&Z80_CMD_JNC_NN, /* 0xD2 */
		&Z80_CMD_OUT_N, /* 0xD3 */
		&Z80_CMD_CNC_NN, /* 0xD4 */
		&Z80_CMD_PUSH_D, /* 0xD5 */
		&Z80_CMD_SUI_N, /* 0xD6 */
		&Z80_CMD_RST_2, /* 0xD7 */
		&Z80_CMD_RC, /* 0xD8 */
	&Z80_CMD_DB_N, /* 0xD9 */
		&Z80_CMD_JC_NN, /* 0xDA */
		&Z80_CMD_IN_N, /* 0xDB */
		&Z80_CMD_CC_NN, /* 0xDC */
	&Z80_CMD_DB_N, /* 0xDD */
		&Z80_CMD_SBI_N, /* 0xDE */
		&Z80_CMD_RST_3, /* 0xDF */

		&Z80_CMD_RPO, /* 0xE0 */
		&Z80_CMD_POP_H, /* 0xE1 */
		&Z80_CMD_JPO_NN, /* 0xE2 */
		&Z80_CMD_XTHL, /* 0xE3 */
		&Z80_CMD_CPO_NN, /* 0xE4 */
		&Z80_CMD_PUSH_H, /* 0xE5 */
		&Z80_CMD_ANI_N, /* 0xE6 */
		&Z80_CMD_RST_4, /* 0xE7 */
		&Z80_CMD_RPE, /* 0xE8 */
		&Z80_CMD_PCHL, /* 0xE9 */
		&Z80_CMD_JPE_NN, /* 0xEA */
		&Z80_CMD_XCHG, /* 0xEB */
		&Z80_CMD_CPE_NN, /* 0xEC */
	&Z80_CMD_DB_N, /* 0xED */
		&Z80_CMD_XRI_N, /* 0xEE */
		&Z80_CMD_RST_5, /* 0xEF */

		&Z80_CMD_RP, /* 0xF0 */
		&Z80_CMD_POP_PSW, /* 0xF1 */
		&Z80_CMD_JP_NN, /* 0xF2 */
		&Z80_CMD_DI, /* 0xF3 */
		&Z80_CMD_CP_NN, /* 0xF4 */
		&Z80_CMD_PUSH_PSW, /* 0xF5 */
		&Z80_CMD_ORI_N, /* 0xF6 */
		&Z80_CMD_RST_6, /* 0xF7 */
		&Z80_CMD_RM, /* 0xF8 */
		&Z80_CMD_SPHL, /* 0xF9 */
		&Z80_CMD_JM_NN, /* 0xFA */
		&Z80_CMD_EI, /* 0xFB */
		&Z80_CMD_CM_NN, /* 0xFC */
	&Z80_CMD_DB_N, /* 0xFD */
		&Z80_CMD_CPI_N, /* 0xFE */
		&Z80_CMD_RST_7 /* 0xFF */
	};
