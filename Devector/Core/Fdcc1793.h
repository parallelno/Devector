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

	/** FDIDisk **************************************************/
	/** This structure contains all disk image information and  **/
	/** also the result of the last SeekFDI() call.             **/
	/*************************************************************/
	struct FDIDisk
	{
		// Vector06c specifics:
		static constexpr int VECTOR_SIDES_PER_DISK = 2;
		static constexpr int VECTOR_TRACKS_PER_SIDE = 82;
		static constexpr int VECTOR_SECTORS_PER_TRACK = 5;
		static constexpr int VECTOR_SECTOR_LEN = 1024;

		enum class Format
		{
			AUTO,	// Determine format automatically
			IMG,	// ZX Spectrum disk
			MGT,	// ZX Spectrum disk, same as .DSK
			TRD,	// ZX Spectrum TRDOS disk
			FDI,	// Generic FDI image
			SCL,	// ZX Spectrum TRDOS disk
			HOBETA,	// ZX Spectrum HoBeta disk
			MSXDSK,	// MSX disk
			CPCDSK,	// CPC disk
			SF7000,	// Sega SF-7000 disk
			SAMDSK,	// Sam Coupe disk
			ADMDSK,	// Coleco Adam disk
			DDP,	// Coleco Adam tape
			SAD,	// Sam Coupe disk
			DSK,	// Generic raw disk image
			MEMORY,	// In-memory (RetroArch SRAM)
			VECTOR, // Vector06C disk
		};

		Format format;
		int  sidesPerDisk;
		int  tracksPerSide;
		int  sectorsPerTrack;
		int  sectorLen;

		uint8_t* data = nullptr;
		int  dataLen = 0;

		uint8_t header[6];	// current header, result of Seek()
		bool updated = false;		// the data is updated

		FDIDisk(const Format _format = Format::VECTOR);
	};

#pragma pack(4)
	typedef struct
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
		FDIDisk* Disk[DRIVES_MAX]; /* Disk images */
	} WD1793;
#pragma pack()

	class Fdc1793
	{
		static constexpr int VECTOR_DISK_SIZE =
			FDIDisk::VECTOR_SIDES_PER_DISK *
			FDIDisk::VECTOR_TRACKS_PER_SIDE *
			FDIDisk::VECTOR_SECTORS_PER_TRACK *
			FDIDisk::VECTOR_SECTOR_LEN;

		uint8_t m_data[VECTOR_DISK_SIZE];
		FDIDisk m_disks[DRIVES_MAX];
		WD1793 fdd;
	public:

		enum class Port : int { COMMAND = 0, STATUS = 0, TRACK = 1, SECTOR = 2, DATA = 3, READY = 4, SYSTEM = 4 };

		void Attach(const std::wstring& _path);
		auto Read(const Port _port) -> uint8_t;
		void Write(const Port _port, const uint8_t _val);

		void EjectFDI(FDIDisk* D);
		void InitFDI(FDIDisk* D);
		auto SeekFDI(FDIDisk* D, int Side, int Track, int SideID, int TrackID, int SectorID) -> uint8_t*;
		void Reset1793(WD1793* D, FDIDisk* Disks, uint8_t Eject);
		auto Read1793(WD1793* _driveP, Port _port) -> uint8_t;
		auto Write1793(WD1793* D, Port _port, uint8_t V) -> uint8_t;

	};
}

#endif // DEV_FDC1793_H