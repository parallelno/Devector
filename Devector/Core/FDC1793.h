// Western Digital FD1793 - Floppy Disk Controller

// Based on fd1793.h for vector06sdl by Viacheslav Slavinsky, ported from vector06js
//
// Originally based upon:
// fd1793.js from Videoton TV-Computer project by Bela Babik
// https://github.com/teki/jstvc

#pragma once
#ifndef DEV_FDC1793_H
#define DEV_FDC1793_H

#include <string>
#include <vector>
#include <iostream>

//#include "options.h"
#include "Utils/Utils.h"
//#include "Core/FileSystemImage.h"

////////////////////////////////////////////////////////////////////////////////
//
// DiskImage
//
////////////////////////////////////////////////////////////////////////////////
struct DiskImage
{
	using Data = std::vector<uint8_t>;

	virtual uint8_t Get(int _idx) { return 0; }
	virtual void Set(int _idx, uint8_t _val) {}
	virtual auto Size() -> int { return 0; }
	virtual void Flush() {}
	virtual ~DiskImage() {}
};

////////////////////////////////////////////////////////////////////////////////
//
// DiskImageTemplate
//
////////////////////////////////////////////////////////////////////////////////
struct DiskImageTemplate : public DiskImage {};

////////////////////////////////////////////////////////////////////////////////
//
// DiskImageDetached
//
////////////////////////////////////////////////////////////////////////////////
class DiskImageDetached : public DiskImage
{
	Data m_data;
public:
	DiskImageDetached() = delete;
	DiskImageDetached(const Data& _data) : m_data(_data) {}

// TODO: make the loading the state transparent!
// TODO: make the saving the state transparent!
!	~DiskImageDetached() override {
		FILE* dump = fopen(".saved.fdd", "wb");
		if (dump) {
			fwrite(&m_data[0], 1, m_data.size(), dump);
		}
		fclose(dump);
	}

	auto Get(int _idx) -> uint8_t override		{ return m_data[_idx]; }
	void Set(int _idx, uint8_t _val) override	{ m_data[_idx] = _val; }
	auto Size() -> int override					{ return (int)m_data.size(); }
};

////////////////////////////////////////////////////////////////////////////////
//
// FileDiskImage
//
////////////////////////////////////////////////////////////////////////////////
class FileDiskImage : public DiskImage
{
	std::string m_path;
	Data m_data;
	bool m_loaded = false;
	bool m_dirty = false;
public:
	FileDiskImage(const std::string& _path) : m_path(_path) {}

	~FileDiskImage()
	{
		Flush();
	}

	auto Get(int _idx) -> uint8_t override		{ return m_data[_idx]; }

	void Set(int _idx, uint8_t _val) override
	{
		m_dirty = m_data[_idx] != _val;
		m_data[_idx] = _val;
	}

	// TODO: getting size is loading the data. think of refactoring it
	int Size() override {
		Load();
		return (int)m_data.size();
	}
	
	void Load()
	{
		if (!m_loaded) {
			auto res = dev::LoadFile(m_path);
			m_loaded = res;
			if (m_loaded) {
				m_data = *res;
			}
		}
	}

	void Flush() override
	{
		if (m_dirty) {
			auto tmp = util::tmpname(m_path);
			FILE * copy = fopen(tmp.c_str(), "wb");
			if (copy) {
				fwrite(&m_data[0], 1, m_data.size(), copy);
			}
			fclose(copy);

			if (0 == util::careful_rename(tmp, m_path)) {
				m_dirty = false;
			}
		}
	}
};

/*
////////////////////////////////////////////////////////////////////////////////
//
// DirectoryImage
//
////////////////////////////////////////////////////////////////////////////////
class DirectoryImage : public DiskImage
{
	std::string m_path;
	std::string dirimage;
	FileSystemImage dir;
	
	bool m_loaded;
	bool m_dirty;
public:
	DirectoryImage(const std::string& _path) 
		: m_path(_path), dir(FileSystemImage::MAX_FS_BYTES),
		  m_loaded(false), m_dirty(false)
	{}

	~DirectoryImage()
	{
		Flush();
	}

	uint8_t Get(int _idx) override
	{
		if (_idx < 0 || _idx >= Size()) 
			throw std::out_of_range("boundary error");
		return dir.data()[_idx]; 
	}

	void Set(int _idx, uint8_t _val) override
	{
		if (_idx < 0 || _idx >= Size()) 
			throw std::out_of_range("boundary error");
		bool changed = dir.data()[_idx] != _val;
		if (changed) {
			dir.data()[_idx] = _val;
			m_dirty = true;
		}
	}

// TODO: getting size is loading the data. think of refactoring it
	int Size() override {
		Load();
		return dir.data().size();
	}
	
	void Load()
	{
		if (!m_loaded) {
			dir.mount_local_dir(m_path);
			m_loaded = true;

			dirimage = m_path + "/" + "dirimage.fdd";
			m_dirty = true;
			Flush();
		}
	}

	void Flush() override
	{
		if (m_dirty) {
			std::string tmp = util::tmpname(dirimage);
			FILE * copy = fopen(tmp.c_str(), "wb");
			if (copy) {
				fwrite(&dir.data()[0], 1, dir.data().size(), copy);
			}
			fclose(copy);

			if (0 == util::careful_rename(tmp, dirimage)) {
				m_dirty = false;
			}
		}
	}
};
*/
////////////////////////////////////////////////////////////////////////////////
//
// FDisk
//
////////////////////////////////////////////////////////////////////////////////
class FDisk
{
private:
	std::string name;
public:
	std::unique_ptr<DiskImage> m_diskP;
private:
	int m_sectorsPerTrack;
	int m_sectorSize;
	int totSec;
	int m_headsNum;
	int m_tracksPerSide;
	int m_data;
	int m_track;
	int m_side;
	int m_sector;
	int m_position;
	int m_readOffset;
	int n_readLen;
	int m_readSource;
	uint8_t m_readBuffer[6];


public:
	FDisk() : m_diskP(std::make_unique<DiskImageTemplate>())
	{
		Init();
	}


	void Attach(const DiskImage::Data& _data) 
	{
		m_diskP = std::make_unique<DiskImageDetached>(_data);
		Init();
	}

	void Attach(const std::string& _file)
	{
		m_diskP = std::make_unique<FileDiskImage>(_file);
		// if (m_diskP->Size() == 0) {
		// 	dsk = std::make_unique<DirectoryImage>(file);
		// }
		Init();
	}

	void Init()
	{
		m_sectorsPerTrack = 9;
		m_sectorSize = 512;
		totSec = 720;
		m_headsNum = 1;
		m_tracksPerSide = (totSec / m_sectorsPerTrack / m_headsNum) | 0;
		m_data = 0;
		m_track = 0;
		m_side = 0;
		m_position = 0;
		// read helpers
		m_readOffset = 0;
		n_readLen = 0;
		m_readSource = 0; // 0: dsk, 1: readBuffer
		parse_v06c();
	}

	static int sector_length_to_code(int length)
	{
		switch (length) {
			case 128:   return 0;
			case 256:   return 1;
			case 512:   return 2;
			case 1024:  return 3;
		}
		dev::Log("FDC1793: wrong sector length {}", length);
		return -1;
	}

	bool isReady() const 
	{
		return m_diskP->Size() > 0;
	}

	void seek(int track, int sector, int side)
	{
		if (isReady()) {
			int offsetSector = (sector != 0) ? (sector - 1) : 0;
			m_position = (track * (m_sectorsPerTrack * m_headsNum) 
				+ (m_sectorsPerTrack * side) + offsetSector) * m_sectorSize;
			track = track;
			side = side;
			
			/*if (Options.log.fdc) {
				printf("FDC1793: disk seek position: %08x "
						"(side:%d,trk:%d,sec:%d)\n", position, side,
						track, sector);
			}*/
		}
	};

	void readSector(int sector)
	{
		n_readLen = m_sectorSize;
		m_readOffset = 0;
		m_readSource = 0;
		sector = sector;
		seek(m_track, sector, m_side);
	}

	void readAddress()
	{
		n_readLen = 6;
		m_readSource = 1;
		m_readOffset = 0;
		m_readBuffer[0] = m_track;
		m_readBuffer[1] = m_side; // invert side ? not sure
		m_readBuffer[2] = m_sector;
		m_readBuffer[3] = FDisk::sector_length_to_code(m_sectorSize);
		m_readBuffer[4] = 0;
		m_readBuffer[5] = 0;
	}

	bool Read()
	{
		bool finished = true;
		if (m_readOffset < n_readLen) {
			finished = false;
			if (m_readSource) {
				m_data = m_readBuffer[m_readOffset];
			} else {
				m_data = m_diskP->Get(m_position + m_readOffset);
			}
			m_readOffset++;
		} /*else {
			if (Options.log.fdc) {
				printf("FDC1793: read finished src:%d\n", readSource);
			}
		}*/
		//printf("FDC1793: disk read, rem: %d finished: %d\n", readLength,
		//        finished);
		return finished;
	}

	bool write_data(uint8_t data)
	{
		bool finished = true;
		if (m_readOffset < n_readLen) {
			finished = false;
			m_diskP->Set(m_position + m_readOffset, data);
			m_readOffset++;
			if (m_readOffset == n_readLen) {
				m_diskP->Flush();
			}
		}/*
		else {
			if (Options.log.fdc) {
				printf("FDC1793: Write finished: position=%08x\n", position);

				for (int i = 0; i < readLength; ++i) {
					printf("%02x ", GetDisk->get(position + i));

					if ((i + 1) % 16 == 0 || i + 1 == readLength) {
						printf("  ");
						for (int j = -15; j <= 0; ++j) {
							printf("%c", util::printable_char(
										GetDisk->get(position + i + j)));
						}
						printf("  W\n");
					}
				}
			}
		}*/
		return finished;
	}

	void writeSector(int sector)
	{
		n_readLen = m_sectorSize;
		m_readOffset = 0;
		m_readSource = 0;
		sector = sector;
		seek(m_track, sector, m_side);
	}

	// Vector-06c floppy: 2 sides, 5 sectors of 1024 bytes
	bool parse_v06c() 
	{
		static constexpr int FDD_NSECTORS = 5;

		if (!isReady()) {
			return false;
		}
		m_tracksPerSide = (m_diskP->Size() >> 10) / 2 * FDD_NSECTORS;
		m_headsNum = 2;
		m_sectorSize = 1024;
		m_sectorsPerTrack = 5;
		totSec = m_diskP->Size() / 1024;

		return true;
	};

	uint8_t read_data() const
	{
		return m_data;
	}

};

////////////////////////////////////////////////////////////////////////////////
//
// FDC1793
//
////////////////////////////////////////////////////////////////////////////////

// Delays are not implemented yet:
//   A command and between the next status: mfm 14us, fm 28us
// Reset
//  - registers cleared
//  - restore (03) command
//  - steps until !TRO0 goes low (track 0)
//
class FDC1793
{
private:
	FDisk m_disks[4];

	// port 4, parameter register: SMDH3210
	// 							S - SS or 
	// 							M - MON or Motor On
	// 							D - DDEN or Double Density
	// 							H - HLD or Hold. 1 is head on disk (it is or-ed with motor on)
	// 							3 - Drive 3 is active
	// 							2 - Drive 2 is active
	// 							1 - Drive 1 is active
	// 							0 - Drive 0 is active
	int m_paramReg;

	int m_side;  // side selected: 0 is side 0, 1 is side 1
	int m_intRQ;
	int m_data;
	int m_status;
	int m_commandTr;
	int m_track;
	int m_sector;
	int m_stepDir = 1;
	int m_lingerTime = 3;
	int m_diskSelected = 0;

	int LINGER_BEFORE;
	int LINGER_AFTER = 2;

public:
	static constexpr int ST_NOTREADY = 0x80; // sampled before read/Write
	static constexpr int ST_READONLY = 0x40;
	static constexpr int ST_HEADLOADED = 0x20;
	static constexpr int ST_RECTYPE = 0x20;
	static constexpr int ST_WRFAULT = 0x20;
	static constexpr int ST_SEEKERR = 0x10;
	static constexpr int ST_RECNF = 0x10;
	static constexpr int ST_CRCERR = 0x08;
	static constexpr int ST_TRACK0 = 0x04;
	static constexpr int ST_LOSTDATA = 0x04;
	static constexpr int ST_INDEX = 0x02;
	static constexpr int ST_DRQ = 0x02;
	static constexpr int ST_BUSY = 0x01;

	static constexpr int PRT_INTRQ = 0x01;
	static constexpr int PRT_DRQ = 0x80;

	static constexpr int CMD_READSEC = 1;
	static constexpr int CMD_READADDR = 2;
	static constexpr int CMD_WRITESEC = 3;

	enum class PortAddr : int { CMD = 0, TRACK, SECTOR, DATA, CONTROL, STATUS };

	auto GetDisk(const int _diskIdx = -1) -> FDisk&
	{
		return m_disks[_diskIdx == -1 ? m_diskSelected : _diskIdx];
	}

	void Exec()
	{
		bool finished;
		if (m_commandTr == CMD_READSEC || m_commandTr == CMD_READADDR) 
		{
			if (m_status & ST_DRQ) {
				dev::Log("FDC1793: invalid read");
				return;
			}
			finished = GetDisk().Read();
			if (finished) 
			{
				m_status &= ~ST_BUSY;
				m_intRQ = PRT_INTRQ;
			} else 
			{
				m_status |= ST_DRQ;
				m_data = GetDisk().read_data();
			}
		}
		else if (m_commandTr == CMD_WRITESEC) {
			if ((m_status & ST_DRQ) == 0) {
				finished = GetDisk().write_data(m_data);
				if (finished) {
					m_status &= ~ST_BUSY;
					m_intRQ = PRT_INTRQ;
				}
				else {
					m_status |= ST_DRQ;
				}
			}
		} 
		else {
			// finish lingering
			m_status &= ~ST_BUSY;
		}
	}

	int Read(const PortAddr _portAddr)
	{
		int result = 0;

		if (GetDisk().isReady()) {
			m_status &= ~ST_NOTREADY;
		} else{ 
			m_status |= ST_NOTREADY;
		}

		switch (_portAddr) {
			case PortAddr::STATUS:
				// to make software that waits for the controller to start happy:
				// linger -10 to 0 before setting busy flag, set busy flag
				// linger 0 to 10 before exec
				int returnStatus = m_status;
				if (m_status & ST_BUSY) {
					if (m_lingerTime < 0) {
						returnStatus &= ~ST_BUSY; // pretend that we're slow 
						++m_lingerTime;
					} else if (m_lingerTime < LINGER_AFTER) {
						++m_lingerTime;
					} else if (m_lingerTime == LINGER_AFTER) {
						++m_lingerTime;
						Exec();
						returnStatus = m_status;
					}
				}

				//m_status &= (ST_BUSY | ST_NOTREADY);
				m_intRQ = 0;
				result = returnStatus;
				break;

			case PortAddr::TRACK:
				result = m_track;
				break;

			case PortAddr::SECTOR:
				result = m_sector;
				break;

			case PortAddr::DATA:
				if (!(m_status & ST_DRQ)) {
					dev::Log("FDC1793: reading too much!");
				}
				result = m_data;
				m_status &= ~ST_DRQ;
				Exec();
				break;

/*
			case PortAddr::CONTROL:
				if (m_status & ST_BUSY) {
					Exec();
				}
				// DRQ,0,0,0,0,0,0,INTRQ
				// faster to use than FDC
				result = m_intRQ | ((m_status & ST_DRQ) ? PRT_DRQ : 0);
				break;

			default:
				printf("FDC1793: invalid port read\n");
				result = -1;
				break;
*/
		}
		return result;
	};

	void Command(const int _val)
	{
		int cmd = _val >> 4;
		int param = _val & 0x0f;
		int update = param & 1;
		int multiple = update;

		m_intRQ = 0;
		m_commandTr = 0;
		switch (cmd) {
			case 0x00: // restor, type 1
				m_intRQ = PRT_INTRQ;
				if (GetDisk().isReady()) {
					m_track = 0;
					GetDisk().seek(m_track, 1, m_side);
				} else {
					m_status |= ST_SEEKERR;
				}
				break;

			case 0x01: // seek
				GetDisk().seek(m_data, m_sector, m_side);
				m_track = m_data;
				m_intRQ = PRT_INTRQ;
				m_status |= ST_BUSY;
				m_lingerTime = LINGER_BEFORE;
				break;

			case 0x02: // step, u = 0
			case 0x03: // step, u = 1
				m_track += m_stepDir;
				if (m_track < 0) {
					m_track = 0;
				}
				m_lingerTime = LINGER_BEFORE;
				m_status |= ST_BUSY;
				break;

			case 0x04: // step in, u = 0
			case 0x05: // step in, u = 1
				m_stepDir = 1;
				m_track += m_stepDir;
				m_lingerTime = LINGER_BEFORE;
				m_status |= ST_BUSY;
				break;

			case 0x06: // step out, u = 0
			case 0x07: // step out, u = 1
				m_stepDir = -1;
				m_track += m_stepDir;
				if (m_track < 0) {
					m_track = 0;
				}
				m_lingerTime = LINGER_BEFORE;
				m_status |= ST_BUSY;
				break;

			case 0x08: // read sector, m = 0
			case 0x09: // read sector, m = 1
				m_commandTr = CMD_READSEC;
				m_status |= ST_BUSY;
				GetDisk().seek(m_track, m_sector, m_side);
				GetDisk().readSector(m_sector);
				m_lingerTime = LINGER_BEFORE;
				break;

			case 0x0A: // Write sector, m = 0
			case 0x0B: // Write sector, m = 1
				m_commandTr = CMD_WRITESEC;
				m_status |= ST_BUSY;
				m_status |= ST_DRQ;
				GetDisk().seek(m_track, m_sector, m_side);
				GetDisk().writeSector(m_sector);
				m_lingerTime = LINGER_BEFORE;
				break;

			case 0x0C: // read address
				m_commandTr = CMD_READADDR;
				m_status |= ST_BUSY;
				GetDisk().readAddress();
				m_lingerTime = LINGER_BEFORE;
				break;

			case 0x0D: // force interrupt
				break;

			case 0x0E: // read track
				dev::Log("FDC1793: 'Read Track' command is not implemented");
				break;

			case 0x0F: // Write track
				printf("FDC1793: 'Write Track' command is not implemented");
				break;

			default:
				break;
		}
	};

	void Write(const PortAddr _portAddr, const int _val)
	{
		switch (_portAddr) {
			case PortAddr::CMD:
				Command(_val);
				break;

			case PortAddr::TRACK:
				m_track = _val;
				m_status &= ~ST_DRQ;
				break;

			case PortAddr::SECTOR:
				m_sector = _val;
				m_status &= ~ST_DRQ;
				break;

			case PortAddr::DATA:
				m_data = _val;
				m_status &= ~ST_DRQ;
				Exec();
				break;

			case PortAddr::CONTROL:
				m_paramReg = _val;
				// Kishinev v06c: 0011xSAB
				// 				A - disk A
				// 				B - disk B
				// 				S - side
				m_diskSelected = _val & 3;
				m_side = ~((_val >> 2) & 1); // invert side
				break;
			default:
				dev::Log("FDC1793: Write to unknown port address: {}", _portAddr);
				return;
		}
	}
};
#endif //!DEV_FDC1793_H