#include "Disasm.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>
#include "Utils/StrUtils.h"
#include "Utils/Utils.h"

#define DISASM_CMDS			0x100

static const char* MN_MOV	= "mov";
static const char* MN_MVI	= "mvi";
static const char* MN_LDAX	= "ldax";
static const char* MN_STAX	= "stax";
static const char* MN_LDA	= "lda";
static const char* MN_STA	= "sta";
static const char* MN_LXI	= "lxi";
static const char* MN_LHLD	= "lhld";
static const char* MN_SHLD	= "shld";
static const char* MN_POP	= "pop";
static const char* MN_PUSH	= "push";
static const char* MN_SPHL	= "sphl";
static const char* MN_XCHG	= "xchg";
static const char* MN_XTHL	= "xthl";

static const char* MN_CMC	= "cmc";
static const char* MN_STC	= "stc";
static const char* MN_CMA	= "cma";
static const char* MN_DAA	= "daa";

static const char* MN_RRC	= "rrc";
static const char* MN_RLC	= "rlc";
static const char* MN_RAL	= "ral";
static const char* MN_RAR	= "rar";

static const char* MN_INR	= "inr";
static const char* MN_INX	= "inx";
static const char* MN_DCR	= "dcr";
static const char* MN_DCX	= "dcx";

static const char* MN_ADD	= "add";
static const char* MN_SUB	= "sub";
static const char* MN_ADC	= "adc";
static const char* MN_SBB	= "sbb";
static const char* MN_ANA	= "ana";
static const char* MN_ORA	= "ora";
static const char* MN_XRA	= "xra";
static const char* MN_CMP	= "cmp";

static const char* MN_ADI	= "adi";
static const char* MN_SUI	= "sui";
static const char* MN_ACI	= "aci";
static const char* MN_SBI	= "sbi";
static const char* MN_ANI	= "ani";
static const char* MN_ORI	= "ori";
static const char* MN_XRI	= "xri";
static const char* MN_CPI	= "cpi";
static const char* MN_DAD	= "dad";

static const char* MN_NOP	= "nop";

static const char* MN_EI	= "ei";
static const char* MN_DI	= "di";
static const char* MN_HLT	= "HLT";

static const char* MN_IN	= "in";
static const char* MN_OUT	= "out";

static const char* MN_RST	= "rst";
static const char* MN_PCHL	= "pchl";
static const char* MN_JMP	= "jmp";
static const char* MN_CALL	= "call";
static const char* MN_RET	= "ret";
static const char* MN_RNZ	= "rnz";
static const char* MN_JNZ	= "jnz";
static const char* MN_CNZ	= "cnz";
static const char* MN_RZ	= "rz";
static const char* MN_JZ	= "jz";
static const char* MN_CZ	= "cz";
static const char* MN_RNC	= "rnc";
static const char* MN_JNC	= "jnc";
static const char* MN_CNC	= "cnc";
static const char* MN_RC	= "rc";
static const char* MN_JC	= "jc";
static const char* MN_CC	= "cc";
static const char* MN_RPO	= "rpo";
static const char* MN_JPO	= "jpo";
static const char* MN_CPO	= "cpo";
static const char* MN_RPE	= "rpe";
static const char* MN_JPE	= "jpe";
static const char* MN_CPE	= "cpe";
static const char* MN_RP	= "rp";
static const char* MN_JP	= "jp";
static const char* MN_CP	= "cp";
static const char* MN_RM	= "rm";
static const char* MN_JM	= "jm";
static const char* MN_CM	= "cm";

static const char* MN_DB	= "db";

static const char* MN_A	= "a";
static const char* MN_B	= "b";
static const char* MN_C	= "c";
static const char* MN_D	= "d";
static const char* MN_E	= "e";
static const char* MN_H	= "h";
static const char* MN_L	= "l";
static const char* MN_M	= "m";
static const char* MN_PSW	= "psw";
static const char* MN_BC	= "bc";
static const char* MN_DE	= "de";
static const char* MN_HL	= "hl";
static const char* MN_SP	= "sp";
static const char* MN_10	= "0x10";
static const char* MN_20	= "0x20";
static const char* MN_30	= "0x30";
static const char* MN_08	= "0x08";
static const char* MN_18	= "0x18";
static const char* MN_28	= "0x28";
static const char* MN_38	= "0x38";
static const char* MN_CB	= "0xCB";
static const char* MN_D9	= "0xD9";
static const char* MN_DD	= "0xDD";
static const char* MN_ED	= "0xED";
static const char* MN_FD	= "0xFD";
static const char* MN_0	= "0";
static const char* MN_1	= "1";
static const char* MN_2	= "2";
static const char* MN_3	= "3";
static const char* MN_4	= "4";
static const char* MN_5	= "5";
static const char* MN_6	= "6";
static const char* MN_7	= "7";

static const char* DB_[1] = { MN_DB };

static const char* NOP[1]		= {MN_NOP};
static const char* LXI_B[2]		= {MN_LXI, MN_B};
static const char* STAX_B[2]	= {MN_STAX, MN_B};
static const char* INX_B[2]		= {MN_INX, MN_B};
static const char* INR_B[2]		= {MN_INR, MN_B};
static const char* DCR_B[2]		= {MN_DCR, MN_B};
static const char* MVI_B[2]		= {MN_MVI, MN_B};
static const char* RLC[1]		= {MN_RLC};
static const char* DB_8[2]		= {MN_DB, MN_08};
static const char* DAD_B[2]		= {MN_DAD, MN_B};
static const char* LDAX_B[2]	= {MN_LDAX, MN_B};
static const char* DCX_B[2]		= {MN_DCX, MN_B};
static const char* INR_C[2]		= {MN_INR, MN_C};
static const char* DCR_C[2]		= {MN_DCR, MN_C};
static const char* MVI_C[2]		= {MN_MVI, MN_C};
static const char* RRC[1]		= {MN_RRC};
static const char* DB_10[2]		= {MN_DB, MN_10};
static const char* LXI_D[2]		= {MN_LXI, MN_D};
static const char* STAX_D[2]	= {MN_STAX, MN_D};
static const char* INX_D[2]		= {MN_INX, MN_D};
static const char* INR_D[2]		= {MN_INR, MN_D};
static const char* DCR_D[2]		= {MN_DCR, MN_D};
static const char* MVI_D[2]		= {MN_MVI, MN_D};
static const char* RAL[1]		= {MN_RAL};
static const char* DB_18[2]		= {MN_DB, MN_18};
static const char* DAD_D[2]		= {MN_DAD, MN_D};
static const char* LDAX_D[2]	= {MN_LDAX, MN_D};
static const char* DCX_D[2]		= {MN_DCX, MN_D};
static const char* INR_E[2]		= {MN_INR, MN_E};
static const char* DCR_E[2]		= {MN_DCR, MN_E};
static const char* MVI_E[2]		= {MN_MVI, MN_E};
static const char* RAR[1]		= {MN_RAR};
static const char* DB_20[2]		= {MN_DB, MN_20};
static const char* LXI_H[2]		= {MN_LXI, MN_H};
static const char* SHLD[1]		= {MN_SHLD};
static const char* INX_H[2]		= {MN_INX, MN_H};
static const char* INR_H[2]		= {MN_INR, MN_H};
static const char* DCR_H[2]		= {MN_DCR, MN_H};
static const char* MVI_H[2]		= {MN_MVI, MN_H};
static const char* DAA[1]		= {MN_DAA};
static const char* DB_28[2]		= {MN_DB, MN_28};
static const char* DAD_H[2]		= {MN_DAD, MN_H};
static const char* LHLD[1]		= {MN_LHLD};
static const char* DCX_H[2]		= {MN_DCX, MN_H};
static const char* INR_L[2]		= {MN_INR, MN_L};
static const char* DCR_L[2]		= {MN_DCR, MN_L};
static const char* MVI_L[2]		= {MN_MVI, MN_L};
static const char* CMA[1]		= {MN_CMA};
static const char* DB_30[2]		= {MN_DB, MN_30};
static const char* LXI_SP[2]	= {MN_LXI, MN_SP};
static const char* STA[1]		= {MN_STA};
static const char* INX_SP[2]	= {MN_INX, MN_SP};
static const char* INR_M[2]		= {MN_INR, MN_M};
static const char* DCR_M[2]		= {MN_DCR, MN_M};
static const char* MVI_M[2]		= {MN_MVI, MN_M};
static const char* STC[1]		= {MN_STC};
static const char* DB_38[2]		= {MN_DB, MN_38};
static const char* DAD_SP[2]	= {MN_DAD, MN_SP};
static const char* LDA[1]		= {MN_LDA};
static const char* DCX_SP[2]	= {MN_DCX, MN_SP};
static const char* INR_A[2]		= {MN_INR, MN_A};
static const char* DCR_A[2]		= {MN_DCR, MN_A};
static const char* MVI_A[2]		= {MN_MVI, MN_A};
static const char* CMC[1]		= {MN_CMC};
static const char* MOV_BB[3]	= {MN_MOV, MN_B, MN_B};
static const char* MOV_BC[3]	= {MN_MOV, MN_B, MN_C};
static const char* MOV_BD[3]	= {MN_MOV, MN_B, MN_D};
static const char* MOV_BE[3]	= {MN_MOV, MN_B, MN_E};
static const char* MOV_BH[3]	= {MN_MOV, MN_B, MN_H};
static const char* MOV_BL[3]	= {MN_MOV, MN_B, MN_L};
static const char* MOV_BM[3]	= {MN_MOV, MN_B, MN_M};
static const char* MOV_BA[3]	= {MN_MOV, MN_B, MN_A};
static const char* MOV_CB[3]	= {MN_MOV, MN_C, MN_B};
static const char* MOV_CC[3]	= {MN_MOV, MN_C, MN_C};
static const char* MOV_CD[3]	= {MN_MOV, MN_C, MN_D};
static const char* MOV_CE[3]	= {MN_MOV, MN_C, MN_E};
static const char* MOV_CH[3]	= {MN_MOV, MN_C, MN_H};
static const char* MOV_CL[3]	= {MN_MOV, MN_C, MN_L};
static const char* MOV_CM[3]	= {MN_MOV, MN_C, MN_M};
static const char* MOV_CA[3]	= {MN_MOV, MN_C, MN_A};
static const char* MOV_DB[3]	= {MN_MOV, MN_D, MN_B};
static const char* MOV_DC[3]	= {MN_MOV, MN_D, MN_C};
static const char* MOV_DD[3]	= {MN_MOV, MN_D, MN_D};
static const char* MOV_DE[3]	= {MN_MOV, MN_D, MN_E};
static const char* MOV_DH[3]	= {MN_MOV, MN_D, MN_H};
static const char* MOV_DL[3]	= {MN_MOV, MN_D, MN_L};
static const char* MOV_DM[3]	= {MN_MOV, MN_D, MN_M};
static const char* MOV_DA[3]	= {MN_MOV, MN_D, MN_A};
static const char* MOV_EB[3]	= {MN_MOV, MN_E, MN_B};
static const char* MOV_EC[3]	= {MN_MOV, MN_E, MN_C};
static const char* MOV_ED[3]	= {MN_MOV, MN_E, MN_D};
static const char* MOV_EE[3]	= {MN_MOV, MN_E, MN_E};
static const char* MOV_EH[3]	= {MN_MOV, MN_E, MN_H};
static const char* MOV_EL[3]	= {MN_MOV, MN_E, MN_L};
static const char* MOV_EM[3]	= {MN_MOV, MN_E, MN_M};
static const char* MOV_EA[3]	= {MN_MOV, MN_E, MN_A};
static const char* MOV_HB[3]	= {MN_MOV, MN_H, MN_B};
static const char* MOV_HC[3]	= {MN_MOV, MN_H, MN_C};
static const char* MOV_HD[3]	= {MN_MOV, MN_H, MN_D};
static const char* MOV_HE[3]	= {MN_MOV, MN_H, MN_E};
static const char* MOV_HH[3]	= {MN_MOV, MN_H, MN_H};
static const char* MOV_HL[3]	= {MN_MOV, MN_H, MN_L};
static const char* MOV_HM[3]	= {MN_MOV, MN_H, MN_M};
static const char* MOV_HA[3]	= {MN_MOV, MN_H, MN_A};
static const char* MOV_LB[3]	= {MN_MOV, MN_L, MN_B};
static const char* MOV_LC[3]	= {MN_MOV, MN_L, MN_C};
static const char* MOV_LD[3]	= {MN_MOV, MN_L, MN_D};
static const char* MOV_LE[3]	= {MN_MOV, MN_L, MN_E};
static const char* MOV_LH[3]	= {MN_MOV, MN_L, MN_H};
static const char* MOV_LL[3]	= {MN_MOV, MN_L, MN_L};
static const char* MOV_LM[3]	= {MN_MOV, MN_L, MN_M};
static const char* MOV_LA[3]	= {MN_MOV, MN_L, MN_A};
static const char* MOV_MB[3]	= {MN_MOV, MN_M, MN_B};
static const char* MOV_MC[3]	= {MN_MOV, MN_M, MN_C};
static const char* MOV_MD[3]	= {MN_MOV, MN_M, MN_D};
static const char* MOV_ME[3]	= {MN_MOV, MN_M, MN_E};
static const char* MOV_MH[3]	= {MN_MOV, MN_M, MN_H};
static const char* MOV_ML[3]	= {MN_MOV, MN_M, MN_L};
static const char* HLT[1]		= {MN_HLT};
static const char* MOV_MA[3]	= {MN_MOV, MN_M, MN_A};
static const char* MOV_AB[3]	= {MN_MOV, MN_A, MN_B};
static const char* MOV_AC[3]	= {MN_MOV, MN_A, MN_C};
static const char* MOV_AD[3]	= {MN_MOV, MN_A, MN_D};
static const char* MOV_AE[3]	= {MN_MOV, MN_A, MN_E};
static const char* MOV_AH[3]	= {MN_MOV, MN_A, MN_H};
static const char* MOV_AL[3]	= {MN_MOV, MN_A, MN_L};
static const char* MOV_AM[3]	= {MN_MOV, MN_A, MN_M};
static const char* MOV_AA[3]	= {MN_MOV, MN_A, MN_A};
static const char* ADD_B[2]		= {MN_ADD, MN_B};
static const char* ADD_C[2]		= {MN_ADD, MN_C};
static const char* ADD_D[2]		= {MN_ADD, MN_D};
static const char* ADD_E[2]		= {MN_ADD, MN_E};
static const char* ADD_H[2]		= {MN_ADD, MN_H};
static const char* ADD_L[2]		= {MN_ADD, MN_L};
static const char* ADD_M[2]		= {MN_ADD, MN_M};
static const char* ADD_A[2]		= {MN_ADD, MN_A};
static const char* ADC_B[2]		= {MN_ADC, MN_B};
static const char* ADC_C[2]		= {MN_ADC, MN_C};
static const char* ADC_D[2]		= {MN_ADC, MN_D};
static const char* ADC_E[2]		= {MN_ADC, MN_E};
static const char* ADC_H[2]		= {MN_ADC, MN_H};
static const char* ADC_L[2]		= {MN_ADC, MN_L};
static const char* ADC_M[2]		= {MN_ADC, MN_M};
static const char* ADC_A[2]		= {MN_ADC, MN_A};
static const char* SUB_B[2]		= {MN_SUB, MN_B};
static const char* SUB_C[2]		= {MN_SUB, MN_C};
static const char* SUB_D[2]		= {MN_SUB, MN_D};
static const char* SUB_E[2]		= {MN_SUB, MN_E};
static const char* SUB_H[2]		= {MN_SUB, MN_H};
static const char* SUB_L[2]		= {MN_SUB, MN_L};
static const char* SUB_M[2]		= {MN_SUB, MN_M};
static const char* SUB_A[2]		= {MN_SUB, MN_A};
static const char* SBB_B[2]		= {MN_SBB, MN_B};
static const char* SBB_C[2]		= {MN_SBB, MN_C};
static const char* SBB_D[2]		= {MN_SBB, MN_D};
static const char* SBB_E[2]		= {MN_SBB, MN_E};
static const char* SBB_H[2]		= {MN_SBB, MN_H};
static const char* SBB_L[2]		= {MN_SBB, MN_L};
static const char* SBB_M[2]		= {MN_SBB, MN_M};
static const char* SBB_A[2]		= {MN_SBB, MN_A};
static const char* ANA_B[2]		= {MN_ANA, MN_B};
static const char* ANA_C[2]		= {MN_ANA, MN_C};
static const char* ANA_D[2]		= {MN_ANA, MN_D};
static const char* ANA_E[2]		= {MN_ANA, MN_E};
static const char* ANA_H[2]		= {MN_ANA, MN_H};
static const char* ANA_L[2]		= {MN_ANA, MN_L};
static const char* ANA_M[2]		= {MN_ANA, MN_M};
static const char* ANA_A[2]		= {MN_ANA, MN_A};
static const char* XRA_B[2]		= {MN_XRA, MN_B};
static const char* XRA_C[2]		= {MN_XRA, MN_C};
static const char* XRA_D[2]		= {MN_XRA, MN_D};
static const char* XRA_E[2]		= {MN_XRA, MN_E};
static const char* XRA_H[2]		= {MN_XRA, MN_H};
static const char* XRA_L[2]		= {MN_XRA, MN_L};
static const char* XRA_M[2]		= {MN_XRA, MN_M};
static const char* XRA_A[2]		= {MN_XRA, MN_A};
static const char* ORA_B[2]		= {MN_ORA, MN_B};
static const char* ORA_C[2]		= {MN_ORA, MN_C};
static const char* ORA_D[2]		= {MN_ORA, MN_D};
static const char* ORA_E[2]		= {MN_ORA, MN_E};
static const char* ORA_H[2]		= {MN_ORA, MN_H};
static const char* ORA_L[2]		= {MN_ORA, MN_L};
static const char* ORA_M[2]		= {MN_ORA, MN_M};
static const char* ORA_A[2]		= {MN_ORA, MN_A};
static const char* CMP_B[2]		= {MN_CMP, MN_B};
static const char* CMP_C[2]		= {MN_CMP, MN_C};
static const char* CMP_D[2]		= {MN_CMP, MN_D};
static const char* CMP_E[2]		= {MN_CMP, MN_E};
static const char* CMP_H[2]		= {MN_CMP, MN_H};
static const char* CMP_L[2]		= {MN_CMP, MN_L};
static const char* CMP_M[2]		= {MN_CMP, MN_M};
static const char* CMP_A[2]		= {MN_CMP, MN_A};
static const char* RNZ[1]		= {MN_RNZ};
static const char* POP_B[2]		= {MN_POP, MN_B};
static const char* JNZ[1]		= {MN_JNZ};
static const char* JMP[1]		= {MN_JMP};
static const char* CNZ[1]		= {MN_CNZ};
static const char* PUSH_B[2]	= {MN_PUSH, MN_B};
static const char* ADI[1]		= {MN_ADI};
static const char* RST_0[2]		= {MN_RST, MN_0};
static const char* RZ[1]		= {MN_RZ};
static const char* RET[1]		= {MN_RET};
static const char* JZ[1]		= {MN_JZ};
static const char* DB_CB[2]		= {MN_DB, MN_CB};
static const char* CZ[1]		= {MN_CZ};
static const char* CALL[1]		= {MN_CALL};
static const char* ACI[1]		= {MN_ACI};
static const char* RST_1[2]		= {MN_RST, MN_1};
static const char* RNC[1]		= {MN_RNC};
static const char* POP_D[2]		= {MN_POP, MN_D};
static const char* JNC[1]		= {MN_JNC};
static const char* OUT1[1]		= {MN_OUT};
static const char* CNC[1]		= {MN_CNC};
static const char* PUSH[2]		= {MN_PUSH, MN_D};
static const char* SUI[1]		= {MN_SUI};
static const char* RST_2[2]		= {MN_RST, MN_2};
static const char* RC[1]		= {MN_RC};
static const char* DB_D9[2]		= {MN_DB, MN_D9};
static const char* JC[1]		= {MN_JC};
static const char* IN1[1]		= {MN_IN};
static const char* CC[1]		= {MN_CC};
static const char* DB_DD[2]		= {MN_DB, MN_DD};
static const char* SBI[1]		= {MN_SBI};
static const char* RST_3[2]		= {MN_RST, MN_3};
static const char* RPO[1]		= {MN_RPO};
static const char* POP_H[2]		= {MN_POP, MN_H};
static const char* JPO[1]		= {MN_JPO};
static const char* XTHL[1]		= {MN_XTHL};
static const char* CPO[1]		= {MN_CPO};
static const char* PUSH_H[2]	= {MN_PUSH, MN_H};
static const char* ANI[1]		= {MN_ANI};
static const char* RST_4[2]		= {MN_RST, MN_4};
static const char* RPE[1]		= {MN_RPE};
static const char* PCHL[1]		= {MN_PCHL};
static const char* JPE[1]		= {MN_JPE};
static const char* XCHG[1]		= {MN_XCHG};
static const char* CPE[1]		= {MN_CPE};
static const char* DB_ED[2]		= {MN_DB, MN_ED};
static const char* XRI[1]		= {MN_XRI};
static const char* RST_5[2]		= {MN_RST, MN_5};
static const char* RP[1]		= {MN_RP};
static const char* POP_PSW[2]	= {MN_POP, MN_PSW};
static const char* JP[1]		= {MN_JP};
static const char* DI[1]		= {MN_DI};
static const char* CP[1]		= {MN_CP};
static const char* PUSH_PSW[2]	= {MN_PUSH, MN_PSW};
static const char* ORI[1]		= {MN_ORI};
static const char* RST_6[2]		= {MN_RST, MN_6};
static const char* RM[1]		= {MN_RM};
static const char* SPHL[1]		= {MN_SPHL};
static const char* JM[1]		= {MN_JM};
static const char* EI[1]		= {MN_EI};
static const char* CM[1]		= {MN_CM};
static const char* DB_FD[2]		= {MN_DB, MN_FD};
static const char* CPI[1]		= {MN_CPI};
static const char* RST_7[2]		= {MN_RST, MN_7};

// mnemonics names
static const char** mnenomics[DISASM_CMDS] =
{
	NOP, LXI_B,	STAX_B,	INX_B,	INR_B,	DCR_B,	MVI_B,	RLC, DB_,	DAD_B,	LDAX_B,	DCX_B,	INR_C,	DCR_C,	MVI_C,	RRC,
	DB_, LXI_D,	STAX_D,	INX_D,	INR_D,	DCR_D,	MVI_D,	RAL, DB_,	DAD_D,	LDAX_D,	DCX_D,	INR_E,	DCR_E,	MVI_E,	RAR,
	DB_, LXI_H,	SHLD,	INX_H,	INR_H,	DCR_H,	MVI_H,	DAA, DB_,	DAD_H,	LHLD,	DCX_H,	INR_L,	DCR_L,	MVI_L,	CMA,
	DB_, LXI_SP,STA,	INX_SP,	INR_M,	DCR_M,	MVI_M,	STC, DB_,	DAD_SP,	LDA,	DCX_SP,	INR_A,	DCR_A,	MVI_A,	CMC,

	MOV_BB, MOV_BC, MOV_BD, MOV_BE, MOV_BH, MOV_BL, MOV_BM, MOV_BA, MOV_CB, MOV_CC, MOV_CD, MOV_CE, MOV_CH, MOV_CL, MOV_CM, MOV_CA,
	MOV_DB, MOV_DC, MOV_DD, MOV_DE, MOV_DH, MOV_DL, MOV_DM, MOV_DA, MOV_EB, MOV_EC, MOV_ED, MOV_EE, MOV_EH, MOV_EL, MOV_EM, MOV_EA,
	MOV_HB, MOV_HC, MOV_HD, MOV_HE, MOV_HH, MOV_HL, MOV_HM, MOV_HA, MOV_LB, MOV_LC, MOV_LD, MOV_LE, MOV_LH, MOV_LL, MOV_LM, MOV_LA,
	MOV_MB, MOV_MC, MOV_MD, MOV_ME, MOV_MH, MOV_ML, HLT,	MOV_MA, MOV_AB, MOV_AC, MOV_AD, MOV_AE, MOV_AH, MOV_AL, MOV_AM, MOV_AA,

	ADD_B, ADD_C, ADD_D, ADD_E, ADD_H, ADD_L, ADD_M, ADD_A, ADC_B, ADC_C, ADC_D, ADC_E, ADC_H, ADC_L, ADC_M, ADC_A,
	SUB_B, SUB_C, SUB_D, SUB_E, SUB_H, SUB_L, SUB_M, SUB_A, SBB_B, SBB_C, SBB_D, SBB_E, SBB_H, SBB_L, SBB_M, SBB_A,
	ANA_B, ANA_C, ANA_D, ANA_E, ANA_H, ANA_L, ANA_M, ANA_A, XRA_B, XRA_C, XRA_D, XRA_E, XRA_H, XRA_L, XRA_M, XRA_A,
	ORA_B, ORA_C, ORA_D, ORA_E, ORA_H, ORA_L, ORA_M, ORA_A, CMP_B, CMP_C, CMP_D, CMP_E, CMP_H, CMP_L, CMP_M, CMP_A,

	RNZ, POP_B,	  JNZ,	JMP,  CNZ, PUSH_B,	 ADI, RST_0, RZ,  RET,  JZ,  DB_,	CZ,  CALL,	 ACI, RST_1,
	RNC, POP_D,	  JNC,	OUT1, CNC, PUSH,	 SUI, RST_2, RC,  DB_,	JC,  IN1,	CC,  DB_, SBI, RST_3,
	RPO, POP_H,	  JPO,	XTHL, CPO, PUSH_H,	 ANI, RST_4, RPE, PCHL, JPE, XCHG,	CPE, DB_, XRI, RST_5,
	RP,  POP_PSW, JP,	DI,   CP,  PUSH_PSW, ORI, RST_6, RM,  SPHL, JM,  EI,	CM,  DB_, CPI, RST_7,
};

const uint8_t T_CMD[1]		= {MNT_CMD};
const uint8_t T_CMD_R[2]	= {MNT_CMD, MNT_REG};
const uint8_t T_CMD_RR[3]	= {MNT_CMD, MNT_REG, MNT_REG}; 
const uint8_t T_CMD_IM[2]	= {MNT_CMD, MNT_IMM};

#define T_DB		T_CMD

#define T_NOP		T_CMD
#define T_LXI_B		T_CMD_R
#define T_STAX_B	T_CMD_R
#define T_INX_B		T_CMD_R
#define T_INR_B		T_CMD_R
#define T_DCR_B		T_CMD_R
#define T_MVI_B		T_CMD_R
#define T_RLC		T_CMD
#define T_DB_8		T_CMD_IM
#define T_DAD_B		T_CMD_R
#define T_LDAX_B	T_CMD_R
#define T_DCX_B		T_CMD_R
#define T_INR_C		T_CMD_R
#define T_DCR_C		T_CMD_R
#define T_MVI_C		T_CMD_R
#define T_RRC		T_CMD
#define T_DB_10		T_CMD_IM
#define T_LXI_D		T_CMD_R
#define T_STAX_D	T_CMD_R
#define T_INX_D		T_CMD_R
#define T_INR_D		T_CMD_R
#define T_DCR_D		T_CMD_R
#define T_MVI_D		T_CMD_R
#define T_RAL		T_CMD
#define T_DB_18		T_CMD_IM
#define T_DAD_D		T_CMD_R
#define T_LDAX_D	T_CMD_R
#define T_DCX_D		T_CMD_R
#define T_INR_E		T_CMD_R
#define T_DCR_E		T_CMD_R
#define T_MVI_E		T_CMD_R
#define T_RAR		T_CMD
#define T_DB_20		T_CMD_IM
#define T_LXI_H		T_CMD_R
#define T_SHLD		T_CMD
#define T_INX_H		T_CMD_R
#define T_INR_H		T_CMD_R
#define T_DCR_H		T_CMD_R
#define T_MVI_H		T_CMD_R
#define T_DAA		T_CMD
#define T_DB_28		T_CMD_IM
#define T_DAD_H		T_CMD_R
#define T_LHLD		T_CMD
#define T_DCX_H		T_CMD_R
#define T_INR_L		T_CMD_R
#define T_DCR_L		T_CMD_R
#define T_MVI_L		T_CMD_R
#define T_CMA		T_CMD
#define T_DB_30		T_CMD_IM
#define T_LXI_SP	T_CMD_R
#define T_STA		T_CMD
#define T_INX_SP	T_CMD_R
#define T_INR_M		T_CMD_R
#define T_DCR_M		T_CMD_R
#define T_MVI_M		T_CMD_R
#define T_STC		T_CMD
#define T_DB_38		T_CMD_IM
#define T_DAD_SP	T_CMD_R
#define T_LDA		T_CMD
#define T_DCX_SP	T_CMD_R
#define T_INR_A		T_CMD_R
#define T_DCR_A		T_CMD_R
#define T_MVI_A		T_CMD_R
#define T_CMC		T_CMD
#define T_MOV_BB	T_CMD_RR
#define T_MOV_BC	T_CMD_RR
#define T_MOV_BD	T_CMD_RR
#define T_MOV_BE	T_CMD_RR
#define T_MOV_BH	T_CMD_RR
#define T_MOV_BL	T_CMD_RR
#define T_MOV_BM	T_CMD_RR
#define T_MOV_BA	T_CMD_RR
#define T_MOV_CB	T_CMD_RR
#define T_MOV_CC	T_CMD_RR
#define T_MOV_CD	T_CMD_RR
#define T_MOV_CE	T_CMD_RR
#define T_MOV_CH	T_CMD_RR
#define T_MOV_CL	T_CMD_RR
#define T_MOV_CM	T_CMD_RR
#define T_MOV_CA	T_CMD_RR
#define T_MOV_DB	T_CMD_RR
#define T_MOV_DC	T_CMD_RR
#define T_MOV_DD	T_CMD_RR
#define T_MOV_DE	T_CMD_RR
#define T_MOV_DH	T_CMD_RR
#define T_MOV_DL	T_CMD_RR
#define T_MOV_DM	T_CMD_RR
#define T_MOV_DA	T_CMD_RR
#define T_MOV_EB	T_CMD_RR
#define T_MOV_EC	T_CMD_RR
#define T_MOV_ED	T_CMD_RR
#define T_MOV_EE	T_CMD_RR
#define T_MOV_EH	T_CMD_RR
#define T_MOV_EL	T_CMD_RR
#define T_MOV_EM	T_CMD_RR
#define T_MOV_EA	T_CMD_RR
#define T_MOV_HB	T_CMD_RR
#define T_MOV_HC	T_CMD_RR
#define T_MOV_HD	T_CMD_RR
#define T_MOV_HE	T_CMD_RR
#define T_MOV_HH	T_CMD_RR
#define T_MOV_HL	T_CMD_RR
#define T_MOV_HM	T_CMD_RR
#define T_MOV_HA	T_CMD_RR
#define T_MOV_LB	T_CMD_RR
#define T_MOV_LC	T_CMD_RR
#define T_MOV_LD	T_CMD_RR
#define T_MOV_LE	T_CMD_RR
#define T_MOV_LH	T_CMD_RR
#define T_MOV_LL	T_CMD_RR
#define T_MOV_LM	T_CMD_RR
#define T_MOV_LA	T_CMD_RR
#define T_MOV_MB	T_CMD_RR
#define T_MOV_MC	T_CMD_RR
#define T_MOV_MD	T_CMD_RR
#define T_MOV_ME	T_CMD_RR
#define T_MOV_MH	T_CMD_RR
#define T_MOV_ML	T_CMD_RR
#define T_HLT		T_CMD
#define T_MOV_MA	T_CMD_RR
#define T_MOV_AB	T_CMD_RR
#define T_MOV_AC	T_CMD_RR
#define T_MOV_AD	T_CMD_RR
#define T_MOV_AE	T_CMD_RR
#define T_MOV_AH	T_CMD_RR
#define T_MOV_AL	T_CMD_RR
#define T_MOV_AM	T_CMD_RR
#define T_MOV_AA	T_CMD_RR
#define T_ADD_B		T_CMD_R
#define T_ADD_C		T_CMD_R
#define T_ADD_D		T_CMD_R
#define T_ADD_E		T_CMD_R
#define T_ADD_H		T_CMD_R
#define T_ADD_L		T_CMD_R
#define T_ADD_M		T_CMD_R
#define T_ADD_A		T_CMD_R
#define T_ADC_B		T_CMD_R
#define T_ADC_C		T_CMD_R
#define T_ADC_D		T_CMD_R
#define T_ADC_E		T_CMD_R
#define T_ADC_H		T_CMD_R
#define T_ADC_L		T_CMD_R
#define T_ADC_M		T_CMD_R
#define T_ADC_A		T_CMD_R
#define T_SUB_B		T_CMD_R
#define T_SUB_C		T_CMD_R
#define T_SUB_D		T_CMD_R
#define T_SUB_E		T_CMD_R
#define T_SUB_H		T_CMD_R
#define T_SUB_L		T_CMD_R
#define T_SUB_M		T_CMD_R
#define T_SUB_A		T_CMD_R
#define T_SBB_B		T_CMD_R
#define T_SBB_C		T_CMD_R
#define T_SBB_D		T_CMD_R
#define T_SBB_E		T_CMD_R
#define T_SBB_H		T_CMD_R
#define T_SBB_L		T_CMD_R
#define T_SBB_M		T_CMD_R
#define T_SBB_A		T_CMD_R
#define T_ANA_B		T_CMD_R
#define T_ANA_C		T_CMD_R
#define T_ANA_D		T_CMD_R
#define T_ANA_E		T_CMD_R
#define T_ANA_H		T_CMD_R
#define T_ANA_L		T_CMD_R
#define T_ANA_M		T_CMD_R
#define T_ANA_A		T_CMD_R
#define T_XRA_B		T_CMD_R
#define T_XRA_C		T_CMD_R
#define T_XRA_D		T_CMD_R
#define T_XRA_E		T_CMD_R
#define T_XRA_H		T_CMD_R
#define T_XRA_L		T_CMD_R
#define T_XRA_M		T_CMD_R
#define T_XRA_A		T_CMD_R
#define T_ORA_B		T_CMD_R
#define T_ORA_C		T_CMD_R
#define T_ORA_D		T_CMD_R
#define T_ORA_E		T_CMD_R
#define T_ORA_H		T_CMD_R
#define T_ORA_L		T_CMD_R
#define T_ORA_M		T_CMD_R
#define T_ORA_A		T_CMD_R
#define T_CMP_B		T_CMD_R
#define T_CMP_C		T_CMD_R
#define T_CMP_D		T_CMD_R
#define T_CMP_E		T_CMD_R
#define T_CMP_H		T_CMD_R
#define T_CMP_L		T_CMD_R
#define T_CMP_M		T_CMD_R
#define T_CMP_A		T_CMD_R
#define T_RNZ		T_CMD
#define T_POP_B		T_CMD_R
#define T_JNZ		T_CMD
#define T_JMP		T_CMD
#define T_CNZ		T_CMD
#define T_PUSH_B	T_CMD_R
#define T_ADI		T_CMD_R
#define T_RST_0		T_CMD_IM
#define T_RZ		T_CMD
#define T_RET		T_CMD
#define T_JZ		T_CMD
#define T_DB_CB		T_CMD_IM
#define T_CZ		T_CMD
#define T_CALL		T_CMD
#define T_ACI		T_CMD
#define T_RST_1		T_CMD_IM
#define T_RNC		T_CMD
#define T_POP_D		T_CMD_R
#define T_JNC		T_CMD
#define T_OUT1		T_CMD
#define T_CNC		T_CMD
#define T_PUSH		T_CMD_R
#define T_SUI		T_CMD
#define T_RST_2		T_CMD_IM
#define T_RC		T_CMD
#define T_DB_D9		T_CMD_IM
#define T_JC		T_CMD
#define T_IN1		T_CMD
#define T_CC		T_CMD
#define T_DB_DD		T_CMD_IM
#define T_SBI		T_CMD
#define T_RST_3		T_CMD_IM
#define T_RPO		T_CMD
#define T_POP_H		T_CMD_R
#define T_JPO		T_CMD
#define T_XTHL		T_CMD
#define T_CPO		T_CMD
#define T_PUSH_H	T_CMD_R
#define T_ANI		T_CMD
#define T_RST_4		T_CMD_IM
#define T_RPE		T_CMD
#define T_PCHL		T_CMD
#define T_JPE		T_CMD
#define T_XCHG		T_CMD
#define T_CPE		T_CMD
#define T_DB_ED		T_CMD_IM
#define T_XRI		T_CMD
#define T_RST_5		T_CMD_IM
#define T_RP		T_CMD
#define T_POP_PSW	T_CMD_R
#define T_JP		T_CMD
#define T_DI		T_CMD
#define T_CP		T_CMD
#define T_PUSH_PSW	T_CMD_R
#define T_ORI		T_CMD
#define T_RST_6		T_CMD_IM
#define T_RM		T_CMD
#define T_SPHL		T_CMD
#define T_JM		T_CMD
#define T_EI		T_CMD
#define T_CM		T_CMD
#define T_DB_FD		T_CMD_IM
#define T_CPI		T_CMD
#define T_RST_7		T_CMD_IM

// types of a mnemonic parts
static const uint8_t* mnenomicTypes [DISASM_CMDS] =
{
	T_NOP,T_LXI_B,	T_STAX_B,	T_INX_B,	T_INR_B,	T_DCR_B,	T_MVI_B,	T_RLC, T_DB,	T_DAD_B,	T_LDAX_B,	T_DCX_B,	T_INR_C,	T_DCR_C,	T_MVI_C,	T_RRC,
	T_DB, T_LXI_D,	T_STAX_D,	T_INX_D,	T_INR_D,	T_DCR_D,	T_MVI_D,	T_RAL, T_DB,	T_DAD_D,	T_LDAX_D,	T_DCX_D,	T_INR_E,	T_DCR_E,	T_MVI_E,	T_RAR,
	T_DB, T_LXI_H,	T_SHLD,		T_INX_H,	T_INR_H,	T_DCR_H,	T_MVI_H,	T_DAA, T_DB,	T_DAD_H,	T_LHLD,		T_DCX_H,	T_INR_L,	T_DCR_L,	T_MVI_L,	T_CMA,
	T_DB, T_LXI_SP,	T_STA,		T_INX_SP,	T_INR_M,	T_DCR_M,	T_MVI_M,	T_STC, T_DB,	T_DAD_SP,	T_LDA,		T_DCX_SP,	T_INR_A,	T_DCR_A,	T_MVI_A,	T_CMC,

	T_MOV_BB, T_MOV_BC, T_MOV_BD, T_MOV_BE, T_MOV_BH, T_MOV_BL, T_MOV_BM, T_MOV_BA, T_MOV_CB, T_MOV_CC, T_MOV_CD, T_MOV_CE, T_MOV_CH, T_MOV_CL, T_MOV_CM, T_MOV_CA,
	T_MOV_DB, T_MOV_DC, T_MOV_DD, T_MOV_DE, T_MOV_DH, T_MOV_DL, T_MOV_DM, T_MOV_DA, T_MOV_EB, T_MOV_EC, T_MOV_ED, T_MOV_EE, T_MOV_EH, T_MOV_EL, T_MOV_EM, T_MOV_EA,
	T_MOV_HB, T_MOV_HC, T_MOV_HD, T_MOV_HE, T_MOV_HH, T_MOV_HL, T_MOV_HM, T_MOV_HA, T_MOV_LB, T_MOV_LC, T_MOV_LD, T_MOV_LE, T_MOV_LH, T_MOV_LL, T_MOV_LM, T_MOV_LA,
	T_MOV_MB, T_MOV_MC, T_MOV_MD, T_MOV_ME, T_MOV_MH, T_MOV_ML, T_HLT,	T_MOV_MA, T_MOV_AB, T_MOV_AC, T_MOV_AD, T_MOV_AE, T_MOV_AH, T_MOV_AL, T_MOV_AM, T_MOV_AA,

	T_ADD_B, T_ADD_C, T_ADD_D, T_ADD_E, T_ADD_H, T_ADD_L, T_ADD_M, T_ADD_A, T_ADC_B, T_ADC_C, T_ADC_D, T_ADC_E, T_ADC_H, T_ADC_L, T_ADC_M, T_ADC_A,
	T_SUB_B, T_SUB_C, T_SUB_D, T_SUB_E, T_SUB_H, T_SUB_L, T_SUB_M, T_SUB_A, T_SBB_B, T_SBB_C, T_SBB_D, T_SBB_E, T_SBB_H, T_SBB_L, T_SBB_M, T_SBB_A,
	T_ANA_B, T_ANA_C, T_ANA_D, T_ANA_E, T_ANA_H, T_ANA_L, T_ANA_M, T_ANA_A, T_XRA_B, T_XRA_C, T_XRA_D, T_XRA_E, T_XRA_H, T_XRA_L, T_XRA_M, T_XRA_A,
	T_ORA_B, T_ORA_C, T_ORA_D, T_ORA_E, T_ORA_H, T_ORA_L, T_ORA_M, T_ORA_A, T_CMP_B, T_CMP_C, T_CMP_D, T_CMP_E, T_CMP_H, T_CMP_L, T_CMP_M, T_CMP_A,

	T_RNZ, T_POP_B,	  T_JNZ,	T_JMP,  T_CNZ, T_PUSH_B,	T_ADI, T_RST_0, T_RZ,  T_RET,   T_JZ,  T_DB,	T_CZ,  T_CALL,	T_ACI, T_RST_1,
	T_RNC, T_POP_D,	  T_JNC,	T_OUT1, T_CNC, T_PUSH,		T_SUI, T_RST_2, T_RC,  T_DB,	T_JC,  T_IN1,	T_CC,  T_DB, T_SBI, T_RST_3,
	T_RPO, T_POP_H,	  T_JPO,	T_XTHL, T_CPO, T_PUSH_H,	T_ANI, T_RST_4, T_RPE, T_PCHL,  T_JPE, T_XCHG,	T_CPE, T_DB, T_XRI, T_RST_5,
	T_RP,  T_POP_PSW, T_JP,		T_DI,   T_CP,  T_PUSH_PSW,	T_ORI, T_RST_6, T_RM,  T_SPHL,  T_JM,  T_EI,	T_CM,  T_DB, T_CPI, T_RST_7,
};

// instruction lengths in bytes
static const uint8_t cmdLens[DISASM_CMDS] =
{
	1,3,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,3,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,3,3,1,1,1,2,1,1,1,3,1,1,1,2,1,
	1,3,3,1,1,1,2,1,1,1,3,1,1,1,2,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,3,3,3,1,2,1,1,1,3,1,3,3,2,1,
	1,1,3,2,3,1,2,1,1,1,3,2,3,1,2,1,
	1,1,3,1,3,1,2,1,1,1,3,1,3,1,2,1,
	1,1,3,1,3,1,2,1,1,1,3,1,3,1,2,1
};

#define CMD_IMM_OFFSET_MASK	0x1
#define CMD_IMM_LEN_MASK	0x2

// instruction immediate operand.
static const uint8_t cmdImms[DISASM_CMDS] =
{
	CMD_IM_NONE, CMD_IW_OFF1, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE, CMD_IB_OFF0, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE,
	CMD_IB_OFF0, CMD_IW_OFF1, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE, CMD_IB_OFF0, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE,
	CMD_IB_OFF0, CMD_IW_OFF1, CMD_IW_OFF1, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE, CMD_IB_OFF0, CMD_IM_NONE, CMD_IW_OFF1, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE,
	CMD_IB_OFF0, CMD_IW_OFF1, CMD_IW_OFF1, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE, CMD_IB_OFF0, CMD_IM_NONE, CMD_IW_OFF1, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE,

	CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE,
	CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE,
	CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE,
	CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE,

	CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE,
	CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE,
	CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE,
	CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE,

	CMD_IM_NONE, CMD_IM_NONE, CMD_IW_OFF1, CMD_IW_OFF1, CMD_IW_OFF1, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IW_OFF1, CMD_IB_OFF0, CMD_IW_OFF1, CMD_IW_OFF1, CMD_IB_OFF1, CMD_IM_NONE,
	CMD_IM_NONE, CMD_IM_NONE, CMD_IW_OFF1, CMD_IB_OFF1, CMD_IW_OFF1, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IW_OFF1, CMD_IB_OFF1, CMD_IW_OFF1, CMD_IB_OFF0, CMD_IB_OFF1, CMD_IM_NONE,
	CMD_IM_NONE, CMD_IM_NONE, CMD_IW_OFF1, CMD_IM_NONE, CMD_IW_OFF1, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IW_OFF1, CMD_IM_NONE, CMD_IW_OFF1, CMD_IB_OFF0, CMD_IB_OFF1, CMD_IM_NONE,
	CMD_IM_NONE, CMD_IM_NONE, CMD_IW_OFF1, CMD_IM_NONE, CMD_IW_OFF1, CMD_IM_NONE, CMD_IB_OFF1, CMD_IM_NONE, CMD_IM_NONE, CMD_IM_NONE, CMD_IW_OFF1, CMD_IM_NONE, CMD_IW_OFF1, CMD_IB_OFF0, CMD_IB_OFF1, CMD_IM_NONE,
};

// mnemonics names lens
static const uint8_t mnenomicLens[DISASM_CMDS] =
{
	1, 2, 2, 2,	2, 2, 2, 1, 1, 2, 2, 2,	2, 2, 2, 1,
	1, 2, 2, 2,	2, 2, 2, 1, 1, 2, 2, 2,	2, 2, 2, 1,
	1, 2, 1, 2,	2, 2, 2, 1, 1, 2, 1, 2,	2, 2, 2, 1,
	1, 2, 1, 2,	2, 2, 2, 1, 1, 2, 1, 2,	2, 2, 2, 1,

	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3,

	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,

	1, 2, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2,
	1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1,	1, 1, 1, 2,
	1, 2, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1,	1, 1, 1, 2,
	1, 2, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1,	1, 1, 1, 2,
};

#define I16_ADDRS_LEN 7	// sizeof("0xFFFF");
static char addrsS[0x10000 * I16_ADDRS_LEN]; // for fast Addr to AddrS conversion

#define I8_ADDRS_LEN 5	// sizeof("0xFF");
static char smallAddrsS[0x100 * I8_ADDRS_LEN]; // for fast Addr to AddrS conversion

// 0 - call
// 1 - c*
// 2 - jmp, 
// 3 - j*
// 4 - ret, r*
// 5 - pchl
// 6 - rst
// 7 - other
#define OPTYPE_C__	0
#define OPTYPE_CAL	1
#define OPTYPE_J__	2
#define OPTYPE_JMP	3
#define OPTYPE_R__	5
#define OPTYPE_RET	4
#define OPTYPE_PCH	6
#define OPTYPE_RST	7
#define OPTYPE____	8
static const uint8_t opcodeTypes[DISASM_CMDS] =
{
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,

	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,

	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,

	4, 8, 2, 3, 0, 8, 8, 7, 4, 5, 2, 8, 0, 1, 8, 7,
	4, 8, 2, 8, 0, 8, 8, 7, 4, 8, 2, 8, 0, 8, 8, 7,
	4, 8, 2, 8, 0, 8, 8, 7, 4, 6, 2, 8, 0, 8, 8, 7,
	4, 8, 2, 8, 0, 8, 8, 7, 4, 8, 2, 8, 0, 8, 8, 7,
};

#define OPCODE_TYPE_MAX 7

void InitAddrsS()
{
	static bool inited = false;
	if (inited) return;
	inited = true;

	char addrS[5]; // "FFFF"
	for (int i = 0, addr = 0; i < sizeof(addrsS); addr++)
	{
		sprintf_s(addrS, 5, "%04X", addr);
		addrsS[i++] = '0';
		addrsS[i++] = 'x';
		addrsS[i++] = addrS[0];
		addrsS[i++] = addrS[1];
		addrsS[i++] = addrS[2];
		addrsS[i++] = addrS[3];
		addrsS[i++] = 0;
	}

	char smallAddrS[3]; // "FF"
	for (int i = 0, addr = 0; i < sizeof(smallAddrsS); addr++)
	{
		sprintf_s(smallAddrS, 3, "%02X", addr);
		smallAddrsS[i++] = '0';
		smallAddrsS[i++] = 'x';
		smallAddrsS[i++] = smallAddrS[0];
		smallAddrsS[i++] = smallAddrS[1];
		smallAddrsS[i++] = 0;
	}
}

struct IniterAddrsS {
	IniterAddrsS() { InitAddrsS(); }
};
static IniterAddrsS initerAddrsS;

auto dev::AddrToAddrI16S(const Addr _addr) -> const char* { return addrsS + _addr * I16_ADDRS_LEN; }
auto dev::AddrToAddrI8S(const uint8_t _addr) -> const char* { return smallAddrsS + _addr * I8_ADDRS_LEN; }
auto dev::GetMnemonic(const uint8_t _opcode) -> const char** { return mnenomics[_opcode]; }
auto dev::GetMnemonicLen(const uint8_t _opcode) -> uint8_t { return mnenomicLens[_opcode]; }
auto dev::GetMnemonicType(const uint8_t _opcode) -> const uint8_t* { return mnenomicTypes[_opcode]; }
auto dev::GetImmediateType(const uint8_t _opcode) -> uint8_t { return cmdImms[_opcode]; }

void dev::Disasm::Line::Init()
{
	type = Type::CODE;
	addr = 0;
	opcode = 0;
	imm = 0; // immediate operand
	statsS[0] = '\0'; // contains: runs, reads, writes
	labels = nullptr;
	consts = nullptr; // labels used as constants only
	comment = nullptr;
	accessed = false; // no runs, reads, writes yet
	breakpointStatus = Breakpoint::Status::DISABLED;
}

auto dev::Disasm::Line::GetImmediateS() const
-> const char*
{ 
	return cmdImms[opcode] == CMD_IW_OFF1 ? AddrToAddrI16S(imm) : AddrToAddrI8S(static_cast<uint8_t>(imm));
};

void dev::Disasm::AddLabes(const Addr _addr, const Labels& _labels)
{
	if (lineIdx >= DISASM_LINES_MAX) return;

	auto labelsI = _labels.find(_addr);
	if (labelsI == _labels.end()) return;

	auto& line = lines.at(lineIdx);
	line.Init();

	line.type = Line::Type::LABELS;
	line.addr = _addr;
	line.labels = &labelsI->second;

	lineIdx++;
}

void dev::Disasm::AddComment(const Addr _addr, const Comments& _comments)
{
	if (lineIdx >= DISASM_LINES_MAX) return;

	auto commentI = _comments.find(_addr);
	if (commentI == _comments.end()) return;

	auto& line = lines.at(lineIdx);
	line.Init();

	line.type = Line::Type::COMMENT;
	line.addr = _addr;
	line.comment = &commentI->second;

	lineIdx++;
}

auto dev::Disasm::AddDb(const Addr _addr, const uint8_t _data,
	const Labels& _consts,
	const uint64_t _runs, const uint64_t _reads, const uint64_t _writes,
	const Breakpoint::Status _breakpointStatus)
-> Addr
{
	if (lineIdx >= DISASM_LINES_MAX) return 0;

	auto& line = lines.at(lineIdx);
	line.Init();
	line.type = Line::Type::CODE;
	line.addr = _addr;
	line.opcode = 0x10; // db 0x10 is used as a placeholder
	line.imm = _data;
	line.accessed = _runs != UINT64_MAX && _reads != UINT64_MAX && _writes != UINT64_MAX;
	line.breakpointStatus = _breakpointStatus;

	auto constsI = _consts.find(_data);
	line.consts = constsI == _consts.end() ? nullptr : &(constsI->second);

	snprintf(line.statsS, sizeof(line.statsS), "%zu,%zu,%zu", _runs, _reads, _writes);
	lineIdx++;
	return 1;
}

auto dev::Disasm::AddCode(const Addr _addr, const uint32_t _cmd,
	const Labels& _labels, const Labels& _consts,
	const uint64_t _runs, const uint64_t _reads, const uint64_t _writes,
	const Breakpoint::Status _breakpointStatus)
-> Addr
{
	if (lineIdx >= DISASM_LINES_MAX) return 0;

	uint8_t opcode = _cmd & 0xFF;
	auto immType = cmdImms[opcode];

	if (immType == CMD_IB_OFF0) {
		return AddDb(_addr, opcode, _consts, _runs, _reads, _writes, _breakpointStatus);
	}

	auto cmdLen = cmdLens[opcode];
	uint16_t data = cmdLen == 1 ? 0 : _cmd>>8;
	data &= cmdLen == 2 ? 0xFF : 0xFFFF;

	auto& line = lines.at(lineIdx);
	line.Init();
	line.type = Line::Type::CODE;
	line.addr = _addr;
	line.opcode = opcode;
	line.imm = data;
	line.accessed = _runs != UINT64_MAX && _reads != UINT64_MAX && _writes != UINT64_MAX;
	line.breakpointStatus = _breakpointStatus;

	if (immType != CMD_IM_NONE) {
		auto labelsI = _labels.find(data);
		auto constsI = _consts.find(data);
		line.labels = labelsI == _labels.end() ? nullptr : &(labelsI->second);
		line.consts = constsI == _consts.end() ? nullptr : &(constsI->second);
	}

	snprintf(line.statsS, sizeof(line.statsS), "%zu,%zu,%zu", _runs, _reads, _writes);

	lineIdx++;
	return cmdLen;
}

auto dev::Disasm::Line::GetStr() const
-> std::string
{
	// print an addr
	std::string out = std::format("{} ", GetAddrS());

	switch (type)
	{
	case Type::CODE:
	{
		auto mnemonic = mnenomics[opcode];
		auto mnemonicLen = mnenomicLens[opcode];
		auto mnemonicType = mnenomicTypes[opcode];
		auto immType = cmdImms[opcode];
		// print a mnemonic
		for (int i = 0; i < mnemonicLen; i++)
		{
			switch (mnemonicType[i])
			{
			case MNT_CMD:
				out += std::format("{}", mnemonic[i]);
				break;

			case MNT_IMM:
				out += std::format(" 0x{}", mnemonic[i]);
				break;

			case MNT_REG:
				out += std::format(" {}", mnemonic[i]);
				break;
			}
			// draw an operand separator
			if (i == 1 && (mnemonicLen == 3 || immType == CMD_IB_OFF1 || immType == CMD_IW_OFF1)) out += ",";
		}
		// print an immediate operand
		if (immType != CMD_IM_NONE)
		{
			out += std::format(" {}", GetImmediateS());
		}
		return out;
	}
	case Type::LABELS: 
		for (auto& label : *labels)
		{
			out += std::format("{} ", label);
		}
		return out;

	case Type::COMMENT:
		out += std::format("; ", *comment);
		return out;
	}

	return "";
}

void dev::Disasm::Init(const LineIdx _linesNum)
{
	linesNum = dev::Min(_linesNum, DISASM_LINES_MAX);
	lineIdx = 0;
	immAddrlinkNum = 0;
}

auto dev::Disasm::GetImmLinks() -> const ImmAddrLinks* 
{ 
	Addr addrMin = lines.at(0).addr;
	Addr addrMax = lines.at(linesNum - 1).addr;
	uint8_t linkIdx = 0;

	std::map<Addr, int> immAddrPairs;
	// aggregate <Addr, LineIdx> pairs
	for (int i = 0; i < linesNum; i++){
		immAddrPairs.emplace(lines.at(i).addr, i);
	}
	// generate links
	for (int i = 0; i < linesNum; i++)
	{
		const auto& line = lines.at(i);
		if (line.type != Line::Type::CODE || opcodeTypes[line.opcode] > OPTYPE_JMP ||
			line.imm < addrMin || line.imm > addrMax)
		{
			immAddrLinks[i].lineIdx = IMM_NO_LINK;
			continue;
		}/*
		if (line.imm < addrMin) {
			immAddrLinks[i].lineIdx = IMM_LINK_UP;
			immAddrLinks[i].linkIdx = linkIdx++;
			continue;
		}
		if (line.imm > addrMax) {
			immAddrLinks[i].lineIdx = IMM_LINK_DOWN;
			immAddrLinks[i].linkIdx = linkIdx++;
			continue;
		}
		*/
		auto linkIdxI = immAddrPairs.find(lines[i].imm);
		if (linkIdxI == immAddrPairs.end()) continue;

		immAddrLinks[i].lineIdx = immAddrPairs[lines[i].imm];
		immAddrLinks[i].linkIdx = linkIdx++;
		immAddrlinkNum++;
	}

	return &immAddrLinks; 
}