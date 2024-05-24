// Emulation of Soviet KR1818WG93 (КР1818ВГ93) Floppy Disk Controller (WD1793 analog)

// based on:
// https://github.com/libretro/fmsx-libretro/blob/master/EMULib/WD1793.c

#pragma once
#ifndef DEV_FDC1793_H
#define DEV_FDC1793_H

#include <string>

namespace dev
{
	static constexpr int DRIVES_MAX = 4;

	struct FDisk
	{
	public:
		static constexpr int sidesPerDisk = 2;
		static constexpr int tracksPerSide = 82;
		static constexpr int sectorsPerTrack = 5;
		static constexpr int sectorLen = 1024;
		static constexpr int dataLen = sidesPerDisk * tracksPerSide * sectorsPerTrack * sectorLen;
	
	private:
		uint8_t data[dataLen];
		bool loaded = false;
	public:

		uint8_t header[6];		// current header, result of Seek()
		bool updated = false;

		FDisk();
		void Attach(const std::wstring& _path);
		auto GetData() -> uint8_t*;
		auto GetDisk() -> FDisk*;
	};

#pragma pack(4)
	struct WD1793
	{
		int  Rsrvd1[4];   /* Reserved, do not touch */

		uint8_t R[5];        /* Registers */
		uint8_t Drive;       /* Current disk # */
		uint8_t Side;        /* Current side # */
		uint8_t Track[DRIVES_MAX]; /* Current track # */
		uint8_t LastS;       /* Last STEP direction */
		uint8_t IRQ;         /* 0x80: IRQ pending, 0x40: DRQ pending */
		uint8_t Wait;        /* Expiration counter */
		uint8_t Cmd;         /* Last command */

		int  WRLength;    /* Data left to write */
		int  RDLength;    /* Data left to read */
		int  Rsrvd2;      /* Reserved, do not touch */

		/*--- Save1793() will save state above this line ----*/

		uint8_t* Ptr;        /* Pointer to data */
		FDisk* Disk = nullptr; /* current disk images */
	};
#pragma pack()

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

	private:
		FDisk m_drives[DRIVES_MAX];
		WD1793 fdd;

		auto Seek(int Side, int Track, int SideID, int TrackID, int SectorID) -> uint8_t*;
		void Reset();

	public:
		void Attach(const int _driveIdx, const std::wstring& _path);
		auto Read(const Port _port) -> uint8_t;
		auto Write(const Port _port, uint8_t _val) -> uint8_t;
	};
}

#endif // DEV_FDC1793_H