// Emulation of Soviet KR1818WG93 (КР1818ВГ93) Floppy Disk Controller aka a WD1793 analog

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

/*
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
*/
////////////////////////////////////////////////////////////////////////////////
//
// FileDiskImage
//
////////////////////////////////////////////////////////////////////////////////
class FileDiskImage : public DiskImage
{
	std::wstring m_path;
	Data m_data;
	bool m_loaded = false;
	bool m_modified = false;
public:
	FileDiskImage(const std::wstring& _path) : m_path(_path) {}

	~FileDiskImage()
	{
		Flush();
	}

	auto Get(int _idx) -> uint8_t override		{ return m_data[_idx]; }

	void Set(int _idx, uint8_t _val) override
	{
		m_modified = m_data[_idx] != _val;
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
		if (m_modified)
		{
			m_modified = dev::SaveFile(m_path, m_data);
		}
	}
};

////////////////////////////////////////////////////////////////////////////////
//
// FDisk
//
////////////////////////////////////////////////////////////////////////////////
class FDisk
{
public:
	std::unique_ptr<DiskImage> m_diskP;
private:
	enum class ReadSource : int { DISK = 0, BUFFER = 1 };

	int m_sidesTotal;
	int m_sectorsTotal;
	int m_sectorSize;		// in bytes
	int m_tracksPerSide;
	int m_sectorsPerTrack;

	int m_data;		// lastly read byte
	int m_side;		// current side
	int m_track;	// current track
	int m_sector;	// current sector
	int m_position;	// current position in bytes

	// read helpers
	ReadSource m_readSource;
	int m_readOffset;
	int m_readLen;
	uint8_t m_readBuffer[6];


public:
	FDisk() : m_diskP(std::make_unique<DiskImageTemplate>())
	{
		Init();
	}

	void Attach(const std::string& _file)
	{
		m_diskP = std::make_unique<FileDiskImage>(_file);
		Init();
	}

	void Init()
	{
		/*
		// ? Common FDD specifics ?
		m_sidesTotal = 1;
		m_sectorsTotal = 720;
		m_sectorSize = 512;
		m_sectorsPerTrack = 9;
		*/

		// Vector 06c FFD specifics
		m_sidesTotal = 2;
		m_sectorSize = 1024;
		m_sectorsPerTrack = 5;
		m_sectorsTotal = m_diskP->Size() / m_sectorSize;
		m_tracksPerSide = m_sectorsTotal / m_sidesTotal  / m_sectorsPerTrack;

		// current state
		m_side = 0;
		m_data = 0;
		m_track = 0;
		m_position = 0;

		// read helpers
		m_readSource = ReadSource::DISK;
		m_readOffset = 0;
		m_readLen = 0;
	}

	static int SectorSizeToCode(int _sectorSize)
	{
		switch (_sectorSize) {
			case 128:   return 0;
			case 256:   return 1;
			case 512:   return 2;
			case 1024:  return 3;
		}
		dev::Log("FDC1793: wrong sector length {}", _sectorSize);
		return -1;
	}

	bool IsReady() const
	{
		return m_diskP->Size() > 0;
	}

	void Seek(int _track, int _sector, int _side)
	{
		if (IsReady())
		{
			_sector = dev::Max(0, _sector - 1); // In CHS addressing the sector numbers always start at 1
			int sectors = m_sectorsPerTrack * (_track * m_sidesTotal + _side);

			m_position = (sectors + _sector) * m_sectorSize;
			m_track = _track;
			m_side = _side;

			// TODO: find out why it is not setting m_sector
		}
	};

	void ReadSector(int _sector)
	{
		m_readSource = ReadSource::DISK;
		m_readLen = m_sectorSize;
		m_readOffset = 0;

		m_sector = _sector;
		Seek(m_track, _sector, m_side);
	}

	// TODO: think of using one func for both ReadSector and WriteSector
	void WriteSector(int _sector)
	{
		m_readSource = ReadSource::DISK;
		m_readLen = m_sectorSize;
		m_readOffset = 0;

		m_sector = _sector;
		Seek(m_track, _sector, m_side);
	}

	void ReadAddress()
	{
		m_readSource = ReadSource::BUFFER;
		m_readLen = sizeof(m_readBuffer);
		m_readOffset = 0;

		m_readBuffer[0] = m_track;
		m_readBuffer[1] = m_side; // TODO: investigate this comment left by the svofsi -> invert side ? not sure
		m_readBuffer[2] = m_sector;
		m_readBuffer[3] = FDisk::SectorSizeToCode(m_sectorSize);
		m_readBuffer[4] = 0;
		m_readBuffer[5] = 0;
	}

	// reads one byte at once
	bool Read()
	{
		bool finished = m_readOffset >= m_readLen;
		if (!finished)
		{
			m_data = m_readSource == ReadSource::BUFFER ? 
					m_readBuffer[m_readOffset] :
					m_diskP->Get(m_position + m_readOffset);

			m_readOffset++;
		}
		return finished;
	}

	// writes one byte at once
	// TODO: check if there is no perf loss flushing ~820K file every Write
	bool Write(const uint8_t _data)
	{
		bool finished = m_readOffset >= m_readLen;
		if (!finished)
		{
			m_diskP->Set(m_position + m_readOffset, _data);
			m_readOffset++;
			if (m_readOffset == m_readLen) {
				m_diskP->Flush();
			}
		}
		return finished;
	}

	uint8_t GetData() const { return m_data; }

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
	static constexpr int LINGER_BEFORE = 0;
	static constexpr int LINGER_AFTER = 2;
	
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

	FDisk m_disks[4];

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
	int m_status;

	int m_data;
	int m_side;  // side selected: 0 is side 0, 1 is side 1
	int m_track;
	int m_sector;

	int m_stepDir = 1;
	int m_lingerTime = 3;
	int m_diskSelected = 0;

	enum class Cmd { NONE, READ_SEC, READ_ADDR, WRITE_SEC};
	Cmd m_commandTr;

public:
	enum class PortAddr : int { CMD = 0, TRACK, SECTOR, DATA, CONTROL, STATUS };

	auto GetDisk(const int _diskIdx = -1) -> FDisk&
	{
		return m_disks[_diskIdx == -1 ? m_diskSelected : _diskIdx];
	}

	int Read(const PortAddr _portAddr)
	{
		int result = 0;

		if (GetDisk().IsReady()) {
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

private:
	void Command(const int _val)
	{
		int cmd = _val >> 4;
		int param = _val & 0x0f;
		int update = param & 1;
		int multiple = update;

		m_intRQ = 0;
		m_commandTr = Cmd::NONE;

		switch (cmd) {
			case 0x00: // restor, type 1
				m_intRQ = PRT_INTRQ;
				if (GetDisk().IsReady()) {
					m_track = 0;
					GetDisk().Seek(m_track, 1, m_side);
				} else {
					m_status |= ST_SEEKERR;
				}
				break;

			case 0x01: // Seek
				GetDisk().Seek(m_data, m_sector, m_side);
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
				m_commandTr = Cmd::READ_SEC;
				m_status |= ST_BUSY;
				//GetDisk().Seek(m_track, m_sector, m_side);
				GetDisk().ReadSector(m_sector);
				m_lingerTime = LINGER_BEFORE;
				break;

			case 0x0A: // Write sector, m = 0
			case 0x0B: // Write sector, m = 1
				m_commandTr = Cmd::WRITE_SEC;
				m_status |= ST_BUSY;
				m_status |= ST_DRQ;
				//GetDisk().Seek(m_track, m_sector, m_side);
				GetDisk().WriteSector(m_sector);
				m_lingerTime = LINGER_BEFORE;
				break;

			case 0x0C: // read address
				m_commandTr = Cmd::READ_ADDR;
				m_status |= ST_BUSY;
				GetDisk().ReadAddress();
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

	void Exec()
	{
		bool finished;
		if (m_commandTr == Cmd::READ_SEC || m_commandTr == Cmd::READ_ADDR)
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
				m_data = GetDisk().GetData();
			}
		}
		else if (m_commandTr == Cmd::WRITE_SEC) {
			if ((m_status & ST_DRQ) == 0) {
				finished = GetDisk().Write(m_data);
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
};

#endif //!DEV_FDC1793_H