// Emulation of Soviet KR1818WG93 (КР1818ВГ93) Floppy Disk Controller (WD1793 analog)

// based on:
// https://en.wikipedia.org/wiki/Western_Digital_FD1771
// https://github.com/EremusOne/ESPectrum/blob/master/src/wd1793.cpp
// https://github.com/svofski/vector06sdl
// https://github.com/teki/jstvc

#pragma once
#ifndef DEV_FDC1793_H
#define DEV_FDC1793_H

#include <string>
#include <vector>
#include <array>

#include "Utils/Utils.h"

namespace dev
{
	/*
	////////////////////////////////////////////////////////////////////////////////
	//
	// DiskImage
	//
	////////////////////////////////////////////////////////////////////////////////
	class DiskImage
	{
		using Data = std::vector<uint8_t>;

		Data m_lastReadByte;
		bool m_modified = false;
	public:
		DiskImage() = delete;
		DiskImage(const std::wstring& _path);
		~DiskImage();

		auto Get(int _idx) -> uint8_t;
		void Set(int _idx, uint8_t _val);
		auto Size() -> int;
		void Load();
		void Flush();
	};
	*/
	////////////////////////////////////////////////////////////////////////////////
	//
	// FDrive
	//
	////////////////////////////////////////////////////////////////////////////////
	class FDrive
	{
	public:
		using Data = std::vector<uint8_t>;
	private:
		bool m_modified = false;
		Data m_data;

		int m_sidesTotal;
		int m_sectorsTotal;
		int m_sectorLen;		// in bytes
		int m_tracksPerSide;
		int m_sectorsPerTrack;

		int m_lastReadByte;		// lastly read byte
		int m_side;		// current side
		int m_track;	// current track
		int m_sector;	// current sector
		int m_position;	// current position in bytes

		// read helpers
		enum class ReadSource : int { DISK = 0, BUFFER = 1 };
		ReadSource m_readSource;
		int m_readOffset;
		int m_readLen;
		uint8_t m_readBuffer[6];

	public:
		FDrive(const Data& _data);
		FDrive(FDrive&& _other);
		FDrive() = delete;
		void Seek(int _track, int _sector, int _side);
		void ReadSector(int _sector);
		void WriteSector(int _sector);
		void ReadAddress();
		bool Read();
		bool Write(const uint8_t _data);
		auto GetData() const -> uint8_t;
	};

	////////////////////////////////////////////////////////////////////////////////
	//
	// Fdc1793
	//
	////////////////////////////////////////////////////////////////////////////////

	// Delays are not implemented yet:
	//   A command and between the next status: mfm 14us, fm 28us
	// Reset
	//  - registers cleared
	//  - restore (03) command
	//  - steps until !TRO0 goes low (track 0)
	//
	class Fdc1793
	{
		static constexpr int DRIVES_MAX = 4;
		std::array<std::unique_ptr<FDrive>, DRIVES_MAX> m_drives;
		
		union StatusReg {
			struct {
				uint8_t NOT_READY : 8;
				uint8_t WRITE_PROTECT : 7;
				uint8_t HEAD_LOADED : 6;
				uint8_t SEEK_ERROR : 5;
				uint8_t CRC_ERROR : 4;
				uint8_t TRACK_0 : 3;
				uint8_t INDEX : 2;
				uint8_t BUSY : 1;
			};
			struct {
				uint8_t DRQ : 2;
			};
		};
		int m_status;
		StatusReg m_status1;

		static constexpr int ST_NOTREADY = 0x80; // TODO: not fully supported. old comment: sampled before read/Write
		//static constexpr int ST_READONLY = 0x40;
		//static constexpr int ST_HEADLOADED = 0x20;
		//static constexpr int ST_RECTYPE = 0x20;
		//static constexpr int ST_WRFAULT = 0x20;
		static constexpr int ST_SEEKERR = 0x10; // TODO: it is not fully supported
		//static constexpr int ST_RECNF = 0x10;
		//static constexpr int ST_CRCERR = 0x08;
		//static constexpr int ST_TRACK0 = 0x04;
		//static constexpr int ST_LOSTDATA = 0x04;
		//static constexpr int ST_INDEX = 0x02;
		static constexpr int ST_DRQ = 0x02;
		static constexpr int ST_BUSY = 0x01;

		static constexpr int PRT_INTRQ = 0x01;
		static constexpr int PRT_DRQ = 0x80;

		// port 4, parameter register: SMDH3210
		// 							S - SS or ???
		// 							M - MON or Motor On
		// 							D - DDEN or Double Density
		// 							H - HLD or Hold. 1 is head on disk (it is or-ed with motor on)
		// 							3 - Drive 3 is active
		// 							2 - Drive 2 is active
		// 							1 - Drive 1 is active
		// 							0 - Drive 0 is active
		int m_paramReg;

		int m_intRQ;

		int m_lastReadByte;
		int m_side;
		int m_track;
		int m_sector;

		int m_stepDir = 1;
		int m_driveSelected = 0;

		enum class Cmd { NONE, READ_SEC, READ_ADDR, WRITE_SEC};
		Cmd m_commandTr;

	public:
		enum class PortAddr : int { CMD = 0, TRACK, SECTOR, DATA, CONTROL, STATUS };

		void Attach(const FDrive::Data& _data, const int _driveIdx);
		auto GetDrive(const int _driveIdx = -1) -> FDrive*;
		auto Read(const PortAddr _portAddr) -> uint8_t;
		void Write(const PortAddr _portAddr, const int _val);

	private:
		void Command(const int _val);
		void Exec();
	};
}
#endif //!DEV_FDC1793_H