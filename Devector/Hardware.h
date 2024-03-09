#pragma once
#ifndef DEV_HARDWARE_H
#define DEV_HARDWARE_H

#include <mutex>
#include <string>

#include "Types.h"
#include "I8080.h"
#include "Memory.h"
#include "IO.h"
#include "Display.h"
#include "Debugger.h"
#include "Utils/Utils.h"
#include "Utils/Result.h"

namespace dev 
{
	class Hardware
	{
	public:
		I8080 m_cpu;
		Memory m_memory;
		IO m_io;
		Display m_display;
		Debugger m_debugger;

        Hardware();

        auto LoadRom(const std::wstring& _path) 
			-> Result<std::vector<uint8_t>>;
		// rasterizes the frame. For realtime emulation it should be called by the 50.08 Hz (3000000/59904) timer
		void ExecuteFrame();
		void ExecuteInstruction();

	private:
        void Init();
	};
}
#endif // !DEV_HARDWARE_H