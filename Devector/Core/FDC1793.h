// Emulation of Soviet KR1818WG93 (КР1818ВГ93) Floppy Disk Controller (WD1793 analog)

// based on:
// https://en.wikipedia.org/wiki/Western_Digital_FD1771
// https://github.com/svofski/vector06sdl

#pragma once
#ifndef DEV_FDC1793_H
#define DEV_FDC1793_H

#include <string>
#include <vector>

#include "Utils/Utils.h"
#include "Utils/StrUtils.h"

namespace dev {

	////////////////////////////////////////////////////////////////////////////////
	//
	// FDrive
	//
	////////////////////////////////////////////////////////////////////////////////

	class FDrive
	{
	public:
		enum class OpStatus {
			DONE,
			JUST_DONE,
			IN_PROCESS
		};
	private:
		// Vector06c specifics:
		static constexpr int SIDES_PER_DISK = 2;
		static constexpr int TRACKS_PER_SIDE = 82;
		static constexpr int SECTORS_PER_TRACK = 5;
		static constexpr int SECTOR_LEN = 1024;

		std::string m_path;
		bool m_modified = false;
		std::vector<uint8_t> m_data;

		int m_side = 0;			// current side
		int m_trackReg = 0;		// current track
		int m_sectorReg = 0;	// current sector
		int m_dataReg = 0;		// last read byte
	
		int m_position = 0;		// current position in bytes

		// read helpers
		enum class ReadSource : int { DISK = 0, ID_FIELD = 1 };
		ReadSource m_readSource = ReadSource::DISK;
		int m_transferLen = 0;

	#pragma pack(push, 1)
		union IdField {
			static const uint8_t SectorLenToField(int _length) {
				switch (_length)
				{
				case 128:   return 0;
				case 256:   return 1;
				case 512:   return 2;
				case 1024:  return 3;
				}
				return -1;
			}
			struct Fields
			{
				uint8_t track = 0;
				uint8_t side = 0;
				uint8_t sectorAddr = 0;
				uint8_t sectorLen = SectorLenToField(SECTOR_LEN);
				uint8_t crc1 = 0;
				uint8_t crc2 = 0;
			};
			Fields fields;
			uint8_t data[sizeof(Fields)];

			IdField() {};
		};
	#pragma pack(pop)
		IdField m_idField;

	public:
		FDrive(const std::wstring& _path);
		auto Get(const int _idx) -> uint8_t;
		void Set(const int _idx, const uint8_t _val);
		void Seek(const int _track, const int _sector, const int _side);
		void InitReadSector(int _sector);
		void InitWriteSector(const int _sector);
		void InitReadAddress();
		auto Read(uint8_t& _dataReg) -> OpStatus;
		auto Write(const uint8_t _data) -> OpStatus;

	};

	////////////////////////////////////////////////////////////////////////////////
	//
	// Fdc1793
	//
	////////////////////////////////////////////////////////////////////////////////

	// Delays, not yet implemented:
	//   A command and between the next status: mfm 14us, fm 28us
	// Reset
	//  - registers cleared
	//  - restore (03) command
	//  - steps until !TRO0 goes low (track 0)
	//

	class Fdc1793
	{
	public:
		enum class PortAddr : int { CMD = 0, TRACK, SECTOR, DATA, CONTROL, STATUS };

	private:
		// Common status bits :
		static constexpr int ST_NOTREADY		= 0x80; // TODO: not fully supported. old comment: sampled before read/Write
		//static constexpr int ST_READONLY		= 0x40;
		//static constexpr int ST_HEADLOADED	= 0x20;
		//static constexpr int ST_RECTYPE		= 0x20;
		//static constexpr int ST_WRFAULT		= 0x20;
		static constexpr int ST_SEEKERR			= 0x10; // TODO: it is not fully supported
		//static constexpr int ST_RECNF			= 0x10;
		//static constexpr int ST_CRCERR		= 0x08;
		//static constexpr int ST_TRACK0		= 0x04;
		//static constexpr int ST_LOSTDATA		= 0x04;
		//static constexpr int ST_INDEX			= 0x02;
		static constexpr int ST_DRQ				= 0x02; // Data request pending
		static constexpr int ST_BUSY			= 0x01; // Controller is executing a command

		static constexpr int PRT_INTRQ = 0x01;
		static constexpr int PRT_DRQ = 0x80;

		static constexpr int EXEC_DELAY = 2;

		static constexpr int DRIVES_MAX = 4;
		std::unique_ptr<FDrive> m_drives[DRIVES_MAX];

		enum class Cmd { NONE = 0, READ_SEC = 1, READ_ADDR = 2, WRITE_SEC = 3};
		Cmd m_cmdTransfer;

		// Parameter register
		// some fdc: encoded as SMDH3210
		//				S - SS ???
		//				M - Motor. off = 0, on = 1
		//				D - Double density. off = 0, on = 1
		//				H - Hold. head on disk = 1 (it is or-ed with motor on)
		//				0, 1, 2, 3 - Drive select. actve = 1

		// Kishinev fdc: encoded as 0011xSAB
		// 				A - drive A
		// 				B - drive B
		// 				S - side
		int m_paramReg = 0; 

		int m_drive = 0;
		int m_side;
		int m_intrq = 0;
		uint8_t m_data = 0;
		int m_status = 0;
		int m_track = 0;
		int m_sector = 1;
		int m_stepdir = 1;
		int m_execDelayTimer = 3;

		auto GetDrive(const int _driveIdx = -1) -> FDrive*;
		void Exec();
		void Command(const uint8_t _val);

	public:
		void Attach(const int _driveIdx, const std::wstring& _path);
		auto Read(const PortAddr _portAddr) -> uint8_t;
		void Write(PortAddr _portAddr, const uint8_t _val);
	};
}
#endif //!DEV_FDC1793_H