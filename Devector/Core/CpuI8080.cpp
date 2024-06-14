#include "CpuI8080.h"
#include "Utils/Utils.h"

dev::CpuI8080::CpuI8080(
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

void dev::CpuI8080::Init()
{
	m_a = m_b = m_c = m_d = m_e = m_h = m_l = 0;
	Reset();
}

void dev::CpuI8080::Reset()
{
	m_cc = m_pc = m_sp = 0;
	m_instructionReg = m_tmp = m_act = m_W = m_Z = 0;
	m_flagS = m_flagZ = m_flagAC = m_flagP = m_flagC = m_inte = false;

	m_machineCycle = 0;
	m_hlta = m_inte = m_iff = m_eiPending = false;
}

void dev::CpuI8080::ExecuteMachineCycle(bool _irq)
{
	m_iff |= _irq & m_inte;

	if (m_machineCycle == 0)
	{
		// interrupt processing
		if (m_iff && !m_eiPending)
		{
			m_inte = false;
			m_iff = false;
			m_hlta = false;
			m_instructionReg = OPCODE_RST7;
		}
		// normal instruction execution
		else
		{
			m_eiPending = false;
			m_instructionReg = ReadInstrMovePC();
		}
	}

	Decode();
	m_cc += MACHINE_CC;
}

bool dev::CpuI8080::IsInstructionExecuted() const
{
	return m_machineCycle == CpuI8080::FIRST_MACHINE_CICLE_IDX || m_hlta;
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
	switch (m_instructionReg)
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
		case 0xF1: POP(m_a, m_tmp); SetFlags(m_tmp); break; // POP PSW

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

		case 0xF3: m_inte = false; break; // DI
		case 0xFB: m_inte = true; m_eiPending = true; break; // EI
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
		dev::Log("Handling undocumented instruction. Opcode: {}", m_instructionReg);
		dev::Exit("Exit", m_instructionReg);
		break;
	}
	
	m_machineCycle++;
	m_machineCycle %= M_CYCLES[m_instructionReg];
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
	auto globalAddr = m_memory.GetGlobalAddr(m_pc, Memory::AddrSpace::RAM);

	uint8_t op_code = m_memory.GetByte(m_pc, Memory::AddrSpace::RAM);
	uint8_t _dataL = m_memory.GetByte(m_pc + 1, Memory::AddrSpace::RAM);
	uint8_t _dataH = m_memory.GetByte(m_pc + 2, Memory::AddrSpace::RAM);

	auto DebugOnReadInstr = m_debugOnReadInstr.load();
	if (DebugOnReadInstr && *DebugOnReadInstr) (*DebugOnReadInstr)(globalAddr, op_code, _dataH, _dataL, GetHL());
	m_pc++;
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
	auto result = ReadByte(m_pc, _addrSpace);
	m_pc++;
	return result;
}


////////////////////////////////////////////////////////////////////////////
//
// Register helpers
//
////////////////////////////////////////////////////////////////////////////

uint8_t dev::CpuI8080::GetFlags() const
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

uint16_t dev::CpuI8080::GetAF() const
{
	return (uint16_t)(m_a << 8 | GetFlags());
}

void dev::CpuI8080::SetFlags(uint8_t psw)
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

uint16_t dev::CpuI8080::GetBC() const
{ 
	return (m_b << 8) | m_c; 
}

void dev::CpuI8080::SetBC(uint16_t val)
{
	m_b = (uint8_t)(val >> 8);
	m_c = (uint8_t)(val & 0xFF);
}

uint16_t dev::CpuI8080::GetDE() const
{
	return (uint16_t)((m_d << 8) | m_e);
}

void dev::CpuI8080::SetDE(uint16_t val)
{
	m_d = (uint8_t)(val >> 8);
	m_e = (uint8_t)(val & 0xFF);
}

uint16_t dev::CpuI8080::GetHL() const
{
	return (uint16_t)((m_h << 8) | m_l);
}

void dev::CpuI8080::SetHL(uint16_t val)
{
	m_h = (uint8_t)(val >> 8);
	m_l = (uint8_t)(val & 0xFF);
}

uint64_t dev::CpuI8080::GetCC() const { return m_cc; }
uint16_t dev::CpuI8080::GetPC() const { return m_pc; }
uint16_t dev::CpuI8080::GetSP() const { return m_sp; }
bool dev::CpuI8080::GetFlagS() const {	return m_flagS; }
bool dev::CpuI8080::GetFlagZ() const { return m_flagZ; }
bool dev::CpuI8080::GetFlagAC() const { return m_flagAC; }
bool dev::CpuI8080::GetFlagP() const { return m_flagP; }
bool dev::CpuI8080::GetFlagC() const {	return m_flagC; }
bool dev::CpuI8080::GetINTE() const { return m_inte; }
bool dev::CpuI8080::GetIFF() const { return m_iff; }
bool dev::CpuI8080::GetHLTA() const { return m_hlta; }
auto dev::CpuI8080::GetMachineCycle() const -> int { return m_machineCycle; }

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
	m_flagZ = _val == 0;
	m_flagS = (_val >> 7) == 1;
	m_flagP = GetParity(_val);
}

// rotate register A left
void dev::CpuI8080::RLC()
{
	m_flagC = m_a >> 7 == 1;
	m_a = (uint8_t)(m_a << 1);
	m_a += (uint8_t)(m_flagC ? 1 : 0);
}

// rotate register A right
void dev::CpuI8080::RRC()
{
	m_flagC = (m_a & 1) == 1;
	m_a = (uint8_t)(m_a >> 1);
	m_a |= (uint8_t)(m_flagC ? 1 << 7 : 0);
}

// rotate register A left with the carry flag
void dev::CpuI8080::RAL()
{
	bool cy = m_flagC;
	m_flagC = m_a >> 7 == 1;
	m_a = (uint8_t)(m_a << 1);
	m_a |= (uint8_t)(cy ? 1 : 0);
}

// rotate register A right with the carry flag
void dev::CpuI8080::RAR()
{
	bool cy = m_flagC;
	m_flagC = (m_a & 1) == 1;
	m_a = (uint8_t)(m_a >> 1);
	m_a |= (uint8_t)(cy ? 1 << 7 : 0);
}

void dev::CpuI8080::MOVRegReg(uint8_t& _regDest, uint8_t _regSrc)
{
	switch (m_machineCycle) {
	case 0:
		m_tmp = _regSrc;
		return;
	case 1:
		_regDest = m_tmp;
		return;
	}
}

void dev::CpuI8080::LoadRegPtr(uint8_t& _regDest, Addr _addr)
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		_regDest = ReadByte(_addr);
		return;
	}
}

void dev::CpuI8080::MOVMemReg(uint8_t _sss)
{
	switch (m_machineCycle) {
	case 0:
		m_tmp = _sss;
		return;
	case 1:
		WriteByte(GetHL(), m_tmp);
		return;
	}
}

void dev::CpuI8080::MVIRegData(uint8_t& _regDest)
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		_regDest = ReadByteMovePC();
		return;
	}
}

void dev::CpuI8080::MVIMemData()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_tmp = ReadByteMovePC();
		return;
	case 2:
		WriteByte(GetHL(), m_tmp);
		return;
	}
}

void dev::CpuI8080::LDA()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_Z = ReadByteMovePC();
		return;
	case 2:
		m_W = ReadByteMovePC();
		return;
	case 3:
		m_a = ReadByte(m_W << 8 | m_Z);
		return;
	}
}

void dev::CpuI8080::STA()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_Z = ReadByteMovePC();
		return;
	case 2:
		m_W = ReadByteMovePC();
		return;
	case 3:
		WriteByte(m_W << 8 | m_Z, m_a);
		return;
	}
}

void dev::CpuI8080::STAX(Addr _addr)
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		WriteByte(_addr, m_a);
		return;
	}
}

void dev::CpuI8080::LXI(uint8_t& _regH, uint8_t& _regL)
{
	switch (m_machineCycle) {
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
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:{
			auto _lb = ReadByteMovePC();
			m_sp = m_sp & 0xff00 | _lb;
			return;
		}
	case 2: {
			auto _hb = ReadByteMovePC();
			m_sp = _hb << 8 | m_sp & 0xff;
			return;
		}
	}
}

void dev::CpuI8080::LHLD()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_Z = ReadByteMovePC();
		return;
	case 2:
		m_W = ReadByteMovePC();
		return;
	case 3:
		m_l = ReadByte(m_W << 8 | m_Z);
		m_Z++;
		m_W += m_Z == 0 ? 1 : 0;
		return;
	case 4:
		m_h = ReadByte(m_W << 8 | m_Z);
		return;
	}
}

void dev::CpuI8080::SHLD()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_Z = ReadByteMovePC();
		return;
	case 2:
		m_W = ReadByteMovePC();
		return;
	case 3:
		WriteByte(m_W << 8 | m_Z, m_l);
		m_Z++;
		m_W += m_Z == 0 ? 1 : 0;
		return;
	case 4:
		WriteByte(m_W << 8 | m_Z, m_h);
		return;
	}
}

void dev::CpuI8080::SPHL()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_sp = GetHL();
		return;
	}
}

void dev::CpuI8080::XCHG()
{
	m_tmp = m_d;
	m_d = m_h;
	m_h = m_tmp;

	m_tmp = m_e;
	m_e = m_l;
	m_l = m_tmp;
}

void dev::CpuI8080::XTHL()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_Z = ReadByte(m_sp, Memory::AddrSpace::STACK);
		return;
	case 2:
		m_W = ReadByte(m_sp + 1u, Memory::AddrSpace::STACK);
		return;
	case 3:
		WriteByte(m_sp, m_l, Memory::AddrSpace::STACK);
		return;
	case 4:
		WriteByte(m_sp + 1u, m_h, Memory::AddrSpace::STACK);
		return;
	case 5:
		m_h = m_W;
		m_l = m_Z;
		return;
	}
}

void dev::CpuI8080::PUSH(uint8_t _hb, uint8_t _lb)
{
	switch (m_machineCycle) {
	case 0:
		m_sp--;
		return;
	case 1:
		WriteByte(m_sp, _hb, Memory::AddrSpace::STACK);
		return;
	case 2:
		m_sp--;
		return;
	case 3:
		WriteByte(m_sp, _lb, Memory::AddrSpace::STACK);
		return;
	}
}

void dev::CpuI8080::POP(uint8_t& _regH, uint8_t& _regL)
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		_regL = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
		return;
	case 2:
		_regH = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
		return;
	}
}

// adds a value (+ an optional carry flag) to a register
void dev::CpuI8080::ADD(uint8_t _a, uint8_t _b, bool _cy)
{

	m_a = (uint8_t)(_a + _b + (_cy ? 1 : 0));
	m_flagC = GetCarry(8, _a, _b, _cy);
	m_flagAC = GetCarry(4, _a, _b, _cy);
	SetZSP(m_a);
}

void dev::CpuI8080::ADDMem(bool _cy)
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1:
		m_tmp = ReadByte(GetHL());
		ADD(m_act, m_tmp, _cy);
		return;
	}
}

void dev::CpuI8080::ADI(bool _cy)
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1:
		m_tmp = ReadByteMovePC();
		ADD(m_act, m_tmp, _cy);
		return;
	}
}

// substracts a uint8_t (+ an optional carry flag) from a register
// see https://stackoverflow.com/a/8037485
void dev::CpuI8080::SUB(uint8_t _a, uint8_t _b, bool _cy)
{
	ADD(_a, (uint8_t)(~_b), !_cy);
	m_flagC = !m_flagC;
}

void dev::CpuI8080::SUBMem(bool _cy)
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1:
		m_tmp = ReadByte(GetHL());
		SUB(m_act, m_tmp, _cy);
		return;
	}
}

void dev::CpuI8080::SBI(bool _cy)
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1:
		m_tmp = ReadByteMovePC();
		SUB(m_act, m_tmp, _cy);
		return;
	}
}

void dev::CpuI8080::DAD(uint16_t _val)
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1: {
			m_act = (uint8_t)(_val & 0xff);
			m_tmp = m_l;
			int res = m_act + m_tmp;
			m_flagC = (res >> 8) == 1;
			m_l = (uint8_t)(res);
			return;
		}
	case 2: {
			m_act = (uint8_t)(_val >> 8);
			m_tmp = m_h;
			int result = m_act + m_tmp + (m_flagC ? 1 : 0);
			m_flagC = (result >> 8) == 1;
			m_h = (uint8_t)(result);
			return;
		}
	}
}

void dev::CpuI8080::INR(uint8_t& _regDest)
{
	switch (m_machineCycle) {
	case 0:
		m_tmp = _regDest;
		m_tmp++;
		m_flagAC = (m_tmp & 0xF) == 0;
		SetZSP(m_tmp);
		return;
	case 1:
		_regDest = m_tmp;
		return;
	}
}

void dev::CpuI8080::INRMem()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_tmp = ReadByte(GetHL());
		m_tmp++;
		m_flagAC = (m_tmp & 0xF) == 0;
		SetZSP(m_tmp);
		return;
	case 2:
		WriteByte(GetHL(), m_tmp);
		return;
	}
}

void dev::CpuI8080::DCR(uint8_t& _regDest)
{
	switch (m_machineCycle) {
	case 0:
		m_tmp = _regDest;
		m_tmp--;
		m_flagAC = !((m_tmp & 0xF) == 0xF);
		SetZSP(m_tmp);
		return;
	case 1:
		_regDest = m_tmp;
		return;
	}
}

void dev::CpuI8080::DCRMem()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_tmp = ReadByte(GetHL());
		m_tmp--;
		m_flagAC = !((m_tmp & 0xF) == 0xF);
		SetZSP(m_tmp);
		return;
	case 2:
		WriteByte(GetHL(), m_tmp);
		return;
	}
}

void dev::CpuI8080::INX(uint8_t& _regH, uint8_t& _regL)
{
	switch (m_machineCycle) {
	case 0:
		m_Z = (uint8_t)(_regL + 1);
		m_W = (uint8_t)(m_Z == 0 ? _regH + 1 : _regH);
		return;
	case 1:
		_regH = m_W;
		_regL = m_Z;
		return;
	}
}

void dev::CpuI8080::INXSP()
{
	switch (m_machineCycle) {
	case 0:
		m_Z = (uint8_t)(m_sp + 1);
		m_W = (uint8_t)(m_Z == 0 ? (m_sp >> 8) + 1 : m_sp >> 8);
		return;
	case 1:
		m_sp = m_W << 8 | m_Z;
		return;
	}
}

void dev::CpuI8080::DCX(uint8_t& _regH, uint8_t& _regL)
{
	switch (m_machineCycle) {
	case 0:
		m_Z = (uint8_t)(_regL - 1);
		m_W = (uint8_t)(m_Z == 0xff ? _regH - 1 : _regH);
		return;
	case 1:
		_regH = m_W;
		_regL = m_Z;
		return;
	}
}

void dev::CpuI8080::DCXSP()
{
	switch (m_machineCycle) {
	case 0:
		m_Z = (uint8_t)(m_sp - 1);
		m_W = (uint8_t)(m_Z == 0xff ? (m_sp >> 8) - 1 : m_sp >> 8);
		return;
	case 1:
		m_sp = m_W << 8 | m_Z;
		return;
	}
}

// Decimal Adjust Accumulator: the eight-bit number in register A is adjusted
// to form two four-bit binary-coded-decimal digits.
// For example, if A=$2B and DAA is executed, A becomes $31.
void dev::CpuI8080::DAA()
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

void dev::CpuI8080::ANA(uint8_t _sss)
{
	m_act = m_a;
	m_tmp = _sss;
	m_a = (uint8_t)(m_act & m_tmp);
	m_flagC = false;
	m_flagAC = ((m_act | m_tmp) & 0x08) != 0;
	SetZSP(m_a);
}

void dev::CpuI8080::AMAMem()
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1:
		m_tmp = ReadByte(GetHL());
		m_a = (uint8_t)(m_act & m_tmp);
		m_flagC = false;
		m_flagAC = ((m_act | m_tmp) & 0x08) != 0;
		SetZSP(m_a);
		return;
	}
}

void dev::CpuI8080::ANI()
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1:
		m_tmp = ReadByteMovePC();
		m_a = (uint8_t)(m_act & m_tmp);
		m_flagC = false;
		m_flagAC = ((m_act | m_tmp) & 0x08) != 0;
		SetZSP(m_a);
		return;
	}
}

// executes a logic "xor" between register A and a uint8_t, then stores the
// result in register A
void dev::CpuI8080::XRA(uint8_t _sss)
{
	m_act = m_a;
	m_tmp = _sss;
	m_a = (uint8_t)(m_act ^ m_tmp);
	m_flagC = false;
	m_flagAC = false;
	SetZSP(m_a);
}

void dev::CpuI8080::XRAMem()
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1:
		m_tmp = ReadByte(GetHL());
		m_a = (uint8_t)(m_act ^ m_tmp);
		m_flagC = false;
		m_flagAC = false;
		SetZSP(m_a);
		return;
	}
}

void dev::CpuI8080::XRI()
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1:
		m_tmp = ReadByteMovePC();
		m_a = (uint8_t)(m_act ^ m_tmp);
		m_flagC = false;
		m_flagAC = false;
		SetZSP(m_a);
		return;
	}
}

// executes a logic "or" between register A and a uint8_t, then stores the
// result in register A
void dev::CpuI8080::ORA(uint8_t _sss)
{
	m_act = m_a;
	m_tmp = _sss;
	m_a = (uint8_t)(m_act | m_tmp);
	m_flagC = false;
	m_flagAC = false;
	SetZSP(m_a);
}

void dev::CpuI8080::ORAMem()
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1:
		m_tmp = ReadByte(GetHL());
		m_a = (uint8_t)(m_act | m_tmp);
		m_flagC = false;
		m_flagAC = false;
		SetZSP(m_a);
		return;
	}
}

void dev::CpuI8080::ORI()
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1:
		m_tmp = ReadByteMovePC();
		m_a = (uint8_t)(m_act | m_tmp);
		m_flagC = false;
		m_flagAC = false;
		SetZSP(m_a);
		return;
	}
}

// compares the register A to another uint8_t
void dev::CpuI8080::CMP(uint8_t _sss)
{
	m_act = m_a;
	m_tmp = _sss;
	auto res = m_act - m_tmp;
	m_flagC = (res >> 8) & 1;
	m_flagAC = (~(m_act ^ (res & 0xFF) ^ m_tmp) & 0x10) == 0x10;
	SetZSP((uint8_t)(res & 0xFF));
}

void dev::CpuI8080::CMPMem()
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1: {
			m_tmp = ReadByte(GetHL());
			int res = m_act - m_tmp;
			m_flagC = (res >> 8) & 1;
			m_flagAC = (~(m_act ^ (res & 0xFF) ^ m_tmp) & 0x10) == 0x10;
			SetZSP((uint8_t)(res & 0xFF));
			return;
		}
	}
}

void dev::CpuI8080::CPI()
{
	switch (m_machineCycle) {
	case 0:
		m_act = m_a;
		return;
	case 1:
		m_tmp = ReadByteMovePC();
		auto res = m_act - m_tmp;
		m_flagC = (res >> 8) & 1;
		m_flagAC = (~(m_act ^ (res & 0xFF) ^ m_tmp) & 0x10) == 0x10;
		SetZSP((uint8_t)(res & 0xFF));
		return;
	}
}

void dev::CpuI8080::JMP(bool _condition)
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_Z = ReadByteMovePC();
		return;
	case 2:
		m_W = ReadByteMovePC();
		if (_condition)
		{
			m_pc = (uint16_t)(m_W << 8 | m_Z);
		}
		return;
	}
}

void dev::CpuI8080::PCHL()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_pc = GetHL();
		return;
	}
}

// pushes the current pc to the stack, then jumps to an address
void dev::CpuI8080::CALL(bool _condition)
{
	switch (m_machineCycle) {
	case 0:
		m_sp -= _condition ? 1 : 0;
		return;
	case 1:
		m_Z = ReadByteMovePC();
		return;
	case 2:
		m_W = ReadByteMovePC();
		return;
	case 3:
		if (_condition)	{
			WriteByte(m_sp, (uint8_t)(m_pc >> 8), Memory::AddrSpace::STACK);
			m_sp--;
		} else {
			// end execution
			m_machineCycle = 5;
		}
		return;
	case 4:
		WriteByte(m_sp, (uint8_t)(m_pc & 0xff), Memory::AddrSpace::STACK);
		return;
	case 5:
		m_pc = m_W << 8 | m_Z;
		return;
	}
}

// pushes the current pc to the stack, then jumps to an address
void dev::CpuI8080::RST(uint8_t _arg)
{
	switch (m_machineCycle) {
	case 0:
		m_sp--;
		return;
	case 1:
		WriteByte(m_sp, (uint8_t)(m_pc >> 8), Memory::AddrSpace::STACK);
		m_sp--;
		return;
	case 2:
		m_W = 0;
		m_Z = _arg << 3;
		WriteByte(m_sp, (uint8_t)(m_pc & 0xff), Memory::AddrSpace::STACK);
		return;
	case 3:
		m_pc = m_W << 8 | m_Z;
		return;
	}
}

// returns from subroutine
void dev::CpuI8080::RET()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_Z = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
		return;
	case 2:
		m_W = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
		m_pc = m_W << 8 | m_Z;
		return;
	}
}

// returns from subroutine if a condition is met
void dev::CpuI8080::RETCond(bool _condition)
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		if (!_condition) m_machineCycle = 3;
		return;
	case 2:
		m_Z = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
		return;
	case 3:
		m_W = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
		m_pc = m_W << 8 | m_Z;
		return;
	}
}

void dev::CpuI8080::IN_()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_W = 0;
		m_Z = ReadByteMovePC();
		return;
	case 2:
		m_a = Input(m_Z);
		return;
	}
}

void dev::CpuI8080::OUT_()
{
	switch (m_machineCycle) {
	case 0:
		return;
	case 1:
		m_W = 0;
		m_Z = ReadByteMovePC();
		return;
	case 2:
		Output(m_Z, m_a);
		return;
	}
}

void dev::CpuI8080::HLT()
{
	switch (m_machineCycle) {
	case 0:
		m_pc--;
		return;
	case 1:
		ReadInstrMovePC();
		// to loop into the M2 of HLT
		if (!m_iff) {
			m_hlta = true;
			m_machineCycle--;
			m_pc--;
		}
		return;
	}
}