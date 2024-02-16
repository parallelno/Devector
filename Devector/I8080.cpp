#include "I8080.h"

dev::I8080::I8080(
	MemoryReadFunc _memoryRead, 
	MemoryWriteFunc _memoryWrite, 
	InputFunc _input, 
	OutputFunc _output, 
	DebugMemStatsFunc _debugMemStats)
	:
	MemoryRead(_memoryRead),
	MemoryWrite(_memoryWrite),
	Input(_input),
	Output(_output),
	DebugMemStats(_debugMemStats)
{
	m_flagUnused1 = true;
	m_flagUnused3 = false;
	m_flagUnused5 = false;

	Init();
}

void dev::I8080::Init()
{
	m_cc = m_pc = m_sp = 0;
	m_a = m_b = m_c = m_d = m_e = m_h = m_l = m_instructionRegister = m_TMP = m_ACT = m_W = m_Z = 0;
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
			if (m_instructionRegister == OPCODE_HLT)
			{
				m_pc--; // move the program counter back if the last instruction was HLT
			}

			m_eiPending = false;
			m_instructionRegister = ReadInstrMovePC();
		}
	}

	Decode();
	m_cc += MACHINE_CC;
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
	2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 7

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
		case 0x7F: mov_r_r(m_a, m_a); break; // MOV A,A
		case 0x78: mov_r_r(m_a, m_b); break; // MOV A,B
		case 0x79: mov_r_r(m_a, m_c); break; // MOV A,C
		case 0x7A: mov_r_r(m_a, m_d); break; // MOV A,D
		case 0x7B: mov_r_r(m_a, m_e); break; // MOV A,E
		case 0x7C: mov_r_r(m_a, m_h); break; // MOV A,H
		case 0x7D: mov_r_r(m_a, m_l); break; // MOV A,L

		case 0x47: mov_r_r(m_b, m_a); break; // MOV B,A
		case 0x40: mov_r_r(m_b, m_b); break; // MOV B,B
		case 0x41: mov_r_r(m_b, m_c); break; // MOV B,C
		case 0x42: mov_r_r(m_b, m_d); break; // MOV B,D
		case 0x43: mov_r_r(m_b, m_e); break; // MOV B,E
		case 0x44: mov_r_r(m_b, m_h); break; // MOV B,H
		case 0x45: mov_r_r(m_b, m_l); break; // MOV B,L

		case 0x4F: mov_r_r(m_c, m_a); break; // MOV C,A
		case 0x48: mov_r_r(m_c, m_b); break; // MOV C,B
		case 0x49: mov_r_r(m_c, m_c); break; // MOV C,C
		case 0x4A: mov_r_r(m_c, m_d); break; // MOV C,D
		case 0x4B: mov_r_r(m_c, m_e); break; // MOV C,E
		case 0x4C: mov_r_r(m_c, m_h); break; // MOV C,H
		case 0x4D: mov_r_r(m_c, m_l); break; // MOV C,L

		case 0x57: mov_r_r(m_d, m_a); break; // MOV D,A
		case 0x50: mov_r_r(m_d, m_b); break; // MOV D,B
		case 0x51: mov_r_r(m_d, m_c); break; // MOV D,C
		case 0x52: mov_r_r(m_d, m_d); break; // MOV D,D
		case 0x53: mov_r_r(m_d, m_e); break; // MOV D,E
		case 0x54: mov_r_r(m_d, m_h); break; // MOV D,H
		case 0x55: mov_r_r(m_d, m_l); break; // MOV D,L

		case 0x5F: mov_r_r(m_e, m_a); break; // MOV E,A
		case 0x58: mov_r_r(m_e, m_b); break; // MOV E,B
		case 0x59: mov_r_r(m_e, m_c); break; // MOV E,C
		case 0x5A: mov_r_r(m_e, m_d); break; // MOV E,D
		case 0x5B: mov_r_r(m_e, m_e); break; // MOV E,E
		case 0x5C: mov_r_r(m_e, m_h); break; // MOV E,H
		case 0x5D: mov_r_r(m_e, m_l); break; // MOV E,L

		case 0x67: mov_r_r(m_h, m_a); break; // MOV H,A
		case 0x60: mov_r_r(m_h, m_b); break; // MOV H,B
		case 0x61: mov_r_r(m_h, m_c); break; // MOV H,C
		case 0x62: mov_r_r(m_h, m_d); break; // MOV H,D
		case 0x63: mov_r_r(m_h, m_e); break; // MOV H,E
		case 0x64: mov_r_r(m_h, m_h); break; // MOV H,H
		case 0x65: mov_r_r(m_h, m_l); break; // MOV H,L

		case 0x6F: mov_r_r(m_l, m_a); break; // MOV L,A
		case 0x68: mov_r_r(m_l, m_b); break; // MOV L,B
		case 0x69: mov_r_r(m_l, m_c); break; // MOV L,C
		case 0x6A: mov_r_r(m_l, m_d); break; // MOV L,D
		case 0x6B: mov_r_r(m_l, m_e); break; // MOV L,E
		case 0x6C: mov_r_r(m_l, m_h); break; // MOV L,H
		case 0x6D: mov_r_r(m_l, m_l); break; // MOV L,L

		case 0x7E: load_r_p(m_a, i8080_get_hl()); break; // MOV A,M
		case 0x46: load_r_p(m_b, i8080_get_hl()); break; // MOV B,M
		case 0x4E: load_r_p(m_c, i8080_get_hl()); break; // MOV C,M
		case 0x56: load_r_p(m_d, i8080_get_hl()); break; // MOV D,M
		case 0x5E: load_r_p(m_e, i8080_get_hl()); break; // MOV E,M
		case 0x66: load_r_p(m_h, i8080_get_hl()); break; // MOV H,M
		case 0x6E: load_r_p(m_l, i8080_get_hl()); break; // MOV L,M

		case 0x77: mov_m_r(m_a); break; // MOV M,A
		case 0x70: mov_m_r(m_b); break; // MOV M,B
		case 0x71: mov_m_r(m_c); break; // MOV M,C
		case 0x72: mov_m_r(m_d); break; // MOV M,D
		case 0x73: mov_m_r(m_e); break; // MOV M,E
		case 0x74: mov_m_r(m_h); break; // MOV M,H
		case 0x75: mov_m_r(m_l); break; // MOV M,L

		case 0x3E: mvi_r_d(m_a); break; // MVI A,uint8_t
		case 0x06: mvi_r_d(m_b); break; // MVI B,uint8_t
		case 0x0E: mvi_r_d(m_c); break; // MVI C,uint8_t
		case 0x16: mvi_r_d(m_d); break; // MVI D,uint8_t
		case 0x1E: mvi_r_d(m_e); break; // MVI E,uint8_t
		case 0x26: mvi_r_d(m_h); break; // MVI H,uint8_t
		case 0x2E: mvi_r_d(m_l); break; // MVI L,uint8_t
		case 0x36: mvi_m_d(); break; // MVI M,uint8_t

		case 0x0A: load_r_p(m_a, i8080_get_bc()); break; // LDAX B
		case 0x1A: load_r_p(m_a, i8080_get_de()); break; // LDAX D
		case 0x3A: lda(); break; // LDA word

		case 0x02: stax(i8080_get_bc()); break; // STAX B
		case 0x12: stax(i8080_get_de()); break; // STAX D
		case 0x32: sta(); break; // STA word

		case 0x01: lxi(m_b, m_c); break; // LXI B,word
		case 0x11: lxi(m_d, m_e); break; // LXI D,word
		case 0x21: lxi(m_h, m_l); break; // LXI H,word
		case 0x31: lxi_sp(); break; // LXI SP,word
		case 0x2A: lhld(); break; // LHLD
		case 0x22: shld(); break; // SHLD
		case 0xF9: sphl(); break; // SPHL

		case 0xEB: xchg(); break; // XCHG
		case 0xE3: xthl(); break; // XTHL

		case 0xC5: push(m_b, m_c); break; // PUSH B
		case 0xD5: push(m_d, m_e); break; // PUSH D
		case 0xE5: push(m_h, m_l); break; // PUSH H
		case 0xF5: push(m_a, i8080_get_flags()); break; // PUSH PSW
		case 0xC1: pop(m_b, m_c); break; // POP B
		case 0xD1: pop(m_d, m_e); break; // POP D
		case 0xE1: pop(m_h, m_l); break; // POP H
		case 0xF1: pop(m_a, m_TMP); i8080_set_flags(m_TMP); break; // POP PSW

		case 0x87: add(m_a, m_a, false); break; // ADD A
		case 0x80: add(m_a, m_b, false); break; // ADD B
		case 0x81: add(m_a, m_c, false); break; // ADD C
		case 0x82: add(m_a, m_d, false); break; // ADD D
		case 0x83: add(m_a, m_e, false); break; // ADD E
		case 0x84: add(m_a, m_h, false); break; // ADD H
		case 0x85: add(m_a, m_l, false); break; // ADD L
		case 0x86: add_m(false); break; // ADD M
		case 0xC6: adi(false); break; // ADI uint8_t

		case 0x8F: add(m_a, m_a, m_flagC); break; // ADC A
		case 0x88: add(m_a, m_b, m_flagC); break; // ADC B
		case 0x89: add(m_a, m_c, m_flagC); break; // ADC C
		case 0x8A: add(m_a, m_d, m_flagC); break; // ADC D
		case 0x8B: add(m_a, m_e, m_flagC); break; // ADC E
		case 0x8C: add(m_a, m_h, m_flagC); break; // ADC H
		case 0x8D: add(m_a, m_l, m_flagC); break; // ADC L
		case 0x8E: add_m(m_flagC); break; // ADC M
		case 0xCE: adi(m_flagC); break; // ACI uint8_t

		case 0x97: sub(m_a, m_a, false); break; // SUB A
		case 0x90: sub(m_a, m_b, false); break; // SUB B
		case 0x91: sub(m_a, m_c, false); break; // SUB C
		case 0x92: sub(m_a, m_d, false); break; // SUB D
		case 0x93: sub(m_a, m_e, false); break; // SUB E
		case 0x94: sub(m_a, m_h, false); break; // SUB H
		case 0x95: sub(m_a, m_l, false); break; // SUB L
		case 0x96: sub_m(false); break; // SUB M
		case 0xD6: sbi(false); break; // SUI uint8_t

		case 0x9F: sub(m_a, m_a, m_flagC); break; // SBB A
		case 0x98: sub(m_a, m_b, m_flagC); break; // SBB B
		case 0x99: sub(m_a, m_c, m_flagC); break; // SBB C
		case 0x9A: sub(m_a, m_d, m_flagC); break; // SBB D
		case 0x9B: sub(m_a, m_e, m_flagC); break; // SBB E
		case 0x9C: sub(m_a, m_h, m_flagC); break; // SBB H
		case 0x9D: sub(m_a, m_l, m_flagC); break; // SBB L
		case 0x9E: sub_m(m_flagC); break; // SBB M
		case 0xDE: sbi(m_flagC); break; // SBI uint8_t

		case 0x09: dad(i8080_get_bc()); break; // DAD B
		case 0x19: dad(i8080_get_de()); break; // DAD D
		case 0x29: dad(i8080_get_hl()); break; // DAD H
		case 0x39: dad(m_sp); break; // DAD SP

		case 0x3C: inr(m_a); break; // INR A
		case 0x04: inr(m_b); break; // INR B
		case 0x0C: inr(m_c); break; // INR C
		case 0x14: inr(m_d); break; // INR D
		case 0x1C: inr(m_e); break; // INR E
		case 0x24: inr(m_h); break; // INR H
		case 0x2C: inr(m_l); break; // INR L
		case 0x34: inr_m(); break; // INR M

		case 0x3D: dcr(m_a); break; // DCR A
		case 0x05: dcr(m_b); break; // DCR B
		case 0x0D: dcr(m_c); break; // DCR C
		case 0x15: dcr(m_d); break; // DCR D
		case 0x1D: dcr(m_e); break; // DCR E
		case 0x25: dcr(m_h); break; // DCR H
		case 0x2D: dcr(m_l); break; // DCR L
		case 0x35: dcr_m(); break; // DCR M

		case 0x03: inx(m_b, m_c); break; // INX B
		case 0x13: inx(m_d, m_e); break; // INX D
		case 0x23: inx(m_h, m_l); break; // INX H
		case 0x33: inx_sp(); break; // INX SP

		case 0x0B: dcx(m_b, m_c); break; // DCX B
		case 0x1B: dcx(m_d, m_e); break; // DCX D
		case 0x2B: dcx(m_h, m_l); break; // DCX H
		case 0x3B: dcx_sp(); break; // DCX SP

		case 0x27: daa(); break; // DAA
		case 0x2F: m_a = ~m_a; break; // CMA
		case 0x37: m_flagC = true; break; // STC
		case 0x3F: m_flagC = !m_flagC; break; // CMC

		case 0x07: RLC(); break; // RLC (rotate left)
		case 0x0F: rrc(); break; // RRC (rotate right)
		case 0x17: ral(); break; // RAL
		case 0x1F: rar(); break; // RAR

		case 0xA7: ana(m_a); break; // ANA A
		case 0xA0: ana(m_b); break; // ANA B
		case 0xA1: ana(m_c); break; // ANA C
		case 0xA2: ana(m_d); break; // ANA D
		case 0xA3: ana(m_e); break; // ANA E
		case 0xA4: ana(m_h); break; // ANA H
		case 0xA5: ana(m_l); break; // ANA L
		case 0xA6: ana_m(); break; // ANA M
		case 0xE6: ani(); break; // ANI uint8_t

		case 0xAF: xra(m_a); break; // XRA A
		case 0xA8: xra(m_b); break; // XRA B
		case 0xA9: xra(m_c); break; // XRA C
		case 0xAA: xra(m_d); break; // XRA D
		case 0xAB: xra(m_e); break; // XRA E
		case 0xAC: xra(m_h); break; // XRA H
		case 0xAD: xra(m_l); break; // XRA L
		case 0xAE: xra_m(); break; // XRA M
		case 0xEE: xri(); break; // XRI uint8_t

		case 0xB7: ora(m_a); break; // ORA A
		case 0xB0: ora(m_b); break; // ORA B
		case 0xB1: ora(m_c); break; // ORA C
		case 0xB2: ora(m_d); break; // ORA D
		case 0xB3: ora(m_e); break; // ORA E
		case 0xB4: ora(m_h); break; // ORA H
		case 0xB5: ora(m_l); break; // ORA L
		case 0xB6: ora_m(); break; // ORA M
		case 0xF6: ori(); break; // ORI uint8_t

		case 0xBF: cmp(m_a); break; // CMP A
		case 0xB8: cmp(m_b); break; // CMP B
		case 0xB9: cmp(m_c); break; // CMP C
		case 0xBA: cmp(m_d); break; // CMP D
		case 0xBB: cmp(m_e); break; // CMP E
		case 0xBC: cmp(m_h); break; // CMP H
		case 0xBD: cmp(m_l); break; // CMP L
		case 0xBE: cmp_m(); break; // CMP M
		case 0xFE: cpi(); break; // CPI uint8_t

		case 0xC3: jmp(); break; // JMP
		case 0xCB: jmp(); break; // undocumented JMP
		case 0xC2: jmp(m_flagZ == false); break; // JNZ
		case 0xCA: jmp(m_flagZ == true); break; // JZ
		case 0xD2: jmp(m_flagC == false); break; // JNC
		case 0xDA: jmp(m_flagC == true); break; // JC
		case 0xE2: jmp(m_flagP == false); break; // JPO
		case 0xEA: jmp(m_flagP == true); break; // JPE
		case 0xF2: jmp(m_flagS == false); break; // JP
		case 0xFA: jmp(m_flagS == true); break; // JM

		case 0xE9: pchl(); break; // PCHL
		case 0xCD: call(); break; // CALL
		case 0xDD: call(); break; // undocumented CALL
		case 0xED: call(); break; // undocumented CALL
		case 0xFD: call(); break; // undocumented CALL

		case 0xC4: call(m_flagZ == false); break; // CNZ
		case 0xCC: call(m_flagZ == true); break; // CZ
		case 0xD4: call(m_flagC == false); break; // CNC
		case 0xDC: call(m_flagC == true); break; // CC
		case 0xE4: call(m_flagP == false); break; // CPO
		case 0xEC: call(m_flagP == true); break; // CPE
		case 0xF4: call(m_flagS == false); break; // CP
		case 0xFC: call(m_flagS == true); break; // CM

		case 0xC9: ret(); break; // RET
		case 0xD9: ret(); break; // undocumented RET
		case 0xC0: ret_cond(m_flagZ == false); break; // RNZ
		case 0xC8: ret_cond(m_flagZ == true); break; // RZ
		case 0xD0: ret_cond(m_flagC == false); break; // RNC
		case 0xD8: ret_cond(m_flagC == true); break; // RC
		case 0xE0: ret_cond(m_flagP == false); break; // RPO
		case 0xE8: ret_cond(m_flagP == true); break; // RPE
		case 0xF0: ret_cond(m_flagS == false); break; // RP
		case 0xF8: ret_cond(m_flagS == true); break; // RM

		case 0xC7: rst(0x00); break; // RST 0
		case 0xCF: rst(0x08); break; // RST 1
		case 0xD7: rst(0x10); break; // RST 2
		case 0xDF: rst(0x18); break; // RST 3
		case 0xE7: rst(0x20); break; // RST 4
		case 0xEF: rst(0x28); break; // RST 5
		case 0xF7: rst(0x30); break; // RST 6
		case 0xFF: rst(0x38); break; // RST 7

		case 0xDB: in_d(); break; // IN
		case 0xD3: out_d(); break; // OUT

		case 0xF3: m_INTE = false; break; // DI
		case 0xFB: m_INTE = true; m_eiPending = true; break; // EI
		case 0x76: m_HLTA = true; break; // HLT

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

uint8_t dev::I8080::ReadInstrMovePC()
{
	DebugMemStats(m_pc, Debugger::MemAccess::RUN, Memory::AddrSpace::RAM);
	uint8_t op_code = MemoryRead(m_pc, Memory::AddrSpace::RAM);
	m_pc++;
	return op_code;
}

uint8_t dev::I8080::ReadByte(uint32_t _addr, Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM)
{
	DebugMemStats(_addr, Debugger::MemAccess::READ, _addrSpace);
	return MemoryRead(_addr, _addrSpace);
}

void dev::I8080::WriteByte(uint32_t _addr, uint8_t _value, Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM)
{
	MemoryWrite(_addr, _value, _addrSpace);
	DebugMemStats(_addr, Debugger::MemAccess::WRITE, _addrSpace);
}

uint8_t dev::I8080::ReadByteMovePC(Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM)
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

uint8_t dev::I8080::i8080_get_flags()
{
	int psw = 0;
	psw |= m_flagS ? 1 << 7 : 0;
	psw |= m_flagZ ? 1 << 6 : 0;
	psw |= m_flagAC ? 1 << 4 : 0;
	psw |= m_flagP ? 1 << 2 : 0;
	psw |= m_flagC ? 1 << 0 : 0;

	psw |= m_flagUnused1 ? 1 << 1 : 0;
	psw |= m_flagUnused3 ? 1 << 1 : 0;
	psw |= m_flagUnused5 ? 1 << 1 : 0;
	return (uint8_t)psw;
}

uint16_t dev::I8080::i8080_get_af()
{
	return (uint16_t)(m_a << 8 | i8080_get_flags());
}

void dev::I8080::i8080_set_flags(uint8_t psw)
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

uint16_t dev::I8080::i8080_get_bc() 
{ 
	return (m_b << 8) | m_c; 
}

void dev::I8080::i8080_set_bc(uint16_t val)
{
	m_b = (uint8_t)(val >> 8);
	m_c = (uint8_t)(val & 0xFF);
}

uint16_t dev::I8080::i8080_get_de()
{
	return (uint16_t)((m_d << 8) | m_e);
}

void dev::I8080::i8080_set_de(uint16_t val)
{
	m_d = (uint8_t)(val >> 8);
	m_e = (uint8_t)(val & 0xFF);
}

uint16_t dev::I8080::i8080_get_hl()
{
	return (uint16_t)((m_h << 8) | m_l);
}

void dev::I8080::i8080_set_hl(uint16_t val)
{
	m_h = (uint8_t)(val >> 8);
	m_l = (uint8_t)(val & 0xFF);
}

////////////////////////////////////////////////////////////////////////////
//
// Instruction helpers
//
////////////////////////////////////////////////////////////////////////////

static constexpr bool parity_table[]
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
	return parity_table[_val];
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
void dev::I8080::rrc()
{
	m_flagC = (m_a & 1) == 1;
	m_a = (uint8_t)(m_a >> 1);
	m_a |= (uint8_t)(m_flagC ? 1 << 7 : 0);
}

// rotate register A left with the carry flag
void dev::I8080::ral()
{
	bool cy = m_flagC;
	m_flagC = m_a >> 7 == 1;
	m_a = (uint8_t)(m_a << 1);
	m_a |= (uint8_t)(cy ? 1 : 0);
}

// rotate register A right with the carry flag
void dev::I8080::rar()
{
	bool cy = m_flagC;
	m_flagC = (m_a & 1) == 1;
	m_a = (uint8_t)(m_a >> 1);
	m_a |= (uint8_t)(cy ? 1 << 7 : 0);
}

void dev::I8080::mov_r_r(uint8_t& _ddd, uint8_t _sss)
{
	if (m_machineCycle == 0)
	{
		m_TMP = _sss;
	}
	else
	{
		_ddd = m_TMP;
	}
}

void dev::I8080::load_r_p(uint8_t& _ddd, uint16_t addr)
{
	if (m_machineCycle == 1)
	{
		_ddd = ReadByte(addr);
	}
}

void dev::I8080::mov_m_r(uint8_t _sss)
{
	if (m_machineCycle == 0)
	{
		m_TMP = _sss;
	}
	else
	{
		WriteByte(i8080_get_hl(), m_TMP);
	}
}

void dev::I8080::mvi_r_d(uint8_t& _ddd)
{
	if (m_machineCycle == 1)
	{
		_ddd = ReadByteMovePC();
	}
}

void dev::I8080::mvi_m_d()
{
	if (m_machineCycle == 1)
	{
		m_TMP = ReadByteMovePC();
	}
	else if (m_machineCycle == 2)
	{
		WriteByte(i8080_get_hl(), m_TMP);
	}
}

void dev::I8080::lda()
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
		m_a = ReadByte((uint16_t)(m_W << 8 | m_Z));
	}
}

void dev::I8080::sta()
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
		WriteByte((uint16_t)(m_W << 8 | m_Z), m_a);
	}
}

void dev::I8080::stax(uint16_t _addr)
{
	if (m_machineCycle == 1)
	{
		WriteByte(_addr, m_a);
	}
}

void dev::I8080::lxi(uint8_t& _hb, uint8_t& _lb)
{
	if (m_machineCycle == 1)
	{
		_lb = ReadByteMovePC();
	}
	else if (m_machineCycle == 2)
	{
		_hb = ReadByteMovePC();
	}
}

void dev::I8080::lxi_sp()
{
	if (m_machineCycle == 1)
	{
		uint8_t _lb = ReadByteMovePC();
		m_sp = (uint16_t)(m_sp & 0xff00 | _lb);
	}
	else if (m_machineCycle == 2)
	{
		uint8_t _hb = ReadByteMovePC();
		m_sp = (uint16_t)(_hb << 8 | m_sp & 0xff);
	}
}

void dev::I8080::lhld()
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
		m_l = ReadByte((uint16_t)(m_W << 8 | m_Z));
		m_Z++;
		m_W += (uint8_t)(m_Z == 0 ? 1 : 0);
	}
	else if (m_machineCycle == 4)
	{
		m_h = ReadByte((uint16_t)(m_W << 8 | m_Z));
	}
}

void dev::I8080::shld()
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
		WriteByte((uint16_t)(m_W << 8 | m_Z), m_l);
		m_Z++;
		m_W += (uint8_t)(m_Z == 0 ? 1 : 0);
	}
	else if (m_machineCycle == 4)
	{
		WriteByte((uint16_t)(m_W << 8 | m_Z), m_h);
	}
}

void dev::I8080::sphl()
{
	if (m_machineCycle == 1)
	{
		m_sp = i8080_get_hl();
	}
}

void dev::I8080::xchg()
{
	m_TMP = m_d;
	m_d = m_h;
	m_h = m_TMP;

	m_TMP = m_e;
	m_e = m_l;
	m_l = m_TMP;
}

void dev::I8080::xthl()
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

void dev::I8080::push(uint8_t _hb, uint8_t _lb)
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

void dev::I8080::pop(uint8_t& _hb, uint8_t& _lb)
{
	if (m_machineCycle == 1)
	{
		_lb = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
	}
	else if (m_machineCycle == 2)
	{
		_hb = ReadByte(m_sp, Memory::AddrSpace::STACK);
		m_sp++;
	}
}

// adds a value (+ an optional carry flag) to a register
void dev::I8080::add(uint8_t _a, uint8_t _b, bool _cy)
{

	m_a = (uint8_t)(_a + _b + (_cy ? 1 : 0));
	m_flagC = GetCarry(8, _a, _b, _cy);
	m_flagAC = GetCarry(4, _a, _b, _cy);
	SetZSP(m_a);
}

void dev::I8080::add_m(bool _cy)
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(i8080_get_hl());
		add(m_ACT, m_TMP, _cy);
	}
}

void dev::I8080::adi(bool _cy)
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByteMovePC();
		add(m_ACT, m_TMP, _cy);
	}
}

// substracts a uint8_t (+ an optional carry flag) from a register
// see https://stackoverflow.com/a/8037485
void dev::I8080::sub(uint8_t _a, uint8_t _b, bool _cy)
{
	add(_a, (uint8_t)(~_b), !_cy);
	m_flagC = !m_flagC;
}

void dev::I8080::sub_m(bool _cy)
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(i8080_get_hl());
		sub(m_ACT, m_TMP, _cy);
	}
}

void dev::I8080::sbi(bool _cy)
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByteMovePC();
		sub(m_ACT, m_TMP, _cy);
	}
}

void dev::I8080::dad(uint16_t _val)
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

void dev::I8080::inr(uint8_t& _ddd)
{
	if (m_machineCycle == 0)
	{
		m_TMP = _ddd;
		m_TMP++;
		m_flagAC = (m_TMP & 0xF) == 0;
		SetZSP(m_TMP);
	}
	else if (m_machineCycle == 1)
	{
		_ddd = m_TMP;
	}
}

void dev::I8080::inr_m()
{
	if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(i8080_get_hl());
		m_TMP++;
		m_flagAC = (m_TMP & 0xF) == 0;
		SetZSP(m_TMP);
	}
	else if (m_machineCycle == 2)
	{
		WriteByte(i8080_get_hl(), m_TMP);
	}
}

void dev::I8080::dcr(uint8_t& _ddd)
{
	if (m_machineCycle == 0)
	{
		m_TMP = _ddd;
		m_TMP--;
		m_flagAC = !((m_TMP & 0xF) == 0xF);
		SetZSP(m_TMP);
	}
	else if (m_machineCycle == 1)
	{
		_ddd = m_TMP;
	}
}

void dev::I8080::dcr_m()
{
	if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(i8080_get_hl());
		m_TMP--;
		m_flagAC = !((m_TMP & 0xF) == 0xF);
		SetZSP(m_TMP);
	}
	else if (m_machineCycle == 2)
	{
		WriteByte(i8080_get_hl(), m_TMP);
	}
}

void dev::I8080::inx(uint8_t& _hb, uint8_t& _lb)
{
	if (m_machineCycle == 0)
	{
		m_Z = (uint8_t)(_lb + 1);
		m_W = (uint8_t)(m_Z == 0 ? _hb + 1 : _hb);
	}
	else if (m_machineCycle == 1)
	{
		_hb = m_W;
		_lb = m_Z;
	}
}

void dev::I8080::inx_sp()
{
	if (m_machineCycle == 0)
	{
		m_Z = (uint8_t)(m_sp + 1);
		m_W = (uint8_t)(m_Z == 0 ? m_sp >> 8 + 1 : m_sp >> 8);
	}
	else if (m_machineCycle == 1)
	{
		m_sp = (uint16_t)(m_W << 8 | m_Z);
	}
}

void dev::I8080::dcx(uint8_t& _hb, uint8_t& _lb)
{
	if (m_machineCycle == 0)
	{
		m_Z = (uint8_t)(_lb - 1);
		m_W = (uint8_t)(m_Z == 0xff ? _hb - 1 : _hb);
	}
	else if (m_machineCycle == 1)
	{
		_hb = m_W;
		_lb = m_Z;
	}
}

void dev::I8080::dcx_sp()
{
	if (m_machineCycle == 0)
	{
		m_Z = (uint8_t)(m_sp - 1);
		m_W = (uint8_t)(m_Z == 0xff ? m_sp >> 8 - 1 : m_sp >> 8);
	}
	else if (m_machineCycle == 1)
	{
		m_sp = (uint16_t)(m_W << 8 | m_Z);
	}
}

// Decimal Adjust Accumulator: the eight-bit number in register A is adjusted
// to form two four-bit binary-coded-decimal digits.
// For example, if A=$2B and DAA is executed, A becomes $31.
void dev::I8080::daa()
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

	add(m_a, correction, false);
	m_flagC = cy;
}

void dev::I8080::ana(uint8_t _sss)
{
	m_ACT = m_a;
	m_TMP = _sss;
	m_a = (uint8_t)(m_ACT & m_TMP);
	m_flagC = false;
	m_flagAC = ((m_ACT | m_TMP) & 0x08) != 0;
	SetZSP(m_a);
}

void dev::I8080::ana_m()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(i8080_get_hl());
		m_a = (uint8_t)(m_ACT & m_TMP);
		m_flagC = false;
		m_flagAC = ((m_ACT | m_TMP) & 0x08) != 0;
		SetZSP(m_a);
	}
}

void dev::I8080::ani()
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
void dev::I8080::xra(uint8_t _sss)
{
	m_ACT = m_a;
	m_TMP = _sss;
	m_a = (uint8_t)(m_ACT ^ m_TMP);
	m_flagC = false;
	m_flagAC = false;
	SetZSP(m_a);
}

void dev::I8080::xra_m()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(i8080_get_hl());
		m_a = (uint8_t)(m_ACT ^ m_TMP);
		m_flagC = false;
		m_flagAC = false;
		SetZSP(m_a);
	}
}

void dev::I8080::xri()
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
void dev::I8080::ora(uint8_t _sss)
{
	m_ACT = m_a;
	m_TMP = _sss;
	m_a = (uint8_t)(m_ACT | m_TMP);
	m_flagC = false;
	m_flagAC = false;
	SetZSP(m_a);
}

void dev::I8080::ora_m()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(i8080_get_hl());
		m_a = (uint8_t)(m_ACT | m_TMP);
		m_flagC = false;
		m_flagAC = false;
		SetZSP(m_a);
	}
}

void dev::I8080::ori()
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
void dev::I8080::cmp(uint8_t _sss)
{
	m_ACT = m_a;
	m_TMP = _sss;
	uint16_t result = (uint16_t)(m_ACT - m_TMP);
	m_flagC = result >> 8 == 1;
	m_flagAC = (~(m_ACT ^ result ^ m_TMP) & 0x10) == 0x10;
	SetZSP((uint8_t)(result & 0xFF));
}

void dev::I8080::cmp_m()
{
	if (m_machineCycle == 0)
	{
		m_ACT = m_a;
	}
	else if (m_machineCycle == 1)
	{
		m_TMP = ReadByte(i8080_get_hl());
		uint16_t result = (uint16_t)(m_ACT - m_TMP);
		m_flagC = result >> 8 == 1;
		m_flagAC = (~(m_ACT ^ result ^ m_TMP) & 0x10) == 0x10;
		SetZSP((uint8_t)(result & 0xFF));
	}
}

void dev::I8080::cpi()
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

void dev::I8080::jmp(bool _condition = true)
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

void dev::I8080::pchl()
{
	if (m_machineCycle == 1)
	{
		m_pc = i8080_get_hl();
	}
}

// pushes the current pc to the stack, then jumps to an address
void dev::I8080::call(bool _condition = true)
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
		m_pc = (uint16_t)(m_W << 8 | m_Z);
	}
}

// pushes the current pc to the stack, then jumps to an address
void dev::I8080::rst(uint8_t _addr)
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
		m_Z = _addr;
		WriteByte(m_sp, (uint8_t)(m_pc & 0xff), Memory::AddrSpace::STACK);
	}
	else if (m_machineCycle == 3)
	{
		m_pc = (uint16_t)(m_W << 8 | m_Z);
	}
}

// returns from subroutine
void dev::I8080::ret()
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
		m_pc = (uint16_t)(m_W << 8 | m_Z);
	}
}

// returns from subroutine if a condition is met
void dev::I8080::ret_cond(bool _condition)
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
		m_pc = (uint16_t)(m_W << 8 | m_Z);
	}
}

void dev::I8080::in_d()
{
	if (m_machineCycle == 1)
	{
		m_W = 0;
		m_Z = ReadByteMovePC();
		m_a = Input(m_Z);
	}
}

void dev::I8080::out_d()
{
	if (m_machineCycle == 1)
	{
		m_W = 0;
		m_Z = ReadByteMovePC();
		Output(m_Z, m_a);
	}
}

