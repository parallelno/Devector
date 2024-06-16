#include "CpuI8080.h"
#include "Utils/Utils.h"

// consts
static constexpr uint8_t OPCODE_RST7 = 0xff;
// a number of clock cycles one machine cycle takes
static constexpr uint64_t MACHINE_CC = 4; 
// machine_cycle index indicating the instruction executon is over
static constexpr uint8_t FIRST_MACHINE_CICLE_IDX = 0; 
static constexpr uint16_t PSW_INIT = 0b00000010;
static constexpr uint16_t PSW_NUL_FLAGS = !0b00101000;


#define CC			state.cc
#define PC			state.regs.pc
#define SP			state.regs.sp
#define IR			state.regs.ir
#define TMP			state.regs.tmp
#define ACT			state.regs.act
#define W			state.regs.w
#define Z			state.regs.z

#define A			state.regs.psw.af.h
#define F			state.regs.psw.af.l
#define PSW			state.regs.psw.af.word
#define BC			state.regs.bc.word
#define DE			state.regs.de.word
#define HL			state.regs.hl.word
#define B			state.regs.bc.h
#define C			state.regs.bc.l
#define D			state.regs.de.h
#define E			state.regs.de.l
#define H			state.regs.hl.h
#define L			state.regs.hl.l

#define FC			state.regs.psw.c
#define FP			state.regs.psw.p
#define FAC			state.regs.psw.ac
#define FZ			state.regs.psw.z
#define FS			state.regs.psw.s

#define INTS		state.ints.data
#define INTE		state.ints.inte
#define IFF			state.ints.iff
#define HLTA		state.ints.hlta
#define EI_PENDING	state.ints.eiPending
#define MC			state.ints.mc


dev::CpuI8080::CpuI8080(
	Memory& _memory,
	InputFunc _input, 
	OutputFunc _output)
	:
	m_memory(_memory),
	Input(_input),
	Output(_output)
{
	Init();
}

void dev::CpuI8080::Init()
{
	PSW = PSW_INIT;
	BC = DE = HL = 1;

	Reset();
}

void dev::CpuI8080::Reset()
{
	CC = PC = SP = IR = TMP = ACT = W = Z = INTS = 0;
	F = PSW_INIT;
}

void dev::CpuI8080::ExecuteMachineCycle(bool _irq)
{
	IFF |= _irq & INTE;

	if (MC == 0)
	{
		// interrupt processing
		if (IFF && !EI_PENDING)
		{
			INTE = false;
			IFF = false;
			HLTA = false;
			IR = OPCODE_RST7;
		}
		// normal instruction execution
		else
		{
			EI_PENDING = false;
			IR = ReadInstrMovePC();
		}
	}

	Decode();
	CC += MACHINE_CC;
}

bool dev::CpuI8080::IsInstructionExecuted() const
{
	return MC == FIRST_MACHINE_CICLE_IDX || HLTA;
}

// an instruction execution time in macine cycles. each machine cicle is 4 cc
static constexpr uint8_t M_CYCLES[]
{
//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	1, 3, 2, 2, 2, 2, 2, 1, 1, 3, 2, 2, 2, 2, 2, 1, // 0
	1, 3, 2, 2, 2, 2, 2, 1, 1, 3, 2, 2, 2, 2, 2, 1, // 1
	1, 3, 5, 2, 2, 2, 2, 1, 1, 3, 5, 2, 2, 2, 2, 1, // 2
	1, 3, 4, 2, 3, 3, 3, 1, 1, 3, 4, 2, 2, 2, 2, 1, // 3

	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 4
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 5
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 6
	2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 7

	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 8
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 9
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // A
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // B

	4, 3, 3, 3, 6, 4, 2, 4, 4, 3, 3, 3, 6, 6, 2, 4, // C
	4, 3, 3, 3, 6, 4, 2, 4, 4, 3, 3, 3, 6, 6, 2, 4, // D
	4, 3, 3, 6, 6, 4, 2, 4, 4, 2, 3, 1, 6, 6, 2, 4, // E
	4, 3, 3, 1, 6, 4, 2, 4, 4, 2, 3, 1, 6, 6, 2, 4  // F
};

void dev::CpuI8080::Decode()
{
	switch (IR)
	{
		case 0x7F: MOVRegReg(A, A); break; // MOV A,A
		case 0x78: MOVRegReg(A, B); break; // MOV A,B
		case 0x79: MOVRegReg(A, C); break; // MOV A,C
		case 0x7A: MOVRegReg(A, D); break; // MOV A,D
		case 0x7B: MOVRegReg(A, E); break; // MOV A,E
		case 0x7C: MOVRegReg(A, H); break; // MOV A,H
		case 0x7D: MOVRegReg(A, L); break; // MOV A,L

		case 0x47: MOVRegReg(B, A); break; // MOV B,A
		case 0x40: MOVRegReg(B, B); break; // MOV B,B
		case 0x41: MOVRegReg(B, C); break; // MOV B,C
		case 0x42: MOVRegReg(B, D); break; // MOV B,D
		case 0x43: MOVRegReg(B, E); break; // MOV B,E
		case 0x44: MOVRegReg(B, H); break; // MOV B,H
		case 0x45: MOVRegReg(B, L); break; // MOV B,L

		case 0x4F: MOVRegReg(C, A); break; // MOV C,A
		case 0x48: MOVRegReg(C, B); break; // MOV C,B
		case 0x49: MOVRegReg(C, C); break; // MOV C,C
		case 0x4A: MOVRegReg(C, D); break; // MOV C,D
		case 0x4B: MOVRegReg(C, E); break; // MOV C,E
		case 0x4C: MOVRegReg(C, H); break; // MOV C,H
		case 0x4D: MOVRegReg(C, L); break; // MOV C,L

		case 0x57: MOVRegReg(D, A); break; // MOV D,A
		case 0x50: MOVRegReg(D, B); break; // MOV D,B
		case 0x51: MOVRegReg(D, C); break; // MOV D,C
		case 0x52: MOVRegReg(D, D); break; // MOV D,D
		case 0x53: MOVRegReg(D, E); break; // MOV D,E
		case 0x54: MOVRegReg(D, H); break; // MOV D,H
		case 0x55: MOVRegReg(D, L); break; // MOV D,L

		case 0x5F: MOVRegReg(E, A); break; // MOV E,A
		case 0x58: MOVRegReg(E, B); break; // MOV E,B
		case 0x59: MOVRegReg(E, C); break; // MOV E,C
		case 0x5A: MOVRegReg(E, D); break; // MOV E,D
		case 0x5B: MOVRegReg(E, E); break; // MOV E,E
		case 0x5C: MOVRegReg(E, H); break; // MOV E,H
		case 0x5D: MOVRegReg(E, L); break; // MOV E,L

		case 0x67: MOVRegReg(H, A); break; // MOV H,A
		case 0x60: MOVRegReg(H, B); break; // MOV H,B
		case 0x61: MOVRegReg(H, C); break; // MOV H,C
		case 0x62: MOVRegReg(H, D); break; // MOV H,D
		case 0x63: MOVRegReg(H, E); break; // MOV H,E
		case 0x64: MOVRegReg(H, H); break; // MOV H,H
		case 0x65: MOVRegReg(H, L); break; // MOV H,L

		case 0x6F: MOVRegReg(L, A); break; // MOV L,A
		case 0x68: MOVRegReg(L, B); break; // MOV L,B
		case 0x69: MOVRegReg(L, C); break; // MOV L,C
		case 0x6A: MOVRegReg(L, D); break; // MOV L,D
		case 0x6B: MOVRegReg(L, E); break; // MOV L,E
		case 0x6C: MOVRegReg(L, H); break; // MOV L,H
		case 0x6D: MOVRegReg(L, L); break; // MOV L,L

		case 0x7E: LoadRegPtr(A, HL); break; // MOV A,M
		case 0x46: LoadRegPtr(B, HL); break; // MOV B,M
		case 0x4E: LoadRegPtr(C, HL); break; // MOV C,M
		case 0x56: LoadRegPtr(D, HL); break; // MOV D,M
		case 0x5E: LoadRegPtr(E, HL); break; // MOV E,M
		case 0x66: LoadRegPtr(H, HL); break; // MOV H,M
		case 0x6E: LoadRegPtr(L, HL); break; // MOV L,M

		case 0x77: MOVMemReg(A); break; // MOV M,A
		case 0x70: MOVMemReg(B); break; // MOV M,B
		case 0x71: MOVMemReg(C); break; // MOV M,C
		case 0x72: MOVMemReg(D); break; // MOV M,D
		case 0x73: MOVMemReg(E); break; // MOV M,E
		case 0x74: MOVMemReg(H); break; // MOV M,H
		case 0x75: MOVMemReg(L); break; // MOV M,L

		case 0x3E: MVIRegData(A); break; // MVI A,uint8_t
		case 0x06: MVIRegData(B); break; // MVI B,uint8_t
		case 0x0E: MVIRegData(C); break; // MVI C,uint8_t
		case 0x16: MVIRegData(D); break; // MVI D,uint8_t
		case 0x1E: MVIRegData(E); break; // MVI E,uint8_t
		case 0x26: MVIRegData(H); break; // MVI H,uint8_t
		case 0x2E: MVIRegData(L); break; // MVI L,uint8_t
		case 0x36: MVIMemData(); break; // MVI M,uint8_t

		case 0x0A: LoadRegPtr(A, BC); break; // LDAX B
		case 0x1A: LoadRegPtr(A, DE); break; // LDAX D
		case 0x3A: LDA(); break; // LDA word

		case 0x02: STAX(BC); break; // STAX B
		case 0x12: STAX(DE); break; // STAX D
		case 0x32: STA(); break; // STA word

		case 0x01: LXI(B, C); break; // LXI B,word
		case 0x11: LXI(D, E); break; // LXI D,word
		case 0x21: LXI(H, L); break; // LXI H,word
		case 0x31: LXISP(); break; // LXI SP,word
		case 0x2A: LHLD(); break; // LHLD
		case 0x22: SHLD(); break; // SHLD
		case 0xF9: SPHL(); break; // SPHL

		case 0xEB: XCHG(); break; // XCHG
		case 0xE3: XTHL(); break; // XTHL

		case 0xC5: PUSH(B, C); break; // PUSH B
		case 0xD5: PUSH(D, E); break; // PUSH D
		case 0xE5: PUSH(H, L); break; // PUSH H
		case 0xF5: PUSH(A, F); break; // PUSH PSW
		case 0xC1: POP(B, C); break; // POP B
		case 0xD1: POP(D, E); break; // POP D
		case 0xE1: POP(H, L); break; // POP H
		case 0xF1: POP(A, F); break; // POP PSW

		case 0x87: ADD(A, A, false); break; // ADD A
		case 0x80: ADD(A, B, false); break; // ADD B
		case 0x81: ADD(A, C, false); break; // ADD C
		case 0x82: ADD(A, D, false); break; // ADD D
		case 0x83: ADD(A, E, false); break; // ADD E
		case 0x84: ADD(A, H, false); break; // ADD H
		case 0x85: ADD(A, L, false); break; // ADD L
		case 0x86: ADDMem(false); break; // ADD M
		case 0xC6: ADI(false); break; // ADI uint8_t

		case 0x8F: ADD(A, A, FC); break; // ADC A
		case 0x88: ADD(A, B, FC); break; // ADC B
		case 0x89: ADD(A, C, FC); break; // ADC C
		case 0x8A: ADD(A, D, FC); break; // ADC D
		case 0x8B: ADD(A, E, FC); break; // ADC E
		case 0x8C: ADD(A, H, FC); break; // ADC H
		case 0x8D: ADD(A, L, FC); break; // ADC L
		case 0x8E: ADDMem(FC); break; // ADC M
		case 0xCE: ADI(FC); break; // ACI uint8_t

		case 0x97: SUB(A, A, false); break; // SUB A
		case 0x90: SUB(A, B, false); break; // SUB B
		case 0x91: SUB(A, C, false); break; // SUB C
		case 0x92: SUB(A, D, false); break; // SUB D
		case 0x93: SUB(A, E, false); break; // SUB E
		case 0x94: SUB(A, H, false); break; // SUB H
		case 0x95: SUB(A, L, false); break; // SUB L
		case 0x96: SUBMem(false); break; // SUB M
		case 0xD6: SBI(false); break; // SUI uint8_t

		case 0x9F: SUB(A, A, FC); break; // SBB A
		case 0x98: SUB(A, B, FC); break; // SBB B
		case 0x99: SUB(A, C, FC); break; // SBB C
		case 0x9A: SUB(A, D, FC); break; // SBB D
		case 0x9B: SUB(A, E, FC); break; // SBB E
		case 0x9C: SUB(A, H, FC); break; // SBB H
		case 0x9D: SUB(A, L, FC); break; // SBB L
		case 0x9E: SUBMem(FC); break; // SBB M
		case 0xDE: SBI(FC); break; // SBI uint8_t

		case 0x09: DAD(BC); break; // DAD B
		case 0x19: DAD(DE); break; // DAD D
		case 0x29: DAD(HL); break; // DAD H
		case 0x39: DAD(SP); break; // DAD SP

		case 0x3C: INR(A); break; // INR A
		case 0x04: INR(B); break; // INR B
		case 0x0C: INR(C); break; // INR C
		case 0x14: INR(D); break; // INR D
		case 0x1C: INR(E); break; // INR E
		case 0x24: INR(H); break; // INR H
		case 0x2C: INR(L); break; // INR L
		case 0x34: INRMem(); break; // INR M

		case 0x3D: DCR(A); break; // DCR A
		case 0x05: DCR(B); break; // DCR B
		case 0x0D: DCR(C); break; // DCR C
		case 0x15: DCR(D); break; // DCR D
		case 0x1D: DCR(E); break; // DCR E
		case 0x25: DCR(H); break; // DCR H
		case 0x2D: DCR(L); break; // DCR L
		case 0x35: DCRMem(); break; // DCR M

		case 0x03: INX(B, C); break; // INX B
		case 0x13: INX(D, E); break; // INX D
		case 0x23: INX(H, L); break; // INX H
		case 0x33: INXSP(); break; // INX SP

		case 0x0B: DCX(B, C); break; // DCX B
		case 0x1B: DCX(D, E); break; // DCX D
		case 0x2B: DCX(H, L); break; // DCX H
		case 0x3B: DCXSP(); break; // DCX SP

		case 0x27: DAA(); break; // DAA
		case 0x2F: A = ~A; break; // CMA
		case 0x37: FC = true; break; // STC
		case 0x3F: FC = !FC; break; // CMC

		case 0x07: RLC(); break; // RLC (rotate left)
		case 0x0F: RRC(); break; // RRC (rotate right)
		case 0x17: RAL(); break; // RAL
		case 0x1F: RAR(); break; // RAR

		case 0xA7: ANA(A); break; // ANA A
		case 0xA0: ANA(B); break; // ANA B
		case 0xA1: ANA(C); break; // ANA C
		case 0xA2: ANA(D); break; // ANA D
		case 0xA3: ANA(E); break; // ANA E
		case 0xA4: ANA(H); break; // ANA H
		case 0xA5: ANA(L); break; // ANA L
		case 0xA6: AMAMem(); break; // ANA M
		case 0xE6: ANI(); break; // ANI uint8_t

		case 0xAF: XRA(A); break; // XRA A
		case 0xA8: XRA(B); break; // XRA B
		case 0xA9: XRA(C); break; // XRA C
		case 0xAA: XRA(D); break; // XRA D
		case 0xAB: XRA(E); break; // XRA E
		case 0xAC: XRA(H); break; // XRA H
		case 0xAD: XRA(L); break; // XRA L
		case 0xAE: XRAMem(); break; // XRA M
		case 0xEE: XRI(); break; // XRI uint8_t

		case 0xB7: ORA(A); break; // ORA A
		case 0xB0: ORA(B); break; // ORA B
		case 0xB1: ORA(C); break; // ORA C
		case 0xB2: ORA(D); break; // ORA D
		case 0xB3: ORA(E); break; // ORA E
		case 0xB4: ORA(H); break; // ORA H
		case 0xB5: ORA(L); break; // ORA L
		case 0xB6: ORAMem(); break; // ORA M
		case 0xF6: ORI(); break; // ORI uint8_t

		case 0xBF: CMP(A); break; // CMP A
		case 0xB8: CMP(B); break; // CMP B
		case 0xB9: CMP(C); break; // CMP C
		case 0xBA: CMP(D); break; // CMP D
		case 0xBB: CMP(E); break; // CMP E
		case 0xBC: CMP(H); break; // CMP H
		case 0xBD: CMP(L); break; // CMP L
		case 0xBE: CMPMem(); break; // CMP M
		case 0xFE: CPI(); break; // CPI uint8_t

		case 0xC3: JMP(); break; // JMP
		case 0xCB: JMP(); break; // undocumented JMP
		case 0xC2: JMP(FZ == false); break; // JNZ
		case 0xCA: JMP(FZ == true); break; // JZ
		case 0xD2: JMP(FC == false); break; // JNC
		case 0xDA: JMP(FC == true); break; // JC
		case 0xE2: JMP(FP == false); break; // JPO
		case 0xEA: JMP(FP == true); break; // JPE
		case 0xF2: JMP(FS == false); break; // JP
		case 0xFA: JMP(FS == true); break; // JM

		case 0xE9: PCHL(); break; // PCHL
		case 0xCD: CALL(); break; // CALL
		case 0xDD: CALL(); break; // undocumented CALL
		case 0xED: CALL(); break; // undocumented CALL
		case 0xFD: CALL(); break; // undocumented CALL

		case 0xC4: CALL(FZ == false); break; // CNZ
		case 0xCC: CALL(FZ == true); break; // CZ
		case 0xD4: CALL(FC == false); break; // CNC
		case 0xDC: CALL(FC == true); break; // CC
		case 0xE4: CALL(FP == false); break; // CPO
		case 0xEC: CALL(FP == true); break; // CPE
		case 0xF4: CALL(FS == false); break; // CP
		case 0xFC: CALL(FS == true); break; // CM

		case 0xC9: RET(); break; // RET
		case 0xD9: RET(); break; // undocumented RET
		case 0xC0: RETCond(FZ == false); break; // RNZ
		case 0xC8: RETCond(FZ == true); break; // RZ
		case 0xD0: RETCond(FC == false); break; // RNC
		case 0xD8: RETCond(FC == true); break; // RC
		case 0xE0: RETCond(FP == false); break; // RPO
		case 0xE8: RETCond(FP == true); break; // RPE
		case 0xF0: RETCond(FS == false); break; // RP
		case 0xF8: RETCond(FS == true); break; // RM

		case 0xC7: RST(0); break; // RST 0
		case 0xCF: RST(1); break; // RST 1
		case 0xD7: RST(2); break; // RST 2
		case 0xDF: RST(3); break; // RST 3
		case 0xE7: RST(4); break; // RST 4
		case 0xEF: RST(5); break; // RST 5
		case 0xF7: RST(6); break; // RST 6
		case 0xFF: RST(7); break; // RST 7

		case 0xDB: IN_(); break; // IN
		case 0xD3: OUT_(); break; // OUT

		case 0xF3: INTE = false; break; // DI
		case 0xFB: INTE = true; EI_PENDING = true; break; // EI
		case 0x76: HLT(); break; // HLT

		case 0x00: break; // NOP
		case 0x08: break; // undocumented NOP
		case 0x10: break; // undocumented NOP
		case 0x18: break; // undocumented NOP
		case 0x20: break; // undocumented NOP
		case 0x28: break; // undocumented NOP
		case 0x30: break; // undocumented NOP
		case 0x38: break; // undocumented NOP


	default:
		dev::Log("Handling undocumented instruction. Opcode: {}", IR);
		dev::Exit("Exit", IR);
		break;
	}
	
	MC++;
	MC %= M_CYCLES[IR];
}


////////////////////////////////////////////////////////////////////////////
//
// Memory helpers
//
////////////////////////////////////////////////////////////////////////////

void dev::CpuI8080::AttachDebugOnReadInstr(DebugOnReadInstrFunc* _funcP) { m_debugOnReadInstr.store(_funcP); }
void dev::CpuI8080::AttachDebugOnRead(DebugOnReadFunc* _funcP) { m_debugOnRead.store(_funcP); }
void dev::CpuI8080::AttachDebugOnWrite(DebugOnWriteFunc* _funcP) { m_debugOnWrite.store(_funcP); }

uint8_t dev::CpuI8080::ReadInstrMovePC()
{
	auto globalAddr = m_memory.GetGlobalAddr(PC, Memory::AddrSpace::RAM);

	uint8_t op_code = m_memory.GetByte(PC, Memory::AddrSpace::RAM);
	uint8_t _dataL = m_memory.GetByte(PC + 1, Memory::AddrSpace::RAM);
	uint8_t _dataH = m_memory.GetByte(PC + 2, Memory::AddrSpace::RAM);

	auto DebugOnReadInstr = m_debugOnReadInstr.load();
	if (DebugOnReadInstr && *DebugOnReadInstr) (*DebugOnReadInstr)(globalAddr, op_code, _dataH, _dataL, HL);
	PC++;
	return op_code;
}

uint8_t dev::CpuI8080::ReadByte(const Addr _addr, Memory::AddrSpace _addrSpace)
{
	auto globalAddr = m_memory.GetGlobalAddr(_addr, _addrSpace);

	auto val = m_memory.GetByte(_addr, _addrSpace);
	auto DebugOnRead = m_debugOnRead.load();
	if (DebugOnRead && *DebugOnRead) (*DebugOnRead)(globalAddr, val);

	return val;
}

void dev::CpuI8080::WriteByte(const Addr _addr, uint8_t _value, Memory::AddrSpace _addrSpace)
{
	auto globalAddr = m_memory.GetGlobalAddr(_addr, _addrSpace);
	
	m_memory.SetByte(_addr, _value, _addrSpace);
	auto DebugOnWrite = m_debugOnWrite.load();
	if (DebugOnWrite && *DebugOnWrite) (*DebugOnWrite)(globalAddr, _value);
}

uint8_t dev::CpuI8080::ReadByteMovePC(Memory::AddrSpace _addrSpace)
{
	auto result = ReadByte(PC, _addrSpace);
	PC++;
	return result;
}


////////////////////////////////////////////////////////////////////////////
//
// Get the status
//
////////////////////////////////////////////////////////////////////////////


auto dev::CpuI8080::GetState() const -> const State& { return state; }
uint16_t dev::CpuI8080::GetPSW() const { return PSW; }
uint16_t dev::CpuI8080::GetBC() const { return BC; }
uint16_t dev::CpuI8080::GetDE() const {	return DE; }
uint16_t dev::CpuI8080::GetHL() const {	return HL; }
uint64_t dev::CpuI8080::GetCC() const { return CC; }
uint16_t dev::CpuI8080::GetPC() const { return PC; }
uint16_t dev::CpuI8080::GetSP() const { return SP; }
uint8_t dev::CpuI8080::GetA() const { return A; }
uint8_t dev::CpuI8080::GetF() const { return F; }
uint8_t dev::CpuI8080::GetB() const { return B; }
uint8_t dev::CpuI8080::GetC() const { return C; }
uint8_t dev::CpuI8080::GetD() const { return D; }
uint8_t dev::CpuI8080::GetE() const { return E; }
uint8_t dev::CpuI8080::GetH() const { return H; }
uint8_t dev::CpuI8080::GetL() const { return L; }
bool dev::CpuI8080::GetFlagS() const {	return FS; }
bool dev::CpuI8080::GetFlagZ() const { return FZ; }
bool dev::CpuI8080::GetFlagAC() const { return FAC; }
bool dev::CpuI8080::GetFlagP() const { return FP; }
bool dev::CpuI8080::GetFlagC() const {	return FC; }
bool dev::CpuI8080::GetINTE() const { return INTE; }
bool dev::CpuI8080::GetIFF() const { return IFF; }
bool dev::CpuI8080::GetHLTA() const { return HLTA; }
auto dev::CpuI8080::GetMachineCycles() const -> uint8_t { return MC; }

////////////////////////////////////////////////////////////////////////////
//
// Instruction helpers
//
////////////////////////////////////////////////////////////////////////////

static constexpr bool parityTable[]
{
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
};
// returns the parity of a uint8_t: 0 if a number of set bits of `val` is odd, else 1
bool dev::CpuI8080::GetParity(uint8_t _val)
{
	return parityTable[_val];
}
// determines if there was a carry between bit 'bit_no' and 'bit_no - 1' during the calculation of 'a + b + cy'.
bool dev::CpuI8080::GetCarry(int _bit_no, uint8_t _a, uint8_t _b, bool _cy)
{
	int result = _a + _b + (_cy ? 1 : 0);
	int carry = result ^ _a ^ _b;
	return (carry & (1 << _bit_no)) != 0;
}

void dev::CpuI8080::SetZSP(uint8_t _val)
{
	FZ = _val == 0;
	FS = (_val >> 7) == 1;
	FP = GetParity(_val);
}

// rotate register A left
void dev::CpuI8080::RLC()
{
	FC = A >> 7 == 1;
	A = (uint8_t)(A << 1);
	A += (uint8_t)(FC ? 1 : 0);
}

// rotate register A right
void dev::CpuI8080::RRC()
{
	FC = (A & 1) == 1;
	A = (uint8_t)(A >> 1);
	A |= (uint8_t)(FC ? 1 << 7 : 0);
}

// rotate register A left with the carry flag
void dev::CpuI8080::RAL()
{
	bool cy = FC;
	FC = A >> 7 == 1;
	A = (uint8_t)(A << 1);
	A |= (uint8_t)(cy ? 1 : 0);
}

// rotate register A right with the carry flag
void dev::CpuI8080::RAR()
{
	bool cy = FC;
	FC = (A & 1) == 1;
	A = (uint8_t)(A >> 1);
	A |= (uint8_t)(cy ? 1 << 7 : 0);
}

void dev::CpuI8080::MOVRegReg(uint8_t& _regDest, uint8_t _regSrc)
{
	switch (MC) {
	case 0:
		TMP = _regSrc;
		return;
	case 1:
		_regDest = TMP;
		return;
	}
}

void dev::CpuI8080::LoadRegPtr(uint8_t& _regDest, Addr _addr)
{
	switch (MC) {
	case 0:
		return;
	case 1:
		_regDest = ReadByte(_addr);
		return;
	}
}

void dev::CpuI8080::MOVMemReg(uint8_t _sss)
{
	switch (MC) {
	case 0:
		TMP = _sss;
		return;
	case 1:
		WriteByte(HL, TMP);
		return;
	}
}

void dev::CpuI8080::MVIRegData(uint8_t& _regDest)
{
	switch (MC) {
	case 0:
		return;
	case 1:
		_regDest = ReadByteMovePC();
		return;
	}
}

void dev::CpuI8080::MVIMemData()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		TMP = ReadByteMovePC();
		return;
	case 2:
		WriteByte(HL, TMP);
		return;
	}
}

void dev::CpuI8080::LDA()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		Z = ReadByteMovePC();
		return;
	case 2:
		W = ReadByteMovePC();
		return;
	case 3:
		A = ReadByte(W << 8 | Z);
		return;
	}
}

void dev::CpuI8080::STA()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		Z = ReadByteMovePC();
		return;
	case 2:
		W = ReadByteMovePC();
		return;
	case 3:
		WriteByte(W << 8 | Z, A);
		return;
	}
}

void dev::CpuI8080::STAX(Addr _addr)
{
	switch (MC) {
	case 0:
		return;
	case 1:
		WriteByte(_addr, A);
		return;
	}
}

void dev::CpuI8080::LXI(uint8_t& _regH, uint8_t& _regL)
{
	switch (MC) {
	case 0:
		return;
	case 1:
		_regL = ReadByteMovePC();
		return;
	case 2:
		_regH = ReadByteMovePC();
		return;
	}
}

void dev::CpuI8080::LXISP()
{
	switch (MC) {
	case 0:
		return;
	case 1:{
			auto _lb = ReadByteMovePC();
			SP = SP & 0xff00 | _lb;
			return;
		}
	case 2: {
			auto _hb = ReadByteMovePC();
			SP = _hb << 8 | SP & 0xff;
			return;
		}
	}
}

void dev::CpuI8080::LHLD()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		Z = ReadByteMovePC();
		return;
	case 2:
		W = ReadByteMovePC();
		return;
	case 3:
		L = ReadByte(W << 8 | Z);
		Z++;
		W += Z == 0 ? 1 : 0;
		return;
	case 4:
		H = ReadByte(W << 8 | Z);
		return;
	}
}

void dev::CpuI8080::SHLD()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		Z = ReadByteMovePC();
		return;
	case 2:
		W = ReadByteMovePC();
		return;
	case 3:
		WriteByte(W << 8 | Z, L);
		Z++;
		W += Z == 0 ? 1 : 0;
		return;
	case 4:
		WriteByte(W << 8 | Z, H);
		return;
	}
}

void dev::CpuI8080::SPHL()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		SP = HL;
		return;
	}
}

void dev::CpuI8080::XCHG()
{
	TMP = D;
	D = H;
	H = TMP;

	TMP = E;
	E = L;
	L = TMP;
}

void dev::CpuI8080::XTHL()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		Z = ReadByte(SP, Memory::AddrSpace::STACK);
		return;
	case 2:
		W = ReadByte(SP + 1u, Memory::AddrSpace::STACK);
		return;
	case 3:
		WriteByte(SP, L, Memory::AddrSpace::STACK);
		return;
	case 4:
		WriteByte(SP + 1u, H, Memory::AddrSpace::STACK);
		return;
	case 5:
		H = W;
		L = Z;
		return;
	}
}

void dev::CpuI8080::PUSH(uint8_t _hb, uint8_t _lb)
{
	switch (MC) {
	case 0:
		SP--;
		return;
	case 1:
		WriteByte(SP, _hb, Memory::AddrSpace::STACK);
		return;
	case 2:
		SP--;
		return;
	case 3:
		WriteByte(SP, _lb, Memory::AddrSpace::STACK);
		return;
	}
}

void dev::CpuI8080::POP(uint8_t& _regH, uint8_t& _regL)
{
	switch (MC) {
	case 0:
		return;
	case 1:
		_regL = ReadByte(SP, Memory::AddrSpace::STACK);
		SP++;
		return;
	case 2:
		_regH = ReadByte(SP, Memory::AddrSpace::STACK);
		SP++;
		return;
	}
}

// adds a value (+ an optional carry flag) to a register
void dev::CpuI8080::ADD(uint8_t _a, uint8_t _b, bool _cy)
{

	A = (uint8_t)(_a + _b + (_cy ? 1 : 0));
	FC = GetCarry(8, _a, _b, _cy);
	FAC = GetCarry(4, _a, _b, _cy);
	SetZSP(A);
}

void dev::CpuI8080::ADDMem(bool _cy)
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1:
		TMP = ReadByte(HL);
		ADD(ACT, TMP, _cy);
		return;
	}
}

void dev::CpuI8080::ADI(bool _cy)
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1:
		TMP = ReadByteMovePC();
		ADD(ACT, TMP, _cy);
		return;
	}
}

// substracts a uint8_t (+ an optional carry flag) from a register
// see https://stackoverflow.com/a/8037485
void dev::CpuI8080::SUB(uint8_t _a, uint8_t _b, bool _cy)
{
	ADD(_a, (uint8_t)(~_b), !_cy);
	FC = !FC;
}

void dev::CpuI8080::SUBMem(bool _cy)
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1:
		TMP = ReadByte(HL);
		SUB(ACT, TMP, _cy);
		return;
	}
}

void dev::CpuI8080::SBI(bool _cy)
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1:
		TMP = ReadByteMovePC();
		SUB(ACT, TMP, _cy);
		return;
	}
}

void dev::CpuI8080::DAD(uint16_t _val)
{
	switch (MC) {
	case 0:
		return;
	case 1: {
			ACT = (uint8_t)(_val & 0xff);
			TMP = L;
			int res = ACT + TMP;
			FC = (res >> 8) == 1;
			L = (uint8_t)(res);
			return;
		}
	case 2: {
			ACT = (uint8_t)(_val >> 8);
			TMP = H;
			int result = ACT + TMP + (FC ? 1 : 0);
			FC = (result >> 8) == 1;
			H = (uint8_t)(result);
			return;
		}
	}
}

void dev::CpuI8080::INR(uint8_t& _regDest)
{
	switch (MC) {
	case 0:
		TMP = _regDest;
		TMP++;
		FAC = (TMP & 0xF) == 0;
		SetZSP(TMP);
		return;
	case 1:
		_regDest = TMP;
		return;
	}
}

void dev::CpuI8080::INRMem()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		TMP = ReadByte(HL);
		TMP++;
		FAC = (TMP & 0xF) == 0;
		SetZSP(TMP);
		return;
	case 2:
		WriteByte(HL, TMP);
		return;
	}
}

void dev::CpuI8080::DCR(uint8_t& _regDest)
{
	switch (MC) {
	case 0:
		TMP = _regDest;
		TMP--;
		FAC = !((TMP & 0xF) == 0xF);
		SetZSP(TMP);
		return;
	case 1:
		_regDest = TMP;
		return;
	}
}

void dev::CpuI8080::DCRMem()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		TMP = ReadByte(HL);
		TMP--;
		FAC = !((TMP & 0xF) == 0xF);
		SetZSP(TMP);
		return;
	case 2:
		WriteByte(HL, TMP);
		return;
	}
}

void dev::CpuI8080::INX(uint8_t& _regH, uint8_t& _regL)
{
	switch (MC) {
	case 0:
		Z = (uint8_t)(_regL + 1);
		W = (uint8_t)(Z == 0 ? _regH + 1 : _regH);
		return;
	case 1:
		_regH = W;
		_regL = Z;
		return;
	}
}

void dev::CpuI8080::INXSP()
{
	switch (MC) {
	case 0:
		Z = (uint8_t)(SP + 1);
		W = (uint8_t)(Z == 0 ? (SP >> 8) + 1 : SP >> 8);
		return;
	case 1:
		SP = W << 8 | Z;
		return;
	}
}

void dev::CpuI8080::DCX(uint8_t& _regH, uint8_t& _regL)
{
	switch (MC) {
	case 0:
		Z = (uint8_t)(_regL - 1);
		W = (uint8_t)(Z == 0xff ? _regH - 1 : _regH);
		return;
	case 1:
		_regH = W;
		_regL = Z;
		return;
	}
}

void dev::CpuI8080::DCXSP()
{
	switch (MC) {
	case 0:
		Z = (uint8_t)(SP - 1);
		W = (uint8_t)(Z == 0xff ? (SP >> 8) - 1 : SP >> 8);
		return;
	case 1:
		SP = W << 8 | Z;
		return;
	}
}

// Decimal Adjust Accumulator: the eight-bit number in register A is adjusted
// to form two four-bit binary-coded-decimal digits.
// For example, if A=$2B and DAA is executed, A becomes $31.
void dev::CpuI8080::DAA()
{
	bool cy = FC;
	uint8_t correction = 0;

	uint8_t lsb = (uint8_t)(A & 0x0F);
	uint8_t msb = (uint8_t)(A >> 4);

	if (FAC || lsb > 9)
	{
		correction += 0x06;
	}

	if (FC || msb > 9 || (msb >= 9 && lsb > 9))
	{
		correction += 0x60;
		cy = true;
	}

	ADD(A, correction, false);
	FC = cy;
}

void dev::CpuI8080::ANA(uint8_t _sss)
{
	ACT = A;
	TMP = _sss;
	A = (uint8_t)(ACT & TMP);
	FC = false;
	FAC = ((ACT | TMP) & 0x08) != 0;
	SetZSP(A);
}

void dev::CpuI8080::AMAMem()
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1:
		TMP = ReadByte(HL);
		A = (uint8_t)(ACT & TMP);
		FC = false;
		FAC = ((ACT | TMP) & 0x08) != 0;
		SetZSP(A);
		return;
	}
}

void dev::CpuI8080::ANI()
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1:
		TMP = ReadByteMovePC();
		A = (uint8_t)(ACT & TMP);
		FC = false;
		FAC = ((ACT | TMP) & 0x08) != 0;
		SetZSP(A);
		return;
	}
}

// executes a logic "xor" between register A and a uint8_t, then stores the
// result in register A
void dev::CpuI8080::XRA(uint8_t _sss)
{
	ACT = A;
	TMP = _sss;
	A = (uint8_t)(ACT ^ TMP);
	FC = false;
	FAC = false;
	SetZSP(A);
}

void dev::CpuI8080::XRAMem()
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1:
		TMP = ReadByte(HL);
		A = (uint8_t)(ACT ^ TMP);
		FC = false;
		FAC = false;
		SetZSP(A);
		return;
	}
}

void dev::CpuI8080::XRI()
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1:
		TMP = ReadByteMovePC();
		A = (uint8_t)(ACT ^ TMP);
		FC = false;
		FAC = false;
		SetZSP(A);
		return;
	}
}

// executes a logic "or" between register A and a uint8_t, then stores the
// result in register A
void dev::CpuI8080::ORA(uint8_t _sss)
{
	ACT = A;
	TMP = _sss;
	A = (uint8_t)(ACT | TMP);
	FC = false;
	FAC = false;
	SetZSP(A);
}

void dev::CpuI8080::ORAMem()
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1:
		TMP = ReadByte(HL);
		A = (uint8_t)(ACT | TMP);
		FC = false;
		FAC = false;
		SetZSP(A);
		return;
	}
}

void dev::CpuI8080::ORI()
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1:
		TMP = ReadByteMovePC();
		A = (uint8_t)(ACT | TMP);
		FC = false;
		FAC = false;
		SetZSP(A);
		return;
	}
}

// compares the register A to another uint8_t
void dev::CpuI8080::CMP(uint8_t _sss)
{
	ACT = A;
	TMP = _sss;
	auto res = ACT - TMP;
	FC = (res >> 8) & 1;
	FAC = (~(ACT ^ (res & 0xFF) ^ TMP) & 0x10) == 0x10;
	SetZSP((uint8_t)(res & 0xFF));
}

void dev::CpuI8080::CMPMem()
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1: {
			TMP = ReadByte(HL);
			int res = ACT - TMP;
			FC = (res >> 8) & 1;
			FAC = (~(ACT ^ (res & 0xFF) ^ TMP) & 0x10) == 0x10;
			SetZSP((uint8_t)(res & 0xFF));
			return;
		}
	}
}

void dev::CpuI8080::CPI()
{
	switch (MC) {
	case 0:
		ACT = A;
		return;
	case 1:
		TMP = ReadByteMovePC();
		auto res = ACT - TMP;
		FC = (res >> 8) & 1;
		FAC = (~(ACT ^ (res & 0xFF) ^ TMP) & 0x10) == 0x10;
		SetZSP((uint8_t)(res & 0xFF));
		return;
	}
}

void dev::CpuI8080::JMP(bool _condition)
{
	switch (MC) {
	case 0:
		return;
	case 1:
		Z = ReadByteMovePC();
		return;
	case 2:
		W = ReadByteMovePC();
		if (_condition)
		{
			PC = (uint16_t)(W << 8 | Z);
		}
		return;
	}
}

void dev::CpuI8080::PCHL()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		PC = HL;
		return;
	}
}

// pushes the current pc to the stack, then jumps to an address
void dev::CpuI8080::CALL(bool _condition)
{
	switch (MC) {
	case 0:
		SP -= _condition ? 1 : 0;
		return;
	case 1:
		Z = ReadByteMovePC();
		return;
	case 2:
		W = ReadByteMovePC();
		return;
	case 3:
		if (_condition)	{
			WriteByte(SP, (uint8_t)(PC >> 8), Memory::AddrSpace::STACK);
			SP--;
		} else {
			// end execution
			MC = 5;
		}
		return;
	case 4:
		WriteByte(SP, (uint8_t)(PC & 0xff), Memory::AddrSpace::STACK);
		return;
	case 5:
		PC = W << 8 | Z;
		return;
	}
}

// pushes the current pc to the stack, then jumps to an address
void dev::CpuI8080::RST(uint8_t _arg)
{
	switch (MC) {
	case 0:
		SP--;
		return;
	case 1:
		WriteByte(SP, (uint8_t)(PC >> 8), Memory::AddrSpace::STACK);
		SP--;
		return;
	case 2:
		W = 0;
		Z = _arg << 3;
		WriteByte(SP, (uint8_t)(PC & 0xff), Memory::AddrSpace::STACK);
		return;
	case 3:
		PC = W << 8 | Z;
		return;
	}
}

// returns from subroutine
void dev::CpuI8080::RET()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		Z = ReadByte(SP, Memory::AddrSpace::STACK);
		SP++;
		return;
	case 2:
		W = ReadByte(SP, Memory::AddrSpace::STACK);
		SP++;
		PC = W << 8 | Z;
		return;
	}
}

// returns from subroutine if a condition is met
void dev::CpuI8080::RETCond(bool _condition)
{
	switch (MC) {
	case 0:
		return;
	case 1:
		if (!_condition) MC = 3;
		return;
	case 2:
		Z = ReadByte(SP, Memory::AddrSpace::STACK);
		SP++;
		return;
	case 3:
		W = ReadByte(SP, Memory::AddrSpace::STACK);
		SP++;
		PC = W << 8 | Z;
		return;
	}
}

void dev::CpuI8080::IN_()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		W = 0;
		Z = ReadByteMovePC();
		return;
	case 2:
		A = Input(Z);
		return;
	}
}

void dev::CpuI8080::OUT_()
{
	switch (MC) {
	case 0:
		return;
	case 1:
		W = 0;
		Z = ReadByteMovePC();
		return;
	case 2:
		Output(Z, A);
		return;
	}
}

void dev::CpuI8080::HLT()
{
	switch (MC) {
	case 0:
		PC--;
		return;
	case 1:
		ReadInstrMovePC();
		// to loop into the M2 of HLT
		if (!IFF) {
			HLTA = true;
			MC--;
			PC--;
		}
		return;
	}
}