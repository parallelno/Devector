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

/*
static char printable_char(const int c)
{
	return (c >= ' ' && c < 128) ? c : '.';
}
*/

////////////////////////////////////////////////////////////////////////////////
//
// FDrive
//
////////////////////////////////////////////////////////////////////////////////

class FDrive
{
	static const uint8_t SectorLenToField(int _length)
	{
		switch (_length)
		{
		case 128:   return 0;
		case 256:   return 1;
		case 512:   return 2;
		case 1024:  return 3;
		}
		return -1;
	}

	// Vector06c specifics:
	static constexpr int SECTORS_PER_TRACK = 5;
	static constexpr int SIDES_PER_DISK = 2;
	static constexpr int TRACKS_PER_SIDE = 82;
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
	int m_readOffset = 0;
	int m_readLen = 0;

#pragma pack(push, 1)
	union IdField {
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
	FDrive(const std::wstring& _path)
	{
		auto res = dev::LoadFile(_path);
		m_data = *res;
	}

	uint8_t Get(const int _idx)
	{
		return m_data[_idx];
	}

	void Set(const int _idx, const uint8_t _val)
	{
		if (m_data[_idx] == _val) return;

		m_data[_idx] = _val;
		m_modified = true;
	}

	void Seek(const int _track, const int _sector, const int _side)
	{
		int sectors = SECTORS_PER_TRACK * (_track * SIDES_PER_DISK + _side);
		int sectorAdjusted = dev::Max(0, _sector - 1); // In CHS addressing the sector numbers always start at 1
		m_position = (sectors + sectorAdjusted) * SECTOR_LEN;
		m_trackReg = _track;
		m_side = _side;
		m_sectorReg = _sector;
	};

	void ReadSector(int _sector)
	{
		m_readSource = ReadSource::DISK;
		m_readLen = SECTOR_LEN;
		m_readOffset = 0;
	}

	void WriteSector(const int _sector)
	{
		m_readSource = ReadSource::DISK;
		m_readLen = SECTOR_LEN;
		m_readOffset = 0;
	}

	void ReadAddress()
	{
		m_readSource = ReadSource::ID_FIELD;
		m_readLen = sizeof(m_idField);
		m_readOffset = 0;

		m_idField.fields.track = m_trackReg;
		m_idField.fields.side = m_side;
		m_idField.fields.sectorAddr = m_sectorReg;
	}

	// reads one byte at once
	// TODO: setting the "finished" status to true requires an extra call. 
	// Check if fixing that helps to mitigate the use of LINGER_AFTER
	bool Read()
	{
		bool finished = m_readOffset >= m_readLen;
		if (!finished)
		{
			m_dataReg = m_readSource == ReadSource::ID_FIELD ?
				m_idField.data[m_readOffset] :
				m_data[m_position + m_readOffset];

			m_readOffset++;
		}
		return finished;
	}

	// writes one byte at once
	bool Write(const uint8_t _data)
	{
		bool finished = m_readOffset >= m_readLen;
		if (!finished)
		{
			m_data[m_position + m_readOffset] = _data;
			m_readOffset++;
		}
		return finished;
	}

	uint8_t GetDataReg() const { return m_dataReg; }

};

////////////////////////////////////////////////////////////////////////////////
//
// Fdc1793
//
////////////////////////////////////////////////////////////////////////////////

class Fdc1793
{
private:
	static constexpr int DRIVES_MAX = 4;

	std::unique_ptr<FDrive> _disks[DRIVES_MAX];
	int _pr;    // port 4, parameter register: SS,MON,DDEN,HLD,DS3,DS2,DS1,DS0
	// motor on: 1: motor on
	// double density: 1: on
	// hold: 1: head on disk (it is or-ed with motor on)
	// drive select: 1: drive active
	int _side;  // side select: 0: side 0, 1: side 1
	int _intrq;
	int _data;  // data
	int _status;
	int _command;
	int _commandtr;
	int _track;
	int _sector;
	int _stepdir;
	int _lingertime;
	int _dsksel;

	int LINGER_BEFORE = 0;
	int LINGER_AFTER;

	char debug_buf[16];
	int debug_n;

	FDrive* dsk(int n = -1) {
		return _disks[n == -1 ? _dsksel : n].get();
	}

public:
	const int ST_NOTREADY = 0x80; // sampled before read/write
	const int ST_READONLY = 0x40;
	const int ST_HEADLOADED = 0x20;
	const int ST_RECTYPE = 0x20;
	const int ST_WRFAULT = 0x20;
	const int ST_SEEKERR = 0x10;
	const int ST_RECNF = 0x10;
	const int ST_CRCERR = 0x08;
	const int ST_TRACK0 = 0x04;
	const int ST_LOSTDATA = 0x04;
	const int ST_INDEX = 0x02;
	const int ST_DRQ = 0x02;
	const int ST_BUSY = 0x01;

	const int PRT_INTRQ = 0x01;
	const int PRT_DRQ = 0x80;

	const int CMD_READSEC = 1;
	const int CMD_READADDR = 2;
	const int CMD_WRITESEC = 3;


	// Delays, not yet implemented:
	//   A command and between the next status: mfm 14us, fm 28us
	// Reset
	//  - registers cleared
	//  - restore (03) command
	//  - steps until !TRO0 goes low (track 0)
	//
	Fdc1793() : _dsksel(0)
	{
		LINGER_AFTER = 2;
		_lingertime = 3;
		_stepdir = 1;
	}

	void init()
	{
	}

	void attach(const int _driveIdx, const std::wstring& _path) 
	{
		_disks[_driveIdx & DRIVES_MAX].reset(new FDrive(_path));
	}

	FDrive* disk(int drive) {
		if (drive < 0 || drive > 3) {
			throw std::out_of_range("drives must be in range 0..3");
		}
		return _disks[drive].get();
	}

	void exec()
	{
		bool finished;
		if (_commandtr == CMD_READSEC || _commandtr == CMD_READADDR)
		{
			if (_status & ST_DRQ) {
				/*printf("Fdc1793: invalid read\n");*/
				return;
			}
			finished = dsk()->Read();
			if (finished) {
				_status &= ~ST_BUSY;
				_intrq = PRT_INTRQ;
			}
			else {
				_status |= ST_DRQ;
				_data = dsk()->GetDataReg();
			}
			/*if (Options.log.fdc) {
				printf("Fdc1793: exec - read done, "
						"finished: %d data: %02x status: %02x\n", 
						finished, _data, _status);
			}*/
		}
		else if (_commandtr == CMD_WRITESEC) {
			if ((_status & ST_DRQ) == 0) {
				finished = dsk()->Write(_data);
				if (finished) {
					_status &= ~ST_BUSY;
					_intrq = PRT_INTRQ;
				}
				else {
					_status |= ST_DRQ;
				}
			}
		}
		else {
			// finish lingering
			_status &= ~ST_BUSY;
		}
	}


	int sectorcnt = 0;
	int readcnt = 0;
	int fault = 0;

	int read(int addr)
	{
		//if (Options.nofdc) return 0xff;
		int result = 0;
		if (dsk()) _status &= ~ST_NOTREADY;
		else _status |= ST_NOTREADY;
		int returnStatus;
		switch (addr) {
		case 0: // status
			// if ((_status & ST_BUSY) && _lingertime > 0) {
			//         if (--_lingertime == 0) {
			//             exec();
			//         }
			// }
			// to make software that waits for the controller to start happy:
			// linger -10 to 0 before setting busy flag, set busy flag
			// linger 0 to 10 before exec
			returnStatus = _status;
			if (_status & ST_BUSY) {
				if (_lingertime < 0) {
					returnStatus &= ~ST_BUSY; // pretend that we're slow 
					++_lingertime;
				}
				else if (_lingertime < LINGER_AFTER) {
					++_lingertime;
				}
				else if (_lingertime == LINGER_AFTER) {
					++_lingertime;
					exec();
					returnStatus = _status;
				}
			}

			//_status &= (ST_BUSY | ST_NOTREADY);
			_intrq = 0;

			//if (fault) {
			//    printf("read status: injecting fault: result= ff\n");
			//    returnStatus = 0xff;
			//}

			result = returnStatus;
			break;

		case 1: // track
			result = _track;
			break;

		case 2: // sector
			result = _sector;
			break;

		case 3: // data
			if (!(_status & ST_DRQ)) { //throw ("invalid read");
				//if (Options.log.fdc) {
					dev::Log("Fdc1793: reading too much!\n");
				//}
			}
			result = _data;
			++readcnt;
			if (readcnt == 1024) {
				++sectorcnt;
			}
			//if (Options.log.fdc) {

				/*if (_status & ST_DRQ) {
					printf("%02x ", result);
					debug_buf[debug_n] = result;
					if (++debug_n == 16) {
						printf("  ");
						debug_n = 0;
						for (int i = 0; i < 16; ++i) {
							printf("%c", printable_char(
								debug_buf[i]));
						}
						printf("\n");
					}
				}*/
				//}
			_status &= ~ST_DRQ;
			exec();
			//console.log("Fdc1793: read data:",Utils.toHex8(result));
			break;

		case 4:
			if (_status & ST_BUSY) {
				exec();
			}
			// DRQ,0,0,0,0,0,0,INTRQ
			// faster to use than FDC
			result = _intrq | ((_status & ST_DRQ) ? PRT_DRQ : 0);
			break;

		default:
			/*printf("Fdc1793: invalid port read\n");*/
			result = -1;
			break;

		}
		if (!(_status & ST_DRQ) /*&& Options.log.fdc*/) {
			/*printf("Fdc1793: read port: %02x result: %02x status: %02x\n", addr,
				result, _status);*/
		}
		return result;
	};

	void command(int val)
	{
		int cmd = val >> 4;
		int param = val & 0x0f;
		int update, multiple;
		update = multiple = (param & 1);
		_intrq = 0;
		_command = val;
		_commandtr = 0;
		switch (cmd) {
		case 0x00: // restor, type 1
			//if (Options.log.fdc) {
				/*printf("CMD restore\n");*/
				//}
			_intrq = PRT_INTRQ;
			if (dsk()) {
				_track = 0;
				dsk()->Seek(_track, 1, _side);
			}
			else {
				_status |= ST_SEEKERR;
			}
			break;
		case 0x01: // seek
			//if (Options.log.fdc) {
				/*printf("CMD seek: %02x\n", param);*/
				//}
			dsk()->Seek(_data, _sector, _side);
			_track = _data;
			_intrq = PRT_INTRQ;
			_status |= ST_BUSY;
			_lingertime = LINGER_BEFORE;
			break;
		case 0x02: // step, u = 0
		case 0x03: // step, u = 1
			//if (Options.log.fdc) {
				/*printf("CMD step: update=%d\n", update);*/
				//}
			_track += _stepdir;
			if (_track < 0) {
				_track = 0;
			}
			_lingertime = LINGER_BEFORE;
			_status |= ST_BUSY;
			break;
		case 0x04: // step in, u = 0
		case 0x05: // step in, u = 1
			//if (Options.log.fdc) {
				/*printf("CMD step in: update=%d\n", update);*/
				//}
			_stepdir = 1;
			_track += _stepdir;
			_lingertime = LINGER_BEFORE;
			_status |= ST_BUSY;
			//dsk()->seek(_track, _sector, _side);
			break;
		case 0x06: // step out, u = 0
		case 0x07: // step out, u = 1
			//if (Options.log.fdc) {
				/*printf("CMD step out: update=%d\n", update);*/
				//}
			_stepdir = -1;
			_track += _stepdir;
			if (_track < 0) {
				_track = 0;
			}
			_lingertime = LINGER_BEFORE;
			_status |= ST_BUSY;
			break;
		case 0x08: // read sector, m = 0
		case 0x09: // read sector, m = 1
		{
			//int rsSideCompareFlag = (param & 2) >> 1;
			//int rsDelay = (param & 4) >> 2;
			//int rsSideSelect = (param & 8) >> 3;
			_commandtr = CMD_READSEC;
			_status |= ST_BUSY;
			dsk()->Seek(_track, _sector, _side);
			dsk()->ReadSector(_sector);
			debug_n = 0;
			//if (Options.log.fdc) {
				/*printf("CMD read sector m:%d p:%02x sector:%d "
					"status:%02x\n", multiple, param, _sector,
					_status);*/
				//}
			_lingertime = LINGER_BEFORE;
		}
		break;
		case 0x0A: // write sector, m = 0
		case 0x0B: // write sector, m = 1
		{
			_commandtr = CMD_WRITESEC;
			_status |= ST_BUSY;
			_status |= ST_DRQ;
			dsk()->Seek(_track, _sector, _side);
			dsk()->WriteSector(_sector);
			debug_n = 0;
			//if (Options.log.fdc) {
				/*printf("CMD write sector m:%d p:%02x sector:%d "
					"status:%02x\n", multiple, param, _sector,
					_status);*/
				//}
			_lingertime = LINGER_BEFORE;
		}
		break;
		case 0x0C: // read address
			_commandtr = CMD_READADDR;
			_status |= ST_BUSY;
			dsk()->ReadAddress();
			_lingertime = LINGER_BEFORE;
			//if (Options.log.fdc) {
				/*printf("CMD read address m:%d p:%02x status:%02x",
					multiple, param, _status);*/
				//}
			break;
		case 0x0D: // force interrupt
			//if (Options.log.fdc) {
				/*printf("CMD force interrupt\n");*/
				//}
			break;
		case 0x0E: // read track
			printf("CMD read track (not implemented)\n");
			break;
		case 0x0F: // write track
			printf("CMD write track (not implemented)\n");
			break;
		}
		// if ((_status & ST_BUSY) != 0) {
		//     _lingertime = 10;
		// }
	};

	void write(int addr, int val)
	{
		//if (Options.log.fdc) {
			//printf("Fdc1793: write [%02x]=%02x: ", addr, val);
		//}
		switch (addr) {
		case 0: // command
			//if (Options.log.fdc) {
				/*printf("COMMAND %02x: ", val);*/
				//}
			command(val);
			break;

		case 1: // track (current track)j
			//if (Options.log.fdc) {
				/*printf("set track:%d\n", val);*/
				//}
			_track = val;
			_status &= ~ST_DRQ;
			break;

		case 2: // sector (desired sector)
			//if (Options.log.fdc) {
				/*printf("set sector:%d\n", val);*/
				//}
			_sector = val;
			_status &= ~ST_DRQ;
			break;

		case 3: // data
			//if (Options.log.fdc) {
				/*printf("set data:%02x\n", val);*/
				//}
			_data = val;
			_status &= ~ST_DRQ;
			exec();
			break;

		case 4: // param reg
			_pr = val;
			// Kishinev v06c 
			// 0 0 1 1 x S A B
			_dsksel = val & 3;
			_side = ((~val) >> 2) & 1; // invert side
			//if (Options.log.fdc) {
				/*printf("set pr:%02x disk select: %d side: %d\n",
					val, val & 3, _side);*/
				//}

			// // SS,MON,DDEN,HLD,DS3,DS2,DS1,DS0
			// if (val & 1) dsk() = _disks[0];
			// else if (val & 2) dsk() = _disks[1];
			// else if (val & 4) dsk() = _disks[2];
			// else if (val & 8) dsk() = _disks[3];
			// else dsk() = _disks[0];
			// _side = (_pr & 0x80) >>> 7;
			break;
		default:
			printf("invalid port write\n");
			return;
		}
	}
};
#endif //!DEV_FDC1793_H