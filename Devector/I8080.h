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
#include <atomic>

#include "Types.h"
#include "Memory.h"

namespace dev 
{
	class I8080
	{
		uint64_t m_cc; // clock cycles. it's the debug related data
		Addr m_pc, m_sp; // program counter, stack pointer
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

	public:
		// memory + io interface
		using InputFunc = std::function <uint8_t (const uint8_t _port)>;
		using OutputFunc = std::function <void (const uint8_t _port, const uint8_t _value)>;
		using DebugOnReadInstrFunc = std::function<void(const GlobalAddr _globalAddr, const uint8_t _val, const uint8_t _dataH, const uint8_t _dataL, const Addr _hl)>;
		using DebugOnReadFunc = std::function<void(const GlobalAddr _globalAddr, const uint8_t _val)>;
		using DebugOnWriteFunc = std::function<void(const GlobalAddr _globalAddr, const uint8_t _val)>;
	
		I8080() = delete;
		I8080(
			Memory& _memory,
			InputFunc _input,
			OutputFunc _output);

		void Init();
		void Reset();
		void ExecuteMachineCycle(bool _T50HZ);
		bool IsInstructionExecuted();

		void AttachDebugOnReadInstr(DebugOnReadInstrFunc _funcP);
		void AttachDebugOnRead(DebugOnReadFunc _funcP);
		void AttachDebugOnWrite(DebugOnWriteFunc _funcP);

	private:
		std::atomic <DebugOnReadInstrFunc> m_debugOnReadInstr;
		std::atomic <DebugOnReadFunc> m_debugOnRead;
		std::atomic <DebugOnWriteFunc> m_debugOnWrite;

		Memory& m_memory;
		InputFunc Input;
		OutputFunc Output;

		void Decode();

		////////////////////////////////////////////////////////////////////////////
		//
		// i8080 Intructions
		//
		////////////////////////////////////////////////////////////////////////////

		uint8_t ReadInstrMovePC();
		uint8_t ReadByte(const Addr _addr, Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);
		void WriteByte(const Addr _addr, uint8_t _value, Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);
		uint8_t ReadByteMovePC(Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);

		////////////////////////////////////////////////////////////////////////////
		//
		// Register helpers
		//
		////////////////////////////////////////////////////////////////////////////
	public:
		uint64_t GetCC() const;
		uint16_t GetPC() const;
		uint16_t GetSP() const;
		uint16_t GetAF() const;
		uint16_t GetBC() const;
		uint16_t GetDE() const;
		uint16_t GetHL() const;
		uint8_t GetFlags() const;
		bool GetFlagS() const;
		bool GetFlagZ() const;
		bool GetFlagAC() const;
		bool GetFlagP() const;
		bool GetFlagC() const;
		bool GetINTE() const;
		bool GetIFF() const;
		bool GetHLTA() const;

	private:
		void SetBC(uint16_t _val);
		void SetDE(uint16_t _val);
		void SetHL(uint16_t _val);
		void SetFlags(uint8_t _psw);

		////////////////////////////////////////////////////////////////////////////
		//
		// Instruction helpers
		//
		////////////////////////////////////////////////////////////////////////////

		bool GetParity(uint8_t _val);
		bool GetCarry(int _bit_no, uint8_t _a, uint8_t _b, bool _cy);
		void SetZSP(uint8_t _val);
		void RLC();
		void RRC();
		void RAL();
		void RAR();
		void MOVRegReg(uint8_t& _regDest, uint8_t _regSrc);
		void LoadRegPtr(uint8_t& _regDest, Addr _addr);
		void MOVMemReg(uint8_t _sss);
		void MVIRegData(uint8_t& _regDest);
		void MVIMemData();
		void LDA();
		void STA();
		void STAX(Addr _addr);
		void LXI(uint8_t& _regH, uint8_t& _regL);
		void LXISP();
		void LHLD();
		void SHLD();
		void SPHL();
		void XCHG();
		void XTHL();
		void PUSH(uint8_t _hb, uint8_t _lb);
		void POP(uint8_t& _regH, uint8_t& _regL);
		void ADD(uint8_t _a, uint8_t _b, bool _cy);
		void ADDMem(bool _cy);
		void ADI(bool _cy);
		void SUB(uint8_t _a, uint8_t _b, bool _cy);
		void SUBMem(bool _cy);
		void SBI(bool _cy);
		void DAD(uint16_t _val);
		void INR(uint8_t& _regDest);
		void INRMem();
		void DCR(uint8_t& _regDest);
		void DCRMem();
		void INX(uint8_t& _regH, uint8_t& _regL);
		void INXSP();
		void DCX(uint8_t& _regH, uint8_t& _regL);
		void DCXSP();
		void DAA();
		void ANA(uint8_t _sss);
		void AMAMem();
		void ANI();
		void XRA(uint8_t _sss);
		void XRAMem();
		void XRI();
		void ORA(uint8_t _sss);
		void ORAMem();
		void ORI();
		void CMP(uint8_t _sss);
		void CMPMem();
		void CPI();
		void JMP(bool _condition = true);
		void PCHL();
		void CALL(bool _condition = true);
		void RST(uint8_t _arg);
		void RET();
		void RETCond(bool _condition);
		void IN_();
		void OUT_();

	};
}
#endif // !DEV_I8080_H