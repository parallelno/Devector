// Emulation of Soviet KR1818WG93 (КР1818ВГ93) Floppy Disk Controller (WD1793 analog)

// based on:
// https://github.com/libretro/fmsx-libretro/blob/master/EMULib/WD1793.c
// https://github.com/svofski/vector06sdl/blob/master/src/fd1793.h

#pragma once

#include <string>
#include <vector>

#include "core/fdd_consts.h"

namespace dev
{
	struct FDisk
	{
		uint8_t data[FDD_SIZE];
	public:

		uint8_t header[6];		// current header, result of Seek()
		bool updated = false;
		std::wstring path;
		bool mounted = false;

		size_t reads = 0;
		size_t writes = 0;

		FDisk();
		void Mount(const std::vector<uint8_t>& _data, const std::wstring& _path);
		auto GetData() -> uint8_t*;
		auto GetDisk() -> FDisk*;
	};

	class Fdc1793
	{
	public:
		enum class Port : int { 
			COMMAND = 0, 
			STATUS = 0, 
			TRACK = 1, 
			SECTOR = 2, 
			DATA = 3, 
			READY = 4, 
			SYSTEM = 4
		};

		struct Info {
			uint8_t drive;	// Current disk #
			uint8_t side;	// Current side #
			uint8_t track;	// Current track #
			uint8_t lastS;	// Last STEP direction
			uint8_t irq;	// 0x80: IRQ pending, 0x40: DRQ pending
			uint8_t wait;	// Expiration counter
			uint8_t cmd;	// Last command

			int    rwLen;		// The length of the transfered data
			size_t position;		// sector addr
		};

		struct DiskInfo {
			std::wstring path;
			bool updated = false;
			size_t reads;
			size_t writes;
			bool mounted = false;
		};
		
		static constexpr int DRIVES_MAX = 4;

	private:
		FDisk m_disks[DRIVES_MAX];

		uint8_t m_regs[5];	// Registers
		uint8_t m_drive = 0;// Current disk #
		uint8_t m_side	= 0;// Current side #
		uint8_t m_track = 0;// Current track #
		uint8_t m_lastS = 0;// Last STEP direction
		uint8_t m_irq	= 0;// 0x80: IRQ pending, 0x40: DRQ pending
		uint8_t m_wait	= 0;// Expiration counter
		uint8_t m_cmd	= 0;// Last command

		int m_rwLen = 0;	// The length of the transfered data

		uint8_t* m_ptr = nullptr; // Pointer to data
		FDisk* m_disk = nullptr; // current disk images

		auto Seek(int _side, int _track, int _sideID, int _trackID, int _sectorID) -> uint8_t*;
		void Reset();

	public:
		Fdc1793();
		void Mount(const int _driveIdx, const std::vector<uint8_t>& _data, const std::wstring& _path);
		auto Read(const Port _port) -> uint8_t;
		auto Write(const Port _port, uint8_t _val) -> uint8_t;
		auto GetFdcInfo() -> Info;
		auto GetFddInfo(const int _driveIdx) -> DiskInfo;
		auto GetFddImage(const int _driveIdx) -> const std::vector<uint8_t>;
		void ResetUpdate(const int _driveIdx);
	};
}