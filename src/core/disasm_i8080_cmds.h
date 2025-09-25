#include "core/disasm.h"

static const dev::Cmd CMD_NOP = {
		{ "nop" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_LXI_B_NN = {
		{ "lxi", " ", "b", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_STAX_B = {
		{ "stax", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_INX_B = {
		{ "inx", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_INR_B = {
		{ "inr", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DCR_B = {
		{ "dcr", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MVI_B_N = {
		{ "mvi", " ", "b", ", ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RLC = {
		{ "rlc" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DAD_B = {
		{ "dad", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_LDAX_B = {
		{ "ldax", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DCX_B = {
		{ "dcx", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_INR_C = {
		{ "inr", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DCR_C = {
		{ "dcr", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MVI_C_N = {
		{ "mvi", " ", "c", ", ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RRC = {
		{ "rrc" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_LXI_D = {
		{ "lxi", " ", "d", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_STAX_D = {
		{ "stax", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_INX_D = {
		{ "inx", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_INR_D = {
		{ "inr", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DCR_D = {
		{ "dcr", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MVI_D_N = {
		{ "mvi", " ", "d", ", ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RAL = {
		{ "ral" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DAD_D = {
		{ "dad", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_LDAX_D = {
		{ "ldax", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DCX_D = {
		{ "dcx", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_INR_E = {
		{ "inr", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DCR_E = {
		{ "dcr", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MVI_E_N = {
		{ "mvi", " ", "e", ", ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RAR = {
		{ "rar" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_LXI_H = {
		{ "lxi", " ", "h", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_SHLD_NN = {
		{ "shld", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_INX_H = {
		{ "inx", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_INR_H = {
		{ "inr", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DCR_H = {
		{ "dcr", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MVI_H_N = {
		{ "mvi", " ", "h", ", ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_DAA = {
		{ "daa" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DAD_H = {
		{ "dad", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_LHLD_NN = {
		{ "lhld", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_DCX_H = {
		{ "dcx", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_INR_L = {
		{ "inr", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DCR_L = {
		{ "dcr", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MVI_L_N = {
		{ "mvi", " ", "l", ", ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_CMA = {
		{ "cma" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_LXI_SP = {
		{ "lxi", " ", "sp", ", ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_STA_NN = {
		{ "sta", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_INX_SP = {
		{ "inx", " ", "sp" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_INR_M = {
		{ "inr", " ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DCR_M = {
		{ "dcr", " ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MVI_M_N = {
		{ "mvi", " ", "m", ", ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_STC = {
		{ "stc" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DAD_SP = {
		{ "dad", " ", "sp" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_LDA_NN = {
		{ "lda", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_DCX_SP = {
		{ "dcx", " ", "sp" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_INR_A = {
		{ "inr", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DCR_A = {
		{ "dcr", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MVI_A_N = {
		{ "mvi", " ", "a", ", ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_CMC = {
		{ "cmc" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_B_B = {
		{ "mov", " ", "b", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_B_C = {
		{ "mov", " ", "b", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_B_D = {
		{ "mov", " ", "b", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_B_E = {
		{ "mov", " ", "b", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_B_H = {
		{ "mov", " ", "b", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_B_L = {
		{ "mov", " ", "b", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_B_M = {
		{ "mov", " ", "b", ", ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_B_A = {
		{ "mov", " ", "b", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_C_B = {
		{ "mov", " ", "c", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_C_C = {
		{ "mov", " ", "c", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_C_D = {
		{ "mov", " ", "c", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_C_E = {
		{ "mov", " ", "c", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_C_H = {
		{ "mov", " ", "c", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_C_L = {
		{ "mov", " ", "c", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_C_M = {
		{ "mov", " ", "c", ", ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_C_A = {
		{ "mov", " ", "c", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_D_B = {
		{ "mov", " ", "d", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_D_C = {
		{ "mov", " ", "d", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_D_D = {
		{ "mov", " ", "d", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_D_E = {
		{ "mov", " ", "d", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_D_H = {
		{ "mov", " ", "d", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_D_L = {
		{ "mov", " ", "d", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_D_M = {
		{ "mov", " ", "d", ", ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_D_A = {
		{ "mov", " ", "d", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_E_B = {
		{ "mov", " ", "e", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_E_C = {
		{ "mov", " ", "e", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_E_D = {
		{ "mov", " ", "e", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_E_E = {
		{ "mov", " ", "e", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_E_H = {
		{ "mov", " ", "e", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_E_L = {
		{ "mov", " ", "e", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_E_M = {
		{ "mov", " ", "e", ", ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_E_A = {
		{ "mov", " ", "e", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_H_B = {
		{ "mov", " ", "h", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_H_C = {
		{ "mov", " ", "h", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_H_D = {
		{ "mov", " ", "h", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_H_E = {
		{ "mov", " ", "h", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_H_H = {
		{ "mov", " ", "h", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_H_L = {
		{ "mov", " ", "h", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_H_M = {
		{ "mov", " ", "h", ", ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_H_A = {
		{ "mov", " ", "h", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_L_B = {
		{ "mov", " ", "l", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_L_C = {
		{ "mov", " ", "l", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_L_D = {
		{ "mov", " ", "l", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_L_E = {
		{ "mov", " ", "l", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_L_H = {
		{ "mov", " ", "l", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_L_L = {
		{ "mov", " ", "l", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_L_M = {
		{ "mov", " ", "l", ", ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_L_A = {
		{ "mov", " ", "l", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_M_B = {
		{ "mov", " ", "m", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_M_C = {
		{ "mov", " ", "m", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_M_D = {
		{ "mov", " ", "m", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_M_E = {
		{ "mov", " ", "m", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_M_H = {
		{ "mov", " ", "m", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_M_L = {
		{ "mov", " ", "m", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_HLT = {
		{ "hlt" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_M_A = {
		{ "mov", " ", "m", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_A_B = {
		{ "mov", " ", "a", ", ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_A_C = {
		{ "mov", " ", "a", ", ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_A_D = {
		{ "mov", " ", "a", ", ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_A_E = {
		{ "mov", " ", "a", ", ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_A_H = {
		{ "mov", " ", "a", ", ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_A_L = {
		{ "mov", " ", "a", ", ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_A_M = {
		{ "mov", " ", "a", ", ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_MOV_A_A = {
		{ "mov", " ", "a", ", ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG, dev::CMD_TT_COMMA_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADD_B = {
		{ "add", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADD_C = {
		{ "add", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADD_D = {
		{ "add", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADD_E = {
		{ "add", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADD_H = {
		{ "add", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADD_L = {
		{ "add", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADD_M = {
		{ "add", " ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADD_A = {
		{ "add", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADC_B = {
		{ "adc", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADC_C = {
		{ "adc", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADC_D = {
		{ "adc", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADC_E = {
		{ "adc", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADC_H = {
		{ "adc", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADC_L = {
		{ "adc", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADC_M = {
		{ "adc", " ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADC_A = {
		{ "adc", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SUB_B = {
		{ "sub", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SUB_C = {
		{ "sub", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SUB_D = {
		{ "sub", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SUB_E = {
		{ "sub", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SUB_H = {
		{ "sub", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SUB_L = {
		{ "sub", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SUB_M = {
		{ "sub", " ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SUB_A = {
		{ "sub", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SBB_B = {
		{ "sbb", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SBB_C = {
		{ "sbb", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SBB_D = {
		{ "sbb", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SBB_E = {
		{ "sbb", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SBB_H = {
		{ "sbb", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SBB_L = {
		{ "sbb", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SBB_M = {
		{ "sbb", " ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SBB_A = {
		{ "sbb", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ANA_B = {
		{ "ana", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ANA_C = {
		{ "ana", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ANA_D = {
		{ "ana", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ANA_E = {
		{ "ana", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ANA_H = {
		{ "ana", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ANA_L = {
		{ "ana", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ANA_M = {
		{ "ana", " ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ANA_A = {
		{ "ana", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_XRA_B = {
		{ "xra", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_XRA_C = {
		{ "xra", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_XRA_D = {
		{ "xra", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_XRA_E = {
		{ "xra", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_XRA_H = {
		{ "xra", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_XRA_L = {
		{ "xra", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_XRA_M = {
		{ "xra", " ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_XRA_A = {
		{ "xra", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ORA_B = {
		{ "ora", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ORA_C = {
		{ "ora", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ORA_D = {
		{ "ora", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ORA_E = {
		{ "ora", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ORA_H = {
		{ "ora", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ORA_L = {
		{ "ora", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ORA_M = {
		{ "ora", " ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ORA_A = {
		{ "ora", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_CMP_B = {
		{ "cmp", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_CMP_C = {
		{ "cmp", " ", "c" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_CMP_D = {
		{ "cmp", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_CMP_E = {
		{ "cmp", " ", "e" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_CMP_H = {
		{ "cmp", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_CMP_L = {
		{ "cmp", " ", "l" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_CMP_M = {
		{ "cmp", " ", "m" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_CMP_A = {
		{ "cmp", " ", "a" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_RNZ = {
		{ "rnz" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_POP_B = {
		{ "pop", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_JNZ_NN = {
		{ "jnz", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_JMP_NN = {
		{ "jmp", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};
static const dev::Cmd CMD_CNZ_NN = {
		{ "cnz", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_PUSH_B = {
		{ "push", " ", "b" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ADI_N = {
		{ "adi", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RST_0 = {
		{ "rst", " ", "0" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_RZ = {
		{ "rz" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_RET = {
		{ "ret" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};
static const dev::Cmd CMD_JZ_NN = {
		{ "jz", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_CZ_NN = {
		{ "cz", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_CALL_NN = {
		{ "call", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_ACI_N = {
		{ "aci", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RST_1 = {
		{ "rst", " ", "1" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_RNC = {
		{ "rnc" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_POP_D = {
		{ "pop", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};
static const dev::Cmd CMD_JNC_NN = {
		{ "jnc", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_OUT_N = {
		{ "out", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_CNC_NN = {
		{ "cnc", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_PUSH_D = {
		{ "push", " ", "d" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SUI_N = {
		{ "sui", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RST_2 = {
		{ "rst", " ", "2" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_RC = {
		{ "rc" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};
static const dev::Cmd CMD_JC_NN = {
		{ "jc", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_IN_N = {
		{ "in", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_CC_NN = {
		{ "cc", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_SBI_N = {
		{ "sbi", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RST_3 = {
		{ "rst", " ", "3" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_RPO = {
		{ "rpo" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_POP_H = {
		{ "pop", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_JPO_NN = {
		{ "jpo", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_XTHL = {
		{ "xthl" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_CPO_NN = {
		{ "cpo", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_PUSH_H = {
		{ "push", " ", "h" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ANI_N = {
		{ "ani", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RST_4 = {
		{ "rst", " ", "4" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_RPE = {
		{ "rpe" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};
static const dev::Cmd CMD_PCHL = {
		{ "pchl" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_JPE_NN = {
		{ "jpe", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_XCHG = {
		{ "xchg" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_CPE_NN = {
		{ "cpe", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_XRI_N = {
		{ "xri", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RST_5 = {
		{ "rst", " ", "5" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_RP = {
		{ "rp" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_POP_PSW = {
		{ "pop", " ", "psw" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_JP_NN = {
		{ "jp", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_DI = {
		{ "di" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};
static const dev::Cmd CMD_CP_NN = {
		{ "cp", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_PUSH_PSW = {
		{ "push", " ", "psw" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_REG },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_ORI_N = {
		{ "ori", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RST_6 = {
		{ "rst", " ", "6" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_RM = {
		{ "rm" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_SPHL = {
		{ "sphl" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_JM_NN = {
		{ "jm", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_EI = {
		{ "ei" },
		{ dev::CMD_TT_CMD },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_CM_NN = {
		{ "cm", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_W1
};

static const dev::Cmd CMD_CPI_N = {
		{ "cpi", " ", "NN" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B1
};

static const dev::Cmd CMD_RST_7 = {
		{ "rst", " ", "7" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_BIMM },
		dev::CMD_IT_NONE
};

static const dev::Cmd CMD_DB_N = {
		{ "db", " ", "N" },
		{ dev::CMD_TT_CMD, dev::CMD_TT_SPACE, dev::CMD_TT_IMM },
		dev::CMD_IT_B0
};

static dev::Cmds cmds_i8080 = {
		&CMD_NOP, /* 0x00 */
		&CMD_LXI_B_NN, /* 0x01 */
		&CMD_STAX_B, /* 0x02 */
		&CMD_INX_B, /* 0x03 */
		&CMD_INR_B, /* 0x04 */
		&CMD_DCR_B, /* 0x05 */
		&CMD_MVI_B_N, /* 0x06 */
		&CMD_RLC, /* 0x07 */
	&CMD_DB_N, /* 0x08 */
		&CMD_DAD_B, /* 0x09 */
		&CMD_LDAX_B, /* 0x0A */
		&CMD_DCX_B, /* 0x0B */
		&CMD_INR_C, /* 0x0C */
		&CMD_DCR_C, /* 0x0D */
		&CMD_MVI_C_N, /* 0x0E */
		&CMD_RRC, /* 0x0F */

	&CMD_DB_N, /* 0x10 */
		&CMD_LXI_D, /* 0x11 */
		&CMD_STAX_D, /* 0x12 */
		&CMD_INX_D, /* 0x13 */
		&CMD_INR_D, /* 0x14 */
		&CMD_DCR_D, /* 0x15 */
		&CMD_MVI_D_N, /* 0x16 */
		&CMD_RAL, /* 0x17 */
	&CMD_DB_N, /* 0x18 */
		&CMD_DAD_D, /* 0x19 */
		&CMD_LDAX_D, /* 0x1A */
		&CMD_DCX_D, /* 0x1B */
		&CMD_INR_E, /* 0x1C */
		&CMD_DCR_E, /* 0x1D */
		&CMD_MVI_E_N, /* 0x1E */
		&CMD_RAR, /* 0x1F */

	&CMD_DB_N, /* 0x20 */
		&CMD_LXI_H, /* 0x21 */
		&CMD_SHLD_NN, /* 0x22 */
		&CMD_INX_H, /* 0x23 */
		&CMD_INR_H, /* 0x24 */
		&CMD_DCR_H, /* 0x25 */
		&CMD_MVI_H_N, /* 0x26 */
		&CMD_DAA, /* 0x27 */
	&CMD_DB_N, /* 0x28 */
		&CMD_DAD_H, /* 0x29 */
		&CMD_LHLD_NN, /* 0x2A */
		&CMD_DCX_H, /* 0x2B */
		&CMD_INR_L, /* 0x2C */
		&CMD_DCR_L, /* 0x2D */
		&CMD_MVI_L_N, /* 0x2E */
		&CMD_CMA, /* 0x2F */

	&CMD_DB_N, /* 0x30 */
		&CMD_LXI_SP, /* 0x31 */
		&CMD_STA_NN, /* 0x32 */
		&CMD_INX_SP, /* 0x33 */
		&CMD_INR_M, /* 0x34 */
		&CMD_DCR_M, /* 0x35 */
		&CMD_MVI_M_N, /* 0x36 */
		&CMD_STC, /* 0x37 */
	&CMD_DB_N, /* 0x38 */
		&CMD_DAD_SP, /* 0x39 */
		&CMD_LDA_NN, /* 0x3A */
		&CMD_DCX_SP, /* 0x3B */
		&CMD_INR_A, /* 0x3C */
		&CMD_DCR_A, /* 0x3D */
		&CMD_MVI_A_N, /* 0x3E */
		&CMD_CMC, /* 0x3F */

		&CMD_MOV_B_B, /* 0x40 */
		&CMD_MOV_B_C, /* 0x41 */
		&CMD_MOV_B_D, /* 0x42 */
		&CMD_MOV_B_E, /* 0x43 */
		&CMD_MOV_B_H, /* 0x44 */
		&CMD_MOV_B_L, /* 0x45 */
		&CMD_MOV_B_M, /* 0x46 */
		&CMD_MOV_B_A, /* 0x47 */
		&CMD_MOV_C_B, /* 0x48 */
		&CMD_MOV_C_C, /* 0x49 */
		&CMD_MOV_C_D, /* 0x4A */
		&CMD_MOV_C_E, /* 0x4B */
		&CMD_MOV_C_H, /* 0x4C */
		&CMD_MOV_C_L, /* 0x4D */
		&CMD_MOV_C_M, /* 0x4E */
		&CMD_MOV_C_A, /* 0x4F */

		&CMD_MOV_D_B, /* 0x50 */
		&CMD_MOV_D_C, /* 0x51 */
		&CMD_MOV_D_D, /* 0x52 */
		&CMD_MOV_D_E, /* 0x53 */
		&CMD_MOV_D_H, /* 0x54 */
		&CMD_MOV_D_L, /* 0x55 */
		&CMD_MOV_D_M, /* 0x56 */
		&CMD_MOV_D_A, /* 0x57 */
		&CMD_MOV_E_B, /* 0x58 */
		&CMD_MOV_E_C, /* 0x59 */
		&CMD_MOV_E_D, /* 0x5A */
		&CMD_MOV_E_E, /* 0x5B */
		&CMD_MOV_E_H, /* 0x5C */
		&CMD_MOV_E_L, /* 0x5D */
		&CMD_MOV_E_M, /* 0x5E */
		&CMD_MOV_E_A, /* 0x5F */

		&CMD_MOV_H_B, /* 0x60 */
		&CMD_MOV_H_C, /* 0x61 */
		&CMD_MOV_H_D, /* 0x62 */
		&CMD_MOV_H_E, /* 0x63 */
		&CMD_MOV_H_H, /* 0x64 */
		&CMD_MOV_H_L, /* 0x65 */
		&CMD_MOV_H_M, /* 0x66 */
		&CMD_MOV_H_A, /* 0x67 */
		&CMD_MOV_L_B, /* 0x68 */
		&CMD_MOV_L_C, /* 0x69 */
		&CMD_MOV_L_D, /* 0x6A */
		&CMD_MOV_L_E, /* 0x6B */
		&CMD_MOV_L_H, /* 0x6C */
		&CMD_MOV_L_L, /* 0x6D */
		&CMD_MOV_L_M, /* 0x6E */
		&CMD_MOV_L_A, /* 0x6F */

		&CMD_MOV_M_B, /* 0x70 */
		&CMD_MOV_M_C, /* 0x71 */
		&CMD_MOV_M_D, /* 0x72 */
		&CMD_MOV_M_E, /* 0x73 */
		&CMD_MOV_M_H, /* 0x74 */
		&CMD_MOV_M_L, /* 0x75 */
		&CMD_HLT, /* 0x76 */
		&CMD_MOV_M_A, /* 0x77 */
		&CMD_MOV_A_B, /* 0x78 */
		&CMD_MOV_A_C, /* 0x79 */
		&CMD_MOV_A_D, /* 0x7A */
		&CMD_MOV_A_E, /* 0x7B */
		&CMD_MOV_A_H, /* 0x7C */
		&CMD_MOV_A_L, /* 0x7D */
		&CMD_MOV_A_M, /* 0x7E */
		&CMD_MOV_A_A, /* 0x7F */

		&CMD_ADD_B, /* 0x80 */
		&CMD_ADD_C, /* 0x81 */
		&CMD_ADD_D, /* 0x82 */
		&CMD_ADD_E, /* 0x83 */
		&CMD_ADD_H, /* 0x84 */
		&CMD_ADD_L, /* 0x85 */
		&CMD_ADD_M, /* 0x86 */
		&CMD_ADD_A, /* 0x87 */
		&CMD_ADC_B, /* 0x88 */
		&CMD_ADC_C, /* 0x89 */
		&CMD_ADC_D, /* 0x8A */
		&CMD_ADC_E, /* 0x8B */
		&CMD_ADC_H, /* 0x8C */
		&CMD_ADC_L, /* 0x8D */
		&CMD_ADC_M, /* 0x8E */
		&CMD_ADC_A, /* 0x8F */

		&CMD_SUB_B, /* 0x90 */
		&CMD_SUB_C, /* 0x91 */
		&CMD_SUB_D, /* 0x92 */
		&CMD_SUB_E, /* 0x93 */
		&CMD_SUB_H, /* 0x94 */
		&CMD_SUB_L, /* 0x95 */
		&CMD_SUB_M, /* 0x96 */
		&CMD_SUB_A, /* 0x97 */
		&CMD_SBB_B, /* 0x98 */
		&CMD_SBB_C, /* 0x99 */
		&CMD_SBB_D, /* 0x9A */
		&CMD_SBB_E, /* 0x9B */
		&CMD_SBB_H, /* 0x9C */
		&CMD_SBB_L, /* 0x9D */
		&CMD_SBB_M, /* 0x9E */
		&CMD_SBB_A, /* 0x9F */

		&CMD_ANA_B, /* 0xA0 */
		&CMD_ANA_C, /* 0xA1 */
		&CMD_ANA_D, /* 0xA2 */
		&CMD_ANA_E, /* 0xA3 */
		&CMD_ANA_H, /* 0xA4 */
		&CMD_ANA_L, /* 0xA5 */
		&CMD_ANA_M, /* 0xA6 */
		&CMD_ANA_A, /* 0xA7 */
		&CMD_XRA_B, /* 0xA8 */
		&CMD_XRA_C, /* 0xA9 */
		&CMD_XRA_D, /* 0xAA */
		&CMD_XRA_E, /* 0xAB */
		&CMD_XRA_H, /* 0xAC */
		&CMD_XRA_L, /* 0xAD */
		&CMD_XRA_M, /* 0xAE */
		&CMD_XRA_A, /* 0xAF */

		&CMD_ORA_B, /* 0xB0 */
		&CMD_ORA_C, /* 0xB1 */
		&CMD_ORA_D, /* 0xB2 */
		&CMD_ORA_E, /* 0xB3 */
		&CMD_ORA_H, /* 0xB4 */
		&CMD_ORA_L, /* 0xB5 */
		&CMD_ORA_M, /* 0xB6 */
		&CMD_ORA_A, /* 0xB7 */
		&CMD_CMP_B, /* 0xB8 */
		&CMD_CMP_C, /* 0xB9 */
		&CMD_CMP_D, /* 0xBA */
		&CMD_CMP_E, /* 0xBB */
		&CMD_CMP_H, /* 0xBC */
		&CMD_CMP_L, /* 0xBD */
		&CMD_CMP_M, /* 0xBE */
		&CMD_CMP_A, /* 0xBF */

		&CMD_RNZ, /* 0xC0 */
		&CMD_POP_B, /* 0xC1 */
		&CMD_JNZ_NN, /* 0xC2 */
		&CMD_JMP_NN, /* 0xC3 */
		&CMD_CNZ_NN, /* 0xC4 */
		&CMD_PUSH_B, /* 0xC5 */
		&CMD_ADI_N, /* 0xC6 */
		&CMD_RST_0, /* 0xC7 */
		&CMD_RZ, /* 0xC8 */
		&CMD_RET, /* 0xC9 */
		&CMD_JZ_NN, /* 0xCA */
	&CMD_DB_N, /* 0xCB */
		&CMD_CZ_NN, /* 0xCC */
		&CMD_CALL_NN, /* 0xCD */
		&CMD_ACI_N, /* 0xCE */
		&CMD_RST_1, /* 0xCF */

		&CMD_RNC, /* 0xD0 */
		&CMD_POP_D, /* 0xD1 */
		&CMD_JNC_NN, /* 0xD2 */
		&CMD_OUT_N, /* 0xD3 */
		&CMD_CNC_NN, /* 0xD4 */
		&CMD_PUSH_D, /* 0xD5 */
		&CMD_SUI_N, /* 0xD6 */
		&CMD_RST_2, /* 0xD7 */
		&CMD_RC, /* 0xD8 */
	&CMD_DB_N, /* 0xD9 */
		&CMD_JC_NN, /* 0xDA */
		&CMD_IN_N, /* 0xDB */
		&CMD_CC_NN, /* 0xDC */
	&CMD_DB_N, /* 0xDD */
		&CMD_SBI_N, /* 0xDE */
		&CMD_RST_3, /* 0xDF */

		&CMD_RPO, /* 0xE0 */
		&CMD_POP_H, /* 0xE1 */
		&CMD_JPO_NN, /* 0xE2 */
		&CMD_XTHL, /* 0xE3 */
		&CMD_CPO_NN, /* 0xE4 */
		&CMD_PUSH_H, /* 0xE5 */
		&CMD_ANI_N, /* 0xE6 */
		&CMD_RST_4, /* 0xE7 */
		&CMD_RPE, /* 0xE8 */
		&CMD_PCHL, /* 0xE9 */
		&CMD_JPE_NN, /* 0xEA */
		&CMD_XCHG, /* 0xEB */
		&CMD_CPE_NN, /* 0xEC */
	&CMD_DB_N, /* 0xED */
		&CMD_XRI_N, /* 0xEE */
		&CMD_RST_5, /* 0xEF */

		&CMD_RP, /* 0xF0 */
		&CMD_POP_PSW, /* 0xF1 */
		&CMD_JP_NN, /* 0xF2 */
		&CMD_DI, /* 0xF3 */
		&CMD_CP_NN, /* 0xF4 */
		&CMD_PUSH_PSW, /* 0xF5 */
		&CMD_ORI_N, /* 0xF6 */
		&CMD_RST_6, /* 0xF7 */
		&CMD_RM, /* 0xF8 */
		&CMD_SPHL, /* 0xF9 */
		&CMD_JM_NN, /* 0xFA */
		&CMD_EI, /* 0xFB */
		&CMD_CM_NN, /* 0xFC */
	&CMD_DB_N, /* 0xFD */
		&CMD_CPI_N, /* 0xFE */
		&CMD_RST_7 /* 0xFF */
	};
