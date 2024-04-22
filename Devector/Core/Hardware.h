#pragma once
#ifndef DEV_HARDWARE_H
#define DEV_HARDWARE_H

#include <mutex>
#include <string>

#include "Utils/Types.h"
#include "Core/I8080.h"
#include "Core/Memory.h"
#include "Core/Keyboard.h"
#include "Core/IO.h"
#include "Core/Display.h"
#include "Utils/Utils.h"
#include "Utils/Result.h"
#include "Utils/TQueue.h"
#include "Utils/JsonUtils.h"
#include <thread>
#include <condition_variable>
#include <atomic>

namespace dev 
{
	class Hardware
	{
		I8080 m_cpu;
		Memory m_memory;
		Keyboard m_keyboard;
		IO m_io;
		Display m_display;

		enum class Status : int {
			RUN,
			STOP,
			EXIT
		};

	public:
		using CheckBreakFunc = std::function<bool(const GlobalAddr _addr, const uint8_t _mappingModeRam, const uint8_t _mappingPageRam)>;

		enum class Req: int {
			NONE = 0,
			SET_MEM,
			RUN,
			STOP,
			IS_RUNNING,
			EXIT,
			RESET,
			RESTART,
			EXECUTE_INSTR,
			EXECUTE_FRAME,
			EXECUTE_FRAME_NO_BREAKS,
			GET_REGS,
			GET_REG_PC,
			GET_BYTE_RAM,
			GET_WORD_STACK,
			GET_DISPLAY_DATA,
			GET_MEMORY_MODES,
			GET_GLOBAL_ADDR_RAM,
			KEY_HANDLING,
			SCROLL_VERT,
		};

        Hardware();
		~Hardware();
		auto Request(const Req _req, const nlohmann::json& _dataJ = {}) -> Result <nlohmann::json>;
		auto GetFrame(const bool _vsync) -> const Display::FrameBuffer*;
		auto GetRam() const -> const Memory::Ram*;

		void AttachCheckBreak(CheckBreakFunc* _funcP);
		void AttachDebugOnReadInstr(I8080::DebugOnReadInstrFunc* _funcP);
		void AttachDebugOnRead(I8080::DebugOnReadFunc* _funcP);
		void AttachDebugOnWrite(I8080::DebugOnWriteFunc* _funcP);

	private:
		std::atomic <CheckBreakFunc*> m_checkBreak;
		std::thread m_executionThread;
		std::thread m_reqHandlingThread;
		std::atomic<Status> m_status;
		TQueue <std::pair<Req, nlohmann::json>> m_reqs; // a request type
		TQueue <nlohmann::json> m_reqRes;				// it's a result of a request sent back 

		void Init();
		void Execution();
		void ExecuteInstruction();
		void ExecuteFrameNoBreaks();
		void ReqHandling(const bool _waitReq = false);
		void Reset();
		void Restart();
		void SetMem(const nlohmann::json& _dataJ);
		auto GetRegs() const -> nlohmann::json;
		auto GetByte(const nlohmann::json _addr, const Memory::AddrSpace _addrSpace) -> nlohmann::json;
		auto GetWord(const nlohmann::json _addr, const Memory::AddrSpace _addrSpace) -> nlohmann::json;
	};
}
#endif // !DEV_HARDWARE_H