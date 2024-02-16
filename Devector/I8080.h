// Intel 8080 (soviet analog KR580VM80A) microprocessor core model
//
// credints
// https://github.com/superzazu/8080/blob/master/i8080.c
// https://github.com/amensch/e8080/blob/master/e8080/Intel8080/i8080.cs
// https://github.com/svofski/vector06sdl/blob/master/src/i8080.cpp

// Vector06 cpu timings:
// every Vector06c instruction consists of one to six machine cycles
// every Vector06c machine cycle consists of four active states aften called t-states (T1, T2, etc)
// each t-state triggered by 3 Mhz clock clock

#pragma once
#ifndef DEV_I8080_H
#define DEV_I8080_H

#include <functional>

#include "Memory.h"
#include "Debugger.h"

namespace dev 
{
	class I8080
	{
	public:
		uint64_t m_cc; // clock cycles. it's the debug related data
		uint16_t m_pc, m_sp; // program counter, stack pointer
		uint8_t m_a, m_b, m_c, m_d, m_e, m_h, m_l; // registers
		uint8_t m_instructionRegister; // an internal register that stores the fetched instruction

		// Arithmetic and Logic Unit (ALU)
		uint8_t m_TMP;    // an 8-bit temporary register
		uint8_t m_ACT;    // an 8-bit temporary accumulator
		uint8_t m_W;      // an 8-bit temporary hi addr
		uint8_t m_Z;      // an 8-bit temporary low addr
		bool m_flagS; // sign
		bool m_flagZ; // zero
		bool m_flagAC;// auxiliary carry (half-carry)
		bool m_flagP; // parity
		bool m_flagC; // carry
		bool m_flagUnused1; // unused, always 1 in Vector06c
		bool m_flagUnused3; // unused, always 0 in Vector06c
		bool m_flagUnused5; // unused, always 0 in Vector06c

		int m_machineCycle; // a machine cycle index of the currently executed instruction
		static constexpr uint64_t MACHINE_CC = 4; // a number of clock cycles one machine cycle takes
		static constexpr int INSTR_EXECUTED = 0; // machine_cycle index indicating the instruction executon is over

		// interruption
		bool m_INTE; // set if an iterrupt enabled
		bool m_IFF; // set by the 50 Hz interruption timer. it is ON until an iterruption call (RST7)
		bool m_HLTA; // indicates that HLT instruction is executed
		bool m_eiPending; // if set, the interruption call is pending until the next instruction
		static constexpr uint8_t OPCODE_RST7 = 0xff;
		static constexpr uint8_t OPCODE_HLT = 0x76;

		// memory + io interface
		using MemoryReadFunc = std::function <uint8_t (uint32_t _addr, Memory::AddrSpace _addrSpace)>;
		using MemoryWriteFunc = std::function <void (uint32_t _addr, uint8_t _value, Memory::AddrSpace _addrSpace)>;
		using InputFunc = std::function <uint8_t (uint8_t _port)>;
		using OutputFunc = std::function <void (uint8_t _port, uint8_t _value)>;
		using DebugMemStatsFunc = std::function<void(uint32_t _addr, Debugger::MemAccess _memAccess, Memory::AddrSpace _addrSpace)>;

		I8080() = delete;
		I8080(
			MemoryReadFunc _memoryRead,
			MemoryWriteFunc _memoryWrite,
			InputFunc _input,
			OutputFunc _output,
			DebugMemStatsFunc _debugMemStats);

		void Init();

	private:
		MemoryReadFunc MemoryRead;
		MemoryWriteFunc MemoryWrite;
		InputFunc Input;
		OutputFunc Output;
		DebugMemStatsFunc DebugMemStats;

		static constexpr uint8_t INSTRUCTION_MAX = 0x100;
		using InstructionAction = void(*)();
		InstructionAction m_actions[INSTRUCTION_MAX];

		void ExecuteMachineCycle(bool _T50HZ);
		void Decode();

		////////////////////////////////////////////////////////////////////////////
		//
		// i8080 Intructions
		//
		////////////////////////////////////////////////////////////////////////////

		uint8_t ReadInstrMovePC();
		uint8_t ReadByte(uint32_t _addr, Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);
		void WriteByte(uint32_t _addr, uint8_t _value, Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);
		uint8_t ReadByteMovePC(Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);

		////////////////////////////////////////////////////////////////////////////
		//
		// Register helpers
		//
		////////////////////////////////////////////////////////////////////////////

		uint8_t i8080_get_flags();
		uint16_t i8080_get_af();
		void i8080_set_flags(uint8_t _psw);
		uint16_t i8080_get_bc();
		void i8080_set_bc(uint16_t _val);
		uint16_t i8080_get_de();
		void i8080_set_de(uint16_t _val);
		uint16_t i8080_get_hl();
		void i8080_set_hl(uint16_t _val);

		////////////////////////////////////////////////////////////////////////////
		//
		// Instruction helpers
		//
		////////////////////////////////////////////////////////////////////////////

		bool GetParity(uint8_t _val);
		bool GetCarry(int _bit_no, uint8_t _a, uint8_t _b, bool _cy);
		void SetZSP(uint8_t _val);
		void RLC();
		void rrc();
		void ral();
		void rar();
		void mov_r_r(uint8_t& _ddd, uint8_t _sss);
		void load_r_p(uint8_t& _ddd, uint16_t _addr);
		void mov_m_r(uint8_t _sss);
		void mvi_r_d(uint8_t& _ddd);
		void mvi_m_d();
		void lda();
		void sta();
		void stax(uint16_t _addr);
		void lxi(uint8_t& _hb, uint8_t& _lb);
		void lxi_sp();
		void lhld();
		void shld();
		void sphl();
		void xchg();
		void xthl();
		void push(uint8_t _hb, uint8_t _lb);
		void pop(uint8_t& _hb, uint8_t& _lb);
		void add(uint8_t _a, uint8_t _b, bool _cy);
		void add_m(bool _cy);
		void adi(bool _cy);
		void sub(uint8_t _a, uint8_t _b, bool _cy);
		void sub_m(bool _cy);
		void sbi(bool _cy);
		void dad(uint16_t _val);
		void inr(uint8_t& _ddd);
		void inr_m();
		void dcr(uint8_t& _ddd);
		void dcr_m();
		void inx(uint8_t& _hb, uint8_t& _lb);
		void inx_sp();
		void dcx(uint8_t& _hb, uint8_t& _lb);
		void dcx_sp();
		void daa();
		void ana(uint8_t _sss);
		void ana_m();
		void ani();
		void xra(uint8_t _sss);
		void xra_m();
		void xri();
		void ora(uint8_t _sss);
		void ora_m();
		void ori();
		void cmp(uint8_t _sss);
		void cmp_m();
		void cpi();
		void jmp(bool _condition = true);
		void pchl();
		void call(bool _condition = true);
		void rst(uint8_t _addr);
		void ret();
		void ret_cond(bool _condition);
		void in_d();
		void out_d();

	};
}
#endif // !DEV_I8080_H