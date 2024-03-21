#include "I8080.h"

dev::I8080::I8080(
	Memory& _memory,
	InputFunc _input, 
	OutputFunc _output)
	:
	m_memory(_memory),
	Input(_input),
	Output(_output)
{
	m_flagUnused1 = true;
	m_flagUnused3 = false;
	m_flagUnused5 = false;

	Init();
}

void dev::I8080::Init()
{
	m_a = m_b = m_c = m_d = m_e = m_h = m_l = 0;
	Reset();
}

void dev::I8080::Reset()
{
	m_cc = m_pc = m_sp = 0;
	m_instructionRegister = m_TMP = m_ACT = m_W = m_Z = 0;
	m_flagS = m_flagZ = m_flagAC = m_flagP = m_flagC = m_INTE = false;

	m_machineCycle = 0;
	m_HLTA = m_INTE = m_IFF = m_eiPending = false;
}

void dev::I8080::ExecuteMachineCycle(bool _T50HZ)
{
	m_IFF |= _T50HZ & m_INTE;

	if (m_machineCycle == 0)
	{
		// interrupt processing
		if (m_IFF && !m_eiPending)
		{
			m_INTE = false;
			m_IFF = false;
			m_HLTA = false;
			m_instructionRegister = OPCODE_RST7;
		}
		// normal instruction execution
		else
		{
			m_eiPending = false;
			m_instructionRegister = ReadInstrMovePC();
		}
	}

	Decode();
	m_cc += MACHINE_CC;
}

bool dev::I8080::IsInstructionExecuted() const
{
	return m_machineCycle == I8080::INSTR_EXECUTED || m_HLTA;
}

// an instruction execution time in macine cycles
static constexpr int M_CYCLES[]
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

void dev::I8080::Decode()
{
	switch (m_instructionRegister)
	{
		case 0x7F: MOVRegReg(m_a, m_a); break; // MOV A,A
		case 0x78: MOVRegReg(m_a, m_b); break; // MOV A,B
		case 0x79: MOVRegReg(m_a, m_c); break; // MOV A,C
		case 0x7A: MOVRegReg(m_a, m_d); break; // MOV A,D
		case 0x7B: MOVRegReg(m_a, m_e); break; // MOV A,E
		case 0x7C: MOVRegReg(m_a, m_h); break; // MOV A,H
		case 0x7D: MOVRegReg(m_a, m_l); break; // MOV A,L

		case 0x47: MOVRegReg(m_b, m_a); break; // MOV B,A
		case 0x40: MOVRegReg(m_b, m_b); break; // MOV B,B
		case 0x41: MOVRegReg(m_b, m_c); break; // MOV B,C
		case 0x42: MOVRegReg(m_b, m_d); break; // MOV B,D
		case 0x43: MOVRegReg(m_b, m_e); break; // MOV B,E
		case 0x44: MOVRegReg(m_b, m_h); break; // MOV B,H
		case 0x45: MOVRegReg(m_b, m_l); break; // MOV B,L

		case 0x4F: MOVRegReg(m_c, m_a); break; // MOV C,A
		case 0x48: MOVRegReg(m_c, m_b); break; // MOV C,B
		case 0x49: MOVRegReg(m_c, m_c); break; // MOV C,C
		case 0x4A: MOVRegReg(m_c, m_d); break; // MOV C,D
		case 0x4B: MOVRegReg(m_c, m_e); break; // MOV C,E
		case 0x4C: MOVRegReg(m_c, m_h); break; // MOV C,H
		case 0x4D: MOVRegReg(m_c, m_l); break; // MOV C,L

		case 0x57: MOVRegReg(m_d, m_a); break; // MOV D,A
		case 0x50: MOVRegReg(m_d, m_b); break; // MOV D,B
		case 0x51: MOVRegReg(m_d, m_c); break; // MOV D,C
		case 0x52: MOVRegReg(m_d, m_d); break; // MOV D,D
		case 0x53: MOVRegReg(m_d, m_e); break; // MOV D,E
		case 0x54: MOVRegReg(m_d, m_h); break; // MOV D,H
		case 0x55: MOVRegReg(m_d, m_l); break; // MOV D,L

		case 0x5F: MOVRegReg(m_e, m_a); break; // MOV E,A
		case 0x58: MOVRegReg(m_e, m_b); break; // MOV E,B
		case 0x59: MOVRegReg(m_e, m_c); break; // MOV E,C
		case 0x5A: MOVRegReg(m_e, m_d); break; // MOV E,D
		case 0x5B: MOVRegReg(m_e, m_e); break; // MOV E,E
		case 0x5C: MOVRegReg(m_e, m_h); break; // MOV E,H
		case 0x5D: MOVRegReg(m_e, m_l); break; // MOV E,L

		case 0x67: MOVRegReg(m_h, m_a); break; // MOV H,A
		case 0x60: MOVRegReg(m_h, m_b); break; // MOV H,B
		case 0x61: MOVRegReg(m_h, m_c); break; // MOV H,C
		case 0x62: MOVRegReg(m_h, m_d); break; // MOV H,D
		case 0x63: MOVRegReg(m_h, m_e); break; // MOV H,E
		case 0x64: MOVRegReg(m_h, m_h); break; // MOV H,H
		case 0x65: MOVRegReg(m_h, m_l); break; // MOV H,L

		case 0x6F: MOVRegReg(m_l, m_a); break; // MOV L,A
		case 0x68: MOVRegReg(m_l, m_b); break; // MOV L,B
		case 0x69: MOVRegReg(m_l, m_c); break; // MOV L,C
		case 0x6A: MOVRegReg(m_l, m_d); break; // MOV L,D
		case 0x6B: MOVRegReg(m_l, m_e); break; // MOV L,E
		case 0x6C: MOVRegReg(m_l, m_h); break; // MOV L,H
		case 0x6D: MOVRegReg(m_l, m_l); break; // MOV L,L

		case 0x7E: LoadRegPtr(m_a, GetHL()); break; // MOV A,M
		case 0x46: LoadRegPtr(m_b, GetHL()); break; // MOV B,M
		case 0x4E: LoadRegPtr(m_c, GetHL()); break; // MOV C,M
		case 0x56: LoadRegPtr(m_d, GetHL()); break; // MOV D,M
		case 0x5E: LoadRegPtr(m_e, GetHL()); break; // MOV E,M
		case 0x66: LoadRegPtr(m_h, GetHL()); break; // MOV H,M
		case 0x6E: LoadRegPtr(m_l, GetHL()); break; // MOV L,M

		case 0x77: MOVMemReg(m_a); break; // MOV M,A
		case 0x70: MOVMemReg(m_b); break; // MOV M,B
		case 0x71: MOVMemReg(m_c); break; // MOV M,C
		case 0x72: MOVMemReg(m_d); break; // MOV M,D
		case 0x73: MOVMemReg(m_e); break; // MOV M,E
		case 0x74: MOVMemReg(m_h); break; // MOV M,H
		case 0x75: MOVMemReg(m_l); break; // MOV M,L

		case 0x3E: MVIRegData(m_a); break; // MVI A,uint8_t
		case 0x06: MVIRegData(m_b); break; // MVI B,uint8_t
		case 0x0E: MVIRegData(m_c); break; // MVI C,uint8_t
		case 0x16: MVIRegData(m_d); break; // MVI D,uint8_t
		case 0x1E: MVIRegData(m_e); break; // MVI E,uint8_t
		case 0x26: MVIRegData(m_h); break; // MVI H,uint8_t
		case 0x2E: MVIRegData(m_l); break; // MVI L,uint8_t
		case 0x36: MVIMemData(); break; // MVI M,uint8_t

		case 0x0A: LoadRegPtr(m_a, GetBC()); break; // LDAX B
		case 0x1A: LoadRegPtr(m_a, GetDE()); break; // LDAX D
		case 0x3A: LDA(); break; // LDA word

		case 0x02: STAX(GetBC()); break; // STAX B
		case 0x12: STAX(GetDE()); break; // STAX D
		case 0x32: STA(); break; // STA word

		case 0x01: LXI(m_b, m_c); break; // LXI B,word
		case 0x11: LXI(m_d, m_e); break; // LXI D,word
		case 0x21: LXI(m_h, m_l); break; // LXI H,word
		case 0x31: LXISP(); break; // LXI SP,word
		case 0x2A: LHLD(); break; // LHLD
		case 0x22: SHLD(); break; // SHLD
		case 0xF9: SPHL(); break; // SPHL

		case 0xEB: XCHG(); break; // XCHG
		case 0xE3: XTHL(); break; // XTHL

		case 0xC5: PUSH(m_b, m_c); break; // PUSH B
		case 0xD5: PUSH(m_d, m_e); break; // PUSH D
		case 0xE5: PUSH(m_h, m_l); break; // PUSH H
		case 0xF5: PUSH(m_a, GetFlags()); break; // PUSH PSW
		case 0xC1: POP(m_b, m_c); break; // POP B
		case 0xD1: POP(m_d, m_e); break; // POP D
		case 0xE1: POP(m_h, m_l); break; // POP H
		case 0xF1: POP(m_a, m_TMP); SetFlags(m_TMP); break; // POP PSW

		case 0x87: ADD(m_a, m_a, false); break; // ADD A
		case 0x80: ADD(m_a, m_b, false); break; // ADD B
		case 0x81: ADD(m_a, m_c, false); break; // ADD C
		case 0x82: ADD(m_a, m_d, false); break; // ADD D
		case 0x83: ADD(m_a, m_e, false); break; // ADD E
		case 0x84: ADD(m_a, m_h, false); break; // ADD H
		case 0x85: ADD(m_a, m_l, false); break; // ADD L
		case 0x86: ADDMem(false); break; // ADD M
		case 0xC6: ADI(false); break; // ADI uint8_t

		case 0x8F: ADD(m_a, m_a, m_flagC); break; // ADC A
		case 0x88: ADD(m_a, m_b, m_flagC); break; // ADC B
		case 0x89: ADD(m_a, m_c, m_flagC); break; // ADC C
		case 0x8A: ADD(m_a, m_d, m_flagC); break; // ADC D
		case 0x8B: ADD(m_a, m_e, m_flagC); break; // ADC E
		case 0x8C: ADD(m_a, m_h, m_flagC); break; // ADC H
		case 0x8D: ADD(m_a, m_l, m_flagC); break; // ADC L
		case 0x8E: ADDMem(m_flagC); break; // ADC M
		case 0xCE: ADI(m_flagC); break; // ACI uint8_t

		case 0x97: SUB(m_a, m_a, false); break; // SUB A
		case 0x90: SUB(m_a, m_b, false); break; // SUB B
		case 0x91: SUB(m_a, m_c, false); break; // SUB C
		case 0x92: SUB(m_a, m_d, false); break; // SUB D
		case 0x93: SUB(m_a, m_e, false); break; // SUB E
		case 0x94: SUB(m_a, m_h, false); break; // SUB H
		case 0x95: SUB(m_a, m_l, false); break; // SUB L
		case 0x96: SUBMem(false); break; // SUB M
		case 0xD6: SBI(false); break; // SUI uint8_t

		case 0x9F: SUB(m_a, m_a, m_flagC); break; // SBB A
		case 0x98: SUB(m_a, m_b, m_flagC); break; // SBB B
		case 0x99: SUB(m_a, m_c, m_flagC); break; // SBB C
		case 0x9A: SUB(m_a, m_d, m_flagC); break; // SBB D
		case 0x9B: SUB(m_a, m_e, m_flagC); break; // SBB E
		case 0x9C: SUB(m_a, m_h, m_flagC); break; // SBB H
		case 0x9D: SUB(m_a, m_l, m_flagC); break; // SBB L
		case 0x9E: SUBMem(m_flagC); break; // SBB M
		case 0xDE: SBI(m_flagC); break; // SBI uint8_t

		case 0x09: DAD(GetBC()); break; // DAD B
		case 0x19: DAD(GetDE()); break; // DAD D
		case 0x29: DAD(GetHL()); break; // DAD H
		case 0x39: DAD(m_sp); break; // DAD SP

		case 0x3C: INR(m_a); break; // INR A
		case 0x04: INR(m_b); break; // INR B
		case 0x0C: INR(m_c); break; // INR C
		case 0x14: INR(m_d); break; // INR D
		case 0x1C: INR(m_e); break; // INR E
		case 0x24: INR(m_h); break; // INR H
		case 0x2C: INR(m_l); break; // INR L
		case 0x34: INRMem(); break; // INR M

		case 0x3D: DCR(m_a); break; // DCR A
		case 0x05: DCR(m_b); break; // DCR B
		case 0x0D: DCR(m_c); break; // DCR C
		case 0x15: DCR(m_d); break; // DCR D
		case 0x1D: DCR(m_e); break; // DCR E
		case 0x25: DCR(m_h); break; // DCR H
		case 0x2D: DCR(m_l); break; // DCR L
		case 0x35: DCRMem(); break; // DCR M

		case 0x03: INX(m_b, m_c); break; // INX B
		case 0x13: INX(m_d, m_e); break; // INX D
		case 0x23: INX(m_h, m_l); break; // INX H
		case 0x33: INXSP(); break; // INX SP

		case 0x0B: DCX(m_b, m_c); break; // DCX B
		case 0x1B: DCX(m_d, m_e); break; // DCX D
		case 0x2B: DCX(m_h, m_l); break; // DCX H
		case 0x3B: DCXSP(); break; // DCX SP

		case 0x27: DAA(); break; // DAA
		case 0x2F: m_a = ~m_a; break; // CMA
		case 0x37: m_flagC = true; break; // STC
		case 0x3F: m_flagC = !m_flagC; break; // CMC

		case 0x07: RLC(); break; // RLC (rotate left)
		case 0x0F: RRC(); break; // RRC (rotate right)
		case 0x17: RAL(); break; // RAL
		case 0x1F: RAR(); break; // RAR

		case 0xA7: ANA(m_a); break; // ANA A
		case 0xA0: ANA(m_b); break; // ANA B
		case 0xA1: ANA(m_c); break; // ANA C
		case 0xA2: ANA(m_d); break; // ANA D
		case 0xA3: ANA(m_e); break; // ANA E
		case 0xA4: ANA(m_h); break; // ANA H
		case 0xA5: ANA(m_l); break; // ANA L
		case 0xA6: AMAMem(); break; // ANA M
		case 0xE6: ANI(); break; // ANI uint8_t

		case 0xAF: XRA(m_a); break; // XRA A
		case 0xA8: XRA(m_b); break; // XRA B
		case 0xA9: XRA(m_c); break; // XRA C
		case 0xAA: XRA(m_d); break; // XRA D
		case 0xAB: XRA(m_e); break; // XRA E
		case 0xAC: XRA(m_h); break; // XRA H
		case 0xAD: XRA(m_l); break; // XRA L
		case 0xAE: XRAMem(); break; // XRA M
		case 0xEE: XRI(); break; // XRI uint8_t

		case 0xB7: ORA(m_a); break; // ORA A
		case 0xB0: ORA(m_b); break; // ORA B
		case 0xB1: ORA(m_c); break; // ORA C
		case 0xB2: ORA(m_d); break; // ORA D
		case 0xB3: ORA(m_e); break; // ORA E
		case 0xB4: ORA(m_h); break; // ORA H
		case 0xB5: ORA(m_l); break; // ORA L
		case 0xB6: ORAMem(); break; // ORA M
		case 0xF6: ORI(); break; // ORI uint8_t

		case 0xBF: CMP(m_a); break; // CMP A
		case 0xB8: CMP(m_b); break; // CMP B
		case 0xB9: CMP(m_c); break; // CMP C
		case 0xBA: CMP(m_d); break; // CMP D
		case 0xBB: CMP(m_e); break; // CMP E
		case 0xBC: CMP(m_h); break; // CMP H
		case 0xBD: CMP(m_l); break; // CMP L
		case 0xBE: CMPMem(); break; // CMP M
		case 0xFE: CPI(); break; // CPI uint8_t

		case 0xC3: JMP(); break; // JMP
		case 0xCB: JMP(); break; // undocumented JMP
		case 0xC2: JMP(m_flagZ == false); break; // JNZ
		case 0xCA: JMP(m_flagZ == true); break; // JZ
		case 0xD2: JMP(m_flagC == false); break; // JNC
		case 0xDA: JMP(m_flagC == true); break; // JC
		case 0xE2: JMP(m_flagP == false); break; // JPO
		case 0xEA: JMP(m_flagP == true); break; // JPE
		case 0xF2: JMP(m_flagS == false); break; // JP
		case 0xFA: JMP(m_flagS == true); break; // JM

		case 0xE9: PCHL(); break; // PCHL
		case 0xCD: CALL(); break; // CALL
		case 0xDD: CALL(); break; // undocumented CALL
		case 0xED: CALL(); break; // undocumented CALL
		case 0xFD: CALL(); break; // undocumented CALL

		case 0xC4: CALL(m_flagZ == false); break; // CNZ
		case 0xCC: CALL(m_flagZ == true); break; // CZ
		case 0xD4: CALL(m_flagC == false); break; // CNC
		case 0xDC: CALL(m_flagC == true); break; // CC
		case 0xE4: CALL(m_flagP == false); break; // CPO
		case 0xEC: CALL(m_flagP == true); break; // CPE
		case 0xF4: CALL(m_flagS == false); break; // CP
		case 0xFC: CALL(m_flagS == true); break; // CM

		case 0xC9: RET(); break; // RET
		case 0xD9: RET(); break; // undocumented RET
		case 0xC0: RETCond(m_flagZ == false); break; // RNZ
		case 0xC8: RETCond(m_flagZ == true); break; // RZ
		case 0xD0: RETCond(m_flagC == false); break; // RNC
		case 0xD8: RETCond(m_flagC == true); break; // RC
		case 0xE0: RETCond(m_flagP == false); break; // RPO
		case 0xE8: RETCond(m_flagP == true); break; // RPE
		case 0xF0: RETCond(m_flagS == false); break; // RP
		case 0xF8: RETCond(m_flagS == true); break; // RM

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

		case 0xF3: m_INTE = false; break; // DI
		case 0xFB: m_INTE = true; m_eiPending = true; break; // EI
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
		break;
	}
	
	m_machineCycle++;
	m_machineCycle %= M_CYCLES[m_instructionRegister];
}


////////////////////////////////////////////////////////////////////////////
//
// Memory helpers
//
////////////////////////////////////////////////////////////////////////////

void dev::I8080::AttachDebugOnReadInstr(DebugOnReadInstrFunc* _funcP) { m_debugOnReadInstr.store(_funcP); }
void dev::I8080::AttachDebugOnRead(DebugOnReadFunc* _funcP) { m_debugOnRead.store(_funcP); }
void dev::I8080::AttachDebugOnWrite(DebugOnWriteFunc* _funcP) { m_debugOnWrite.store(_funcP); }

uint8_t dev::I8080::ReadInstrMovePC()
{
	auto globalAddr = m_memory.GetGlobalAddr(m_pc, Memory::AddrSpace::RAM);

	uint8_t op_code = m_memory.GetByte(m_pc, Memory::AddrSpace::RAM);
	uint8_t _dataL = m_memory.GetByte(m_pc + 1, Memory::AddrSpace::RAM);
	uint8_t _dataH = m_memory.GetByte(m_pc + 2, Memory::AddrSpace::RAM);

	auto DebugOnReadInstr = m_debugOnReadInstr.load();
	if (DebugOnReadInstr && *DebugOnReadInstr) (*DebugOnReadInstr)(globalAddr, op_code, _dataH, _dataL, GetHL());
	m_pc++;
	return op_code;
}

uint8_t dev::I8080::ReadByte(const Addr _addr, Memory::AddrSpace _addrSpace)
{
	auto globalAddr = m_memory.GetGlobalAddr(_addr, _addrSpace);

	auto val = m_memory.GetByte(_addr, _addrSpace);
	auto DebugOnRead = m_debugOnRead.load();
	if (DebugOnRead && *DebugOnRead) (*DebugOnRead)(globalAddr, val);

	return val;
}

void dev::I8080::WriteByte(const Addr _addr, uint8_t _value, Memory::AddrSpace _addrSpace)
{
	auto globalAddr = m_memory.GetGlobalAddr(_addr, _addrSpace);
	
	m_memory.SetByte(_addr, _value, _addrSpace);
	auto DebugOnWrite = m_debugOnWrite.load();
	if (DebugOnWrite && *DebugOnWrite) (*DebugOnWrite)(globalAddr, _value);
}

uint8_t dev::I8080::ReadByteMovePC(Memory::AddrSpace _addrSpace)
{
	auto result = ReadByte(m_pc, _addrSpace);
	m_pc++;
	return result;
}


////////////////////////////////////////////////////////////////////////////
//
// Register helpers
//
////////////////////////////////////////////////////////////////////////////

uint8_t dev::I8080::GetFlags() const
{
	int psw = 0;
	psw |= m_flagS ? 1 << 7 : 0;
	psw |= m_flagZ ? 1 << 6 : 0;
	psw |= m_flagAC ? 1 << 4 : 0;
	psw |= m_flagP ? 1 << 2 : 0;
	psw |= m_flagC ? 1 << 0 : 0;

	psw |= m_flagUnused1 ? 1 << 1 : 0;
	psw |= m_flagUnused3 ? 1 << 3 : 0;
	psw |= m_flagUnused5 ? 1 << 5 : 0;
	return (uint8_t)psw;
}

uint16_t dev::I8080::GetAF() const
{
	return (uint16_t)(m_a << 8 | GetFlags());
}

void dev::I8080::SetFlags(uint8_t psw)
{
	m_flagS = ((psw >> 7) & 1) == 1;
	m_flagZ = ((psw >> 6) & 1) == 1;
	m_flagAC = ((psw >> 4) & 1) == 1;
	m_flagP = ((psw >> 2) & 1) == 1;
	m_flagC = ((psw >> 0) & 1) == 1;

	m_flagUnused1 = true;
	m_flagUnused3 = false;
	m_flagUnused5 = false;
}

uint16_t dev::I8080::GetBC() const
{ 
	return (m_b << 8) | m_c; 
}

void dev::I8080::SetBC(uint16_t val)
{
	m_b = (uint8_t)(val >> 8);
	m_c = (uint8_t)(val & 0xFF);
}

uint16_t dev::I8080::GetDE() const
{
	return (uint16_t)((m_d << 8) | m_e);
}

void dev::I8080::SetDE(uint16_t val)
{
	m_d = (uint8_t)(val >> 8);
	m_e = (uint8_t)(val & 0xFF);
}

uint16_t dev::I8080::GetHL() const
{
	return (uint16_t)((m_h << 8) | m_l);
}

void dev::I8080::SetHL(uint16_t val)
{
	m_h = (uint8_t)(val >> 8);
	m_l = (uint8_t)(val & 0xFF);
}

uint64_t dev::I8080::GetCC() const { return m_cc; }
uint16_t dev::I8080::GetPC() const { return m_pc; }
uint16_t dev::I8080::GetSP() const { return m_sp; }
bool dev::I8080::GetFlagS() const {	return m_flagS; }
bool dev::I8080::GetFlagZ() const { return m_flagZ; }
bool dev::I8080::GetFlagAC() const { return m_flagAC; }
bool dev::I8080::GetFlagP() const { return m_flagP; }
bool dev::I8080::GetFlagC() const {	return m_flagC; }
bool dev::I8080::GetINTE() const { return m_INTE; }
bool dev::I8080::GetIFF() const { return m_IFF; }
bool dev::I8080::GetHLTA() const { return m_HLTA; }

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
bool dev::I8080::GetParity(uint8_t _val)
{
	return parityTable[_val];
}
// determines if there was a carry between bit 'bit_no' and 'bit_no - 1' during the calculation of 'a + b + cy'.
bool dev::I8080::GetCarry(int _bit_no, uint8_t _a, uint8_t _b, bool _cy)
{
	int result = _a + _b + (_cy ? 1 : 0);
	int carry = result ^ _a ^ _b;
	return (carry & (1 << _bit_no)) != 0;
}

void dev::I8080::SetZSP(uint8_t _val)
{
	m_flagZ = _val == 0;
	m_flagS = (_val >> 7) == 1;
	m_flagP = GetParity(_val);
}

// rotate register A left
void dev::I8080::RLC()
{
	m_flagC = m_a >> 7 == 1;
	m_a = (uint8_t)(m_a << 1);
	m_a += (uint8_t)(m_flagC ? 1 : 0);
}

// rotate register A right
void dev::I8080::RRC()
{
	m_flagC = (m_a & 1) == 1;
	m_a = (uint8_t)(m_a >> 1);
	m_a |= (uint8_t)(m_flagC ? 1 << 7 : 0);
}

// rotate register A left with the carry flag
void dev::I8080::RAL()
{
	bool cy = m_flagC;
	m_flagC = m_a >> 7 == 1;
	m_a = (uint8_t)(m_a << 1);
	m_a |= (uint8_t)(cy ? 1 : 0);
}

// rotate register A right with the carry flag
void dev::I8080::RAR()
{
	bool cy = m_flagC;
	m_flagC = (m_a & 1) == 1;
	m_a = (uint8_t)(m_a >> 1);
	m_a |= (uint8_t)(cy ? 1 << 7 : 0);
}

void dev::I8080::MOVRegReg(uint8_t& _regDest, uint8_t _regSrc)
{
	if (m_machineCycle == 0)
	{
		m_TMP = _regSrc;
	}
	else
	{
		_regDest = m_TMP;
	}
}

void dev::I8080::LoadRegPtr(uint8_t& _regDest, Addr _addr)
{
	if (m_machineCycle == 1)
	{
		_regDest = ReadByte(_addr);
	}
}

void dev::I8080::MOVMemReg(uint8_t _sss)
{
	if (m_machineCycle == 0)
	{
		m_TMP = _sss;
	}
	else
	{
		WriteByte(GetHL(), m_TMP);
	}
}

void dev::I8080::MVIRegData(uint8_t& _regDest)
{
	if (m_machineCycle == 1)
	{
		_regDest = ReadByteMovePC();
	}
}

void dev::I8080::MVIMemData()
{
	if (m_machineCycle == 1)
	{
		m_TMP = ReadByteMovePC();
	}
	else if (m_machineCycle == 2)
	{
		WriteByte(GetHL(), m_TMP);
	}
}

void dev::I8080::LDA()
{
	if (m_machineCycle == 1)
	{
		m_Z = ReadByteMovePC();
	}
	else if (m_machineCycle == 2)
	{
		m_W = ReadByteMovePC();
	}
	else if (m_machineCycle == 3)
	{
		m_a = ReadByte(m_W << 8 | m_Z);
	}
}

void dev::I8080::STA()
{
	if (m_machineCycle == 1)
	{
		m_Z = ReadByteMovePC();
	}
	else if (m_machineCycle == 2)
	{
		m_W = ReadByteMovePC();
	}
	else if (m_machineCycle == 3)
	{
		WriteByte(m_W << 8 | m_Z, m_a);
	}
}

void dev::I8080::STAX(Addr _addr)
{
	if (m_machineCycle == 1)
	{
		WriteByte(_addr, m_a);
	}
}

void dev::I8080::LXI(uint8_t& _regH, uint8_t& _regL)
{
	if (m_machineCycle == 1)
	{
		_regL = ReadByteMovePC();
	}
	else if (m_machineCycle == 2)
	{
		_regH = ReadByteMovePC();
	}
}

void dev::I8080::LXISP()
{
	if (m_machineCycle == 1)
	{
		auto _lb = ReadByteMovePC();
		m_sp = m_sp & 0xff00 | _lb;
	}
	else if (m_machineCycle == 2)
	{
		auto _hb = ReadByteMovePC();
		m_sp = _hb << 8 | m_sp & 0xff;
	}
}

void dev::I8080::LHLD()
{
	if (m_machineCycle == 1)
	{
		m_Z = ReadByteMovePC();
	}
	else if (m_machineCycle == 2)
	{
		m_W = ReadByteMovePC();
	}
	else if (m_machineCycle == 3)
	{
		m_l = ReadByte(m_W << 8 | m_Z);
		m_Z++;
		m_W += m_Z == 0 ? 1 : 0;
	}
	else if (m_machineCycle == 4)
	{
		m_h = ReadByte(m_W << 8 | m_Z);
	}
}

void dev::I8080::SHLD()
{
	if (m_machineCycle == 1)
	{
		m_Z = ReadByteMovePC();
	}
	else if (m_machineCycle == 2)
	{
		m_W = ReadByteMovePC();
	}
	else if (m_machineCycle == 3)
	{
		WriteByte(m_W << 8 | m_Z, m_l);
		m_Z++;
		m_W += m_Z == 0 ? 1 : 0;
	}
	else if (m_machineCycle == 4)
	{
		WriteByte(m_W << 8 | m_Z, m_h);
	}
}

void dev::I8080::SPHL()
{
	if (m_machineCycle == 1)
	{
		m_sp = GetHL();
	}
}

void dev::I8080::XCHG()
{
	m_TMP = m_d;
	m_d = m_h;
	m_h = m_TMP;

	m_TMP = m_e;
	m_e = m_l;
	m_l = m_TMP;
}

void dev::I8080::XTHL()
{
	if (m_machineCycle == 1)
	{
		m_Z = ReadByte(m_sp, Memory::AddrSpace::STACK);
	}
	else if (m_machineCycle == 2)
	{
		m_W = ReadByte(m_sp + 1u, Memory::AddrSpace::STACK);
	}
	else if (m_machineCycle == 3)
	{
		WriteByte(m_sp, m_l, Memory::AddrSpace::STACK);
	}
	else if (m_machineCycle == 4)
	{
		WriteByte(m_sp, m_h, Memory::AddrSpace::STACK);
	}
	else if (m_machineCycle == 5)
	{
		m_h = m_W;
		m_l = m_Z;
	}
}

void dev::I8080::PUSH(uint8_t _hb, uint8_t _lb)
{
	if (m_machineCycle == 0)
	{
		m_sp--;
	}
	else if (m_machineCycle == 1)
	{
		WriteByte(m_sp, _hb, Memory::AddrSpace::STACK);
	}
	else if (m_machineCycle == 2)
	{
		m_sp--;
	}
	else if (m_machineCycle == 3)
	{
		WriteByte(m_sp, _lb, Memory::AddrSpace::STACK);
	}
}

void dev::I8080::POP(uint8_t& _regH, uint8_t& _regL)
{
	if (m_machineCycle == 1)
	{
		_regL = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
	}
	else if (m_machineCycle == 2)
	{
		_regH = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
	}
}

// adds a value (+ an optional carry flag) to a register
void dev::I8080::ADD(uint8_t _a, uint8_t _b, bool _cy)
{

	m_a = (uint8_t)(_a + _b + (_cy ? 1 : 0));
	m_flagC = GetCarry(8, _a, _b, _cy);
	m_flagAC = GetCarry(4, _a, _b, _cy);
	SetZSP(m_a);
}

void dev::I8080::ADDMem(bool _cy)
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(GetHL());
		ADD(m_ACT, m_TMP, _cy);
	}
}

void dev::I8080::ADI(bool _cy)
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByteMovePC();
		ADD(m_ACT, m_TMP, _cy);
	}
}

// substracts a uint8_t (+ an optional carry flag) from a register
// see https://stackoverflow.com/a/8037485
void dev::I8080::SUB(uint8_t _a, uint8_t _b, bool _cy)
{
	ADD(_a, (uint8_t)(~_b), !_cy);
	m_flagC = !m_flagC;
}

void dev::I8080::SUBMem(bool _cy)
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(GetHL());
		SUB(m_ACT, m_TMP, _cy);
	}
}

void dev::I8080::SBI(bool _cy)
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByteMovePC();
		SUB(m_ACT, m_TMP, _cy);
	}
}

void dev::I8080::DAD(uint16_t _val)
{
	if (m_machineCycle == 1)
	{
		m_ACT = (uint8_t)(_val & 0xff);
		m_TMP = m_l;
		auto res = m_ACT + m_TMP;
		m_flagC = (res >> 8) == 1;
		m_l = (uint8_t)(res);
	}
	else if (m_machineCycle == 2)
	{
		m_ACT = (uint8_t)(_val >> 8);
		m_TMP = m_h;
		auto result = m_ACT + m_TMP + (m_flagC ? 1 : 0);
		m_flagC = (result >> 8) == 1;
		m_h = (uint8_t)(result);
	}
}

void dev::I8080::INR(uint8_t& _regDest)
{
	if (m_machineCycle == 0)
	{
		m_TMP = _regDest;
		m_TMP++;
		m_flagAC = (m_TMP & 0xF) == 0;
		SetZSP(m_TMP);
	}
	else if (m_machineCycle == 1)
	{
		_regDest = m_TMP;
	}
}

void dev::I8080::INRMem()
{
	if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(GetHL());
		m_TMP++;
		m_flagAC = (m_TMP & 0xF) == 0;
		SetZSP(m_TMP);
	}
	else if (m_machineCycle == 2)
	{
		WriteByte(GetHL(), m_TMP);
	}
}

void dev::I8080::DCR(uint8_t& _regDest)
{
	if (m_machineCycle == 0)
	{
		m_TMP = _regDest;
		m_TMP--;
		m_flagAC = !((m_TMP & 0xF) == 0xF);
		SetZSP(m_TMP);
	}
	else if (m_machineCycle == 1)
	{
		_regDest = m_TMP;
	}
}

void dev::I8080::DCRMem()
{
	if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(GetHL());
		m_TMP--;
		m_flagAC = !((m_TMP & 0xF) == 0xF);
		SetZSP(m_TMP);
	}
	else if (m_machineCycle == 2)
	{
		WriteByte(GetHL(), m_TMP);
	}
}

void dev::I8080::INX(uint8_t& _regH, uint8_t& _regL)
{
	if (m_machineCycle == 0)
	{
		m_Z = (uint8_t)(_regL + 1);
		m_W = (uint8_t)(m_Z == 0 ? _regH + 1 : _regH);
	}
	else if (m_machineCycle == 1)
	{
		_regH = m_W;
		_regL = m_Z;
	}
}

void dev::I8080::INXSP()
{
	if (m_machineCycle == 0)
	{
		m_Z = (uint8_t)(m_sp + 1);
		m_W = (uint8_t)(m_Z == 0 ? (m_sp >> 8) + 1 : m_sp >> 8);
	}
	else if (m_machineCycle == 1)
	{
		m_sp = m_W << 8 | m_Z;
	}
}

void dev::I8080::DCX(uint8_t& _regH, uint8_t& _regL)
{
	if (m_machineCycle == 0)
	{
		m_Z = (uint8_t)(_regL - 1);
		m_W = (uint8_t)(m_Z == 0xff ? _regH - 1 : _regH);
	}
	else if (m_machineCycle == 1)
	{
		_regH = m_W;
		_regL = m_Z;
	}
}

void dev::I8080::DCXSP()
{
	if (m_machineCycle == 0)
	{
		m_Z = (uint8_t)(m_sp - 1);
		m_W = (uint8_t)(m_Z == 0xff ? (m_sp >> 8) - 1 : m_sp >> 8);
	}
	else if (m_machineCycle == 1)
	{
		m_sp = m_W << 8 | m_Z;
	}
}

// Decimal Adjust Accumulator: the eight-bit number in register A is adjusted
// to form two four-bit binary-coded-decimal digits.
// For example, if A=$2B and DAA is executed, A becomes $31.
void dev::I8080::DAA()
{
	bool cy = m_flagC;
	uint8_t correction = 0;

	uint8_t lsb = (uint8_t)(m_a & 0x0F);
	uint8_t msb = (uint8_t)(m_a >> 4);

	if (m_flagAC || lsb > 9)
	{
		correction += 0x06;
	}

	if (m_flagC || msb > 9 || (msb >= 9 && lsb > 9))
	{
		correction += 0x60;
		cy = true;
	}

	ADD(m_a, correction, false);
	m_flagC = cy;
}

void dev::I8080::ANA(uint8_t _sss)
{
	m_ACT = m_a;
	m_TMP = _sss;
	m_a = (uint8_t)(m_ACT & m_TMP);
	m_flagC = false;
	m_flagAC = ((m_ACT | m_TMP) & 0x08) != 0;
	SetZSP(m_a);
}

void dev::I8080::AMAMem()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(GetHL());
		m_a = (uint8_t)(m_ACT & m_TMP);
		m_flagC = false;
		m_flagAC = ((m_ACT | m_TMP) & 0x08) != 0;
		SetZSP(m_a);
	}
}

void dev::I8080::ANI()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByteMovePC();
		m_a = (uint8_t)(m_ACT & m_TMP);
		m_flagC = false;
		m_flagAC = ((m_ACT | m_TMP) & 0x08) != 0;
		SetZSP(m_a);
	}
}

// executes a logic "xor" between register A and a uint8_t, then stores the
// result in register A
void dev::I8080::XRA(uint8_t _sss)
{
	m_ACT = m_a;
	m_TMP = _sss;
	m_a = (uint8_t)(m_ACT ^ m_TMP);
	m_flagC = false;
	m_flagAC = false;
	SetZSP(m_a);
}

void dev::I8080::XRAMem()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(GetHL());
		m_a = (uint8_t)(m_ACT ^ m_TMP);
		m_flagC = false;
		m_flagAC = false;
		SetZSP(m_a);
	}
}

void dev::I8080::XRI()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByteMovePC();
		m_a = (uint8_t)(m_ACT ^ m_TMP);
		m_flagC = false;
		m_flagAC = false;
		SetZSP(m_a);
	}
}

// executes a logic "or" between register A and a uint8_t, then stores the
// result in register A
void dev::I8080::ORA(uint8_t _sss)
{
	m_ACT = m_a;
	m_TMP = _sss;
	m_a = (uint8_t)(m_ACT | m_TMP);
	m_flagC = false;
	m_flagAC = false;
	SetZSP(m_a);
}

void dev::I8080::ORAMem()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(GetHL());
		m_a = (uint8_t)(m_ACT | m_TMP);
		m_flagC = false;
		m_flagAC = false;
		SetZSP(m_a);
	}
}

void dev::I8080::ORI()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByteMovePC();
		m_a = (uint8_t)(m_ACT | m_TMP);
		m_flagC = false;
		m_flagAC = false;
		SetZSP(m_a);
	}
}

// compares the register A to another uint8_t
void dev::I8080::CMP(uint8_t _sss)
{
	m_ACT = m_a;
	m_TMP = _sss;
	uint16_t result = m_ACT - m_TMP;
	m_flagC = result >> 8 == 1;
	m_flagAC = (~(m_ACT ^ result ^ m_TMP) & 0x10) == 0x10;
	SetZSP((uint8_t)(result & 0xFF));
}

void dev::I8080::CMPMem()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(GetHL());
		uint16_t result = (uint16_t)(m_ACT - m_TMP);
		m_flagC = result >> 8 == 1;
		m_flagAC = (~(m_ACT ^ result ^ m_TMP) & 0x10) == 0x10;
		SetZSP((uint8_t)(result & 0xFF));
	}
}

void dev::I8080::CPI()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByteMovePC();
		uint16_t result = (uint16_t)(m_ACT - m_TMP);
		m_flagC = result >> 8 == 1;
		m_flagAC = (~(m_ACT ^ result ^ m_TMP) & 0x10) == 0x10;
		SetZSP((uint8_t)(result & 0xFF));
	}
}

void dev::I8080::JMP(bool _condition)
{
	if (m_machineCycle == 1)
	{
		m_Z = ReadByteMovePC();
	}
	else if (m_machineCycle == 2)
	{
		m_W = ReadByteMovePC();
		if (_condition)
		{
			m_pc = (uint16_t)(m_W << 8 | m_Z);
		}
	}
}

void dev::I8080::PCHL()
{
	if (m_machineCycle == 1)
	{
		m_pc = GetHL();
	}
}

// pushes the current pc to the stack, then jumps to an address
void dev::I8080::CALL(bool _condition)
{
	if (m_machineCycle == 0)
	{
		if (_condition)
		{
			m_sp--;
		}
	}
	else if (m_machineCycle == 1)
	{
		m_Z = ReadByteMovePC();
	}
	else if (m_machineCycle == 2)
	{
		m_W = ReadByteMovePC();
	}
	else if (m_machineCycle == 3)
	{
		WriteByte(m_sp, (uint8_t)(m_pc >> 8), Memory::AddrSpace::STACK);
		if (_condition)
		{
			m_sp--;
		}
		else
		{
			m_machineCycle = 5;
		}
	}
	else if (m_machineCycle == 4)
	{
		WriteByte(m_sp, (uint8_t)(m_pc & 0xff), Memory::AddrSpace::STACK);
	}
	else if (m_machineCycle == 5)
	{
		m_pc = m_W << 8 | m_Z;
	}
}

// pushes the current pc to the stack, then jumps to an address
void dev::I8080::RST(uint8_t _arg)
{
	if (m_machineCycle == 0)
	{
		m_sp--;
	}
	else if (m_machineCycle == 1)
	{
		WriteByte(m_sp, (uint8_t)(m_pc >> 8), Memory::AddrSpace::STACK);
		m_sp--;
	}
	else if (m_machineCycle == 2)
	{
		m_W = 0;
		m_Z = _arg << 3;
		WriteByte(m_sp, (uint8_t)(m_pc & 0xff), Memory::AddrSpace::STACK);
	}
	else if (m_machineCycle == 3)
	{
		m_pc = m_W << 8 | m_Z;
	}
}

// returns from subroutine
void dev::I8080::RET()
{
	if (m_machineCycle == 1)
	{
		m_Z = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
	}
	else if (m_machineCycle == 2)
	{
		m_W = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
		m_pc = m_W << 8 | m_Z;
	}
}

// returns from subroutine if a condition is met
void dev::I8080::RETCond(bool _condition)
{
	if (m_machineCycle == 1)
	{
		if (!_condition) m_machineCycle = 3;
	}
	else if (m_machineCycle == 2)
	{
		m_Z = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
	}
	else if (m_machineCycle == 3)
	{
		m_W = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
		m_pc = m_W << 8 | m_Z;
	}
}

void dev::I8080::IN_()
{
	if (m_machineCycle == 1)
	{
		m_W = 0;
		m_Z = ReadByteMovePC();
		m_a = Input(m_Z);
	}
}

void dev::I8080::OUT_()
{
	if (m_machineCycle == 1)
	{
		m_W = 0;
		m_Z = ReadByteMovePC();
		Output(m_Z, m_a);
	}
}

void dev::I8080::HLT()
{
	if (m_machineCycle == 0)
	{
		m_pc--;
	}
	else if (m_machineCycle == 1)
	{
		ReadInstrMovePC();
		// to loop into the M2 of HLT
		if (!m_IFF) {
			m_HLTA = true;
			m_machineCycle -= 1;
			m_pc--;
		}
	}
}

