#pragma once

#include <map>
#include <array>
#include <format>

#include "Utils/Types.h"
#include "Core/Breakpoint.h"


namespace dev
{
	class TraceLog
	{
		static const constexpr size_t TRACE_LOG_SIZE = 100000;

		struct Item
		{
			GlobalAddr globalAddr;
			uint8_t opcode;
			uint8_t dataL;
			uint8_t dataH;

			uint64_t m_cc; // clock cycles. it's the debug related data
			
			Addr m_pc, m_sp; // program counter, stack pointer
			uint8_t m_a, m_b, m_c, m_d, m_e, m_h, m_l; // registers
			uint8_t m_instructionReg; // an internal register that stores the fetched instruction
			
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
			static constexpr uint8_t OPCODE_OUT = 0xd3;

		};

		std::array <Item, TRACE_LOG_SIZE> m_items;
		size_t m_traceLogIdx = 0;
		int m_traceLogIdxViewOffset = 0;
	};


}