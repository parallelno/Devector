// Intel 8080 (soviet analog KR580VM80A) microprocessor core model
//
// credints
// https://github.com/superzazu/8080/blob/master/i8080.c
// https://github.com/amensch/e8080/blob/master/e8080/Intel8080/i8080.cs
// https://github.com/svofski/vector06sdl/blob/master/src/i8080.cpp

// Vector06 cpu timings:
// every Vector06c instruction consists of one to six machine cycles
// every Vector06c machine cycle consists of four active states often called t-states (T1, T2, etc)
// each t-state triggered by 3 Mhz clock

#pragma once

#include <functional>
#include <atomic>
#include <mutex>

#include "Utils/Types.h"
#include "Core/Memory.h"

namespace dev 
{
	class CpuI8080
	{
	public:
		////////////////////////////////////////////////////////////////////////////
		//
		// Get the state
		//
		////////////////////////////////////////////////////////////////////////////
#pragma pack(push, 1)
		union RegPair {
			struct {
				uint8_t l;
				uint8_t h;
			};
			uint16_t word;
			RegPair(const uint16_t _word) : word(_word) {};
			RegPair() : word(0) {};
		};
#pragma pack(pop)

#pragma pack(push, 1)
		union AF {
			struct {
				bool c : 1; // carry flag
				bool _1 : 1; // unused, always 1 in Vector06c
				bool p : 1; // parity flag
				bool _3 : 1; // unused, always 0 in Vector06c
				bool ac : 1; // auxiliary carry (half-carry) flag
				bool _5 : 1; // unused, always 0 in Vector06c
				bool z : 1; // zero flag
				bool s : 1; // sign flag
				uint8_t a : 8;	// register A
			};
			RegPair af;
			AF(const RegPair _af) : af(_af) {};
			AF(const uint16_t _af) : af(_af) {};
			AF() : af(0) {};
		};
#pragma pack(pop)

#pragma pack(push, 1)
		struct Regs {
			RegPair pc; // program counter
			RegPair sp; // stack pointer
			AF psw;	// accumulator & flags
			RegPair bc; // register pair BC
			RegPair de; // register pair DE
			RegPair hl; // register pair HL
			uint8_t ir; // internal register to fetch instructions
			uint8_t tmp; // temporary register
			uint8_t act; // temporary accumulator
			RegPair wz; // temporary address reg
		};
#pragma pack(pop)

#pragma pack(push, 1)
		union Int {
			struct {
				uint8_t mc : 4; // machine cycle index of the currently executed instruction
				bool inte : 1; // set if an iterrupt enabled
				bool iff : 1; // set by the 50 Hz interruption timer. it is ON until an iterruption call (RST7)
				bool hlta : 1; // indicates that HLT instruction is executed
				bool eiPending : 1; // if set, the interruption call is pending until the next instruction
			};
			uint8_t data;
			Int(const uint8_t _data) : data(_data) {};
			Int() : data(0) {};
		};
#pragma pack(pop)

		// defines the machine state
#pragma pack(push, 1)
		struct State {
			uint64_t cc; // clock cycles, debug related data
			Regs regs;
			Int ints;
			uint8_t opcode;
			RegPair data;
		};
#pragma pack(pop)

		State state;

		auto GetState() const -> const State&;
		uint64_t GetCC() const;
		uint16_t GetPC() const;
		uint16_t GetSP() const;
		uint16_t GetPSW() const;
		uint16_t GetBC() const;
		uint16_t GetDE() const;
		uint16_t GetHL() const;
		uint8_t GetA() const;
		uint8_t GetF() const;
		uint8_t GetB() const;
		uint8_t GetC() const;
		uint8_t GetD() const;
		uint8_t GetE() const;
		uint8_t GetH() const;
		uint8_t GetL() const;
		bool GetFlagS() const;
		bool GetFlagZ() const;
		bool GetFlagAC() const;
		bool GetFlagP() const;
		bool GetFlagC() const;
		bool GetINTE() const;
		bool GetIFF() const;
		bool GetHLTA() const;
		uint8_t GetMachineCycles() const;

		// memory + io interface
		using InputFunc = std::function <uint8_t(const uint8_t _port)>;
		using OutputFunc = std::function <void(const uint8_t _port, const uint8_t _value)>;
		using DebugOnReadInstrFunc = std::function<void(const GlobalAddr _globalAddr, const State& _state)>;
		using DebugOnReadFunc = std::function<void(const GlobalAddr _globalAddr, const uint8_t _val)>;
		using DebugOnWriteFunc = std::function<void(const GlobalAddr _globalAddr, const uint8_t _val)>;

		CpuI8080() = delete;
		CpuI8080(
			Memory& _memory,
			InputFunc _input,
			OutputFunc _output);

		void Init();
		void Reset();
		void ExecuteMachineCycle(bool _irq);
		bool IsInstructionExecuted() const;

		void AttachDebugOnReadInstr(DebugOnReadInstrFunc* _funcP);
		void AttachDebugOnRead(DebugOnReadFunc* _funcP);
		void AttachDebugOnWrite(DebugOnWriteFunc* _funcP);

	private:
		std::atomic <DebugOnReadInstrFunc*> m_debugOnReadInstr = nullptr;
		std::atomic <DebugOnReadFunc*> m_debugOnRead = nullptr;
		std::atomic <DebugOnWriteFunc*> m_debugOnWrite = nullptr;

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
		void WriteByte(const Addr _addr, uint8_t _value,
			Memory::AddrSpace _addrSpace, const uint8_t _byteNum);
		uint8_t ReadByteMovePC(Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);

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
		void DAD(RegPair _val);
		void INR(uint8_t& _regDest);
		void INRMem();
		void DCR(uint8_t& _regDest);
		void DCRMem();
		void INX(uint16_t& _regPair);
		void DCX(uint16_t& _regPair);
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
		void HLT();

	};
}