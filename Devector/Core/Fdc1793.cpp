#include "Core/Fdc1793.h"

#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
//
// FDrive
//
////////////////////////////////////////////////////////////////////////////////


dev::FDrive::FDrive(const std::wstring& _path)
{
	auto res = dev::LoadFile(_path);
	m_data = *res;
}

uint8_t dev::FDrive::Get(const int _idx) { return m_data[_idx]; }

void dev::FDrive::Set(const int _idx, const uint8_t _val)
{
	if (m_data[_idx] == _val) return;

	m_data[_idx] = _val;
	m_modified = true;
}

void dev::FDrive::Seek(const int _track, const int _sector, const int _side)
{
	int sectors = SECTORS_PER_TRACK * (_track * SIDES_PER_DISK + _side);
	int sectorAdjusted = dev::Max(0, _sector - 1); // In CHS addressing the sector numbers always start at 1
	m_position = (sectors + sectorAdjusted) * SECTOR_LEN;
	m_trackReg = _track;
	m_side = _side;
	m_sectorReg = _sector;
};

void dev::FDrive::InitReadSector(int _sector)
{
	m_readSource = ReadSource::DISK;
	m_transferLen = SECTOR_LEN;
}

void dev::FDrive::InitWriteSector(const int _sector)
{
	m_readSource = ReadSource::DISK;
	m_transferLen = SECTOR_LEN;
}

void dev::FDrive::InitReadAddress()
{
	m_readSource = ReadSource::ID_FIELD;
	m_transferLen = sizeof(m_idField);
	m_position = 0;

	// init the data
	m_idField.fields.track = m_trackReg;
	m_idField.fields.side = m_side;
	m_idField.fields.sectorAddr = m_sectorReg;
}

// if read is available, reads one byte and returns true
// TODO: setting the "finished" status to true requires an extra call. 
// Check if fixing that helps to mitigate the use of LINGER_AFTER
auto dev::FDrive::Read(uint8_t& _dataReg)
-> OpStatus
{
	if (m_transferLen == 0) return OpStatus::DONE;

	_dataReg = m_readSource == ReadSource::ID_FIELD ?
		m_idField.data[m_position++] : m_data[m_position++];

		m_transferLen--;

	return OpStatus::IN_PROCESS;
}

// writes one byte at once
auto dev::FDrive::Write(const uint8_t _data)
-> OpStatus
{
	if (m_transferLen == 0) return OpStatus::DONE;

	m_data[m_position++] = _data;
	m_transferLen--;

	return OpStatus::IN_PROCESS;
}

////////////////////////////////////////////////////////////////////////////////
//
// Fdc1793
//
////////////////////////////////////////////////////////////////////////////////

auto dev::Fdc1793::GetDrive(const int _driveIdx)
-> FDrive*
{
	return m_drives[_driveIdx == -1 ? m_drive : _driveIdx].get();
}

void dev::Fdc1793::Attach(const int _driveIdx, const std::wstring& _path)
{
	m_drives[_driveIdx & DRIVES_MAX].reset(new FDrive(_path));
}

uint8_t dev::Fdc1793::Read(const PortAddr _portAddr)
{
	//if (Options.nofdc) return 0xff;

	int result = 0;
	if (GetDrive()) m_status &= ~ST_NOTREADY;
	else m_status |= ST_NOTREADY;
	int returnStatus;
	switch (_portAddr) {
	case PortAddr::STATUS: // status

		// to make software that waits for the controller to start happy:
		// linger -10 to 0 before setting busy flag, set busy flag
		// linger 0 to 10 before Exec
		returnStatus = m_status;
		if (m_status & ST_BUSY) {
			m_execDelayTimer = dev::Max(-1, --m_execDelayTimer);
			if (m_execDelayTimer == 0) {
				Exec();
				returnStatus = m_status;
			}
		}

		m_intrq = 0;

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
			dev::Log("Fdc1793: reading too much!\n");
		}
		result = m_data;
		m_status &= ~ST_DRQ;
		Exec();
		break;

	case PortAddr::CONTROL:
		if (m_status & ST_BUSY) {
			Exec();
		}
		// DRQ,0,0,0,0,0,0,INTRQ
		// faster to use than FDC
		result = m_intrq | ((m_status & ST_DRQ) ? PRT_DRQ : 0);
		break;

	default:
		printf("Fdc1793: invalid port read\n");
		result = 0xFF;
		break;
	}
	return result;
};

void dev::Fdc1793::Write(PortAddr _portAddr, const uint8_t _val)
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
		// Kishinev fdc: 0011xSAB
		// 				A - drive A
		// 				B - drive B
		// 				S - side
		m_paramReg = _val;
		m_drive = _val & 3;
		m_side = ((~_val) >> 2) & 1; // inverted side
		break;

	default:
		dev::Log("Fdc1793: Write to unknown port address: {}", static_cast<int>(_portAddr));
		return;
	}
}

void dev::Fdc1793::Exec()
{
	if (m_cmdTransfer == Cmd::READ_SEC || m_cmdTransfer == Cmd::READ_ADDR)
	{
		if (m_status & ST_DRQ) {
			dev::Log("Fdc1793: invalid read");
			return;
		}
		switch (GetDrive()->Read(m_data))
		{
		case FDrive::OpStatus::IN_PROCESS:
			m_status |= ST_DRQ;
			break;

		case FDrive::OpStatus::DONE:
			m_status &= ~ST_BUSY;
			m_intrq = PRT_INTRQ;
			//m_cmdTransfer = Cmd::NONE;
			break;
		}

	}
	else if (m_cmdTransfer == Cmd::WRITE_SEC)
	{
		if ((m_status & ST_DRQ) == 0)
		{
			switch (GetDrive()->Read(m_data)) 
			{
			case FDrive::OpStatus::IN_PROCESS:
				m_status |= ST_DRQ;
				break;

			case FDrive::OpStatus::DONE:
				m_status &= ~ST_BUSY;
				m_intrq = PRT_INTRQ;
				//m_cmdTransfer = Cmd::NONE;
				break;
			}
		}
	}
	else {
		m_status &= ~ST_BUSY;
	}
}

void dev::Fdc1793::Command(const uint8_t _val)
{
	int cmd = _val >> 4;
	int param = _val & 0x0f;
	int update, multiple;
	update = multiple = (param & 1);
	m_intrq = 0;
	m_cmdTransfer = Cmd::NONE;
	switch (cmd) 
	{
	case 0x00: // restor, type 1
		m_intrq = PRT_INTRQ;
		if (GetDrive()) {
			m_track = 0;
			GetDrive()->Seek(m_track, 1, m_side);
		}
		else {
			m_status |= ST_SEEKERR;
		}
		break;

	case 0x01: // seek
		GetDrive()->Seek(m_data, m_sector, m_side);
		m_track = m_data;
		m_intrq = PRT_INTRQ;
		m_status |= ST_BUSY;
		m_execDelayTimer = EXEC_DELAY;
		break;

	case 0x02: // step, u = 0
	case 0x03: // step, u = 1
		m_track += m_stepdir;
		if (m_track < 0) {
			m_track = 0;
		}
		m_execDelayTimer = EXEC_DELAY;
		m_status |= ST_BUSY;
		break;

	case 0x04: // step in, u = 0
	case 0x05: // step in, u = 1
		m_stepdir = 1;
		m_track += m_stepdir;
		m_execDelayTimer = EXEC_DELAY;
		m_status |= ST_BUSY;
		//GetDrive()->seek(_track, _sector, _side);
		break;

	case 0x06: // step out, u = 0
	case 0x07: // step out, u = 1
		m_stepdir = -1;
		m_track += m_stepdir;
		if (m_track < 0) {
			m_track = 0;
		}
		m_execDelayTimer = EXEC_DELAY;
		m_status |= ST_BUSY;
		break;

	case 0x08: // read sector, m = 0
	case 0x09: // read sector, m = 1
	{
		m_cmdTransfer = Cmd::READ_SEC;
		m_status |= ST_BUSY;
		GetDrive()->Seek(m_track, m_sector, m_side);
		GetDrive()->InitReadSector(m_sector);
		m_execDelayTimer = EXEC_DELAY;
	}
	break;
	case 0x0A: // write sector, m = 0
	case 0x0B: // write sector, m = 1
	{
		m_cmdTransfer = Cmd::WRITE_SEC;
		m_status |= ST_BUSY;
		m_status |= ST_DRQ;
		GetDrive()->Seek(m_track, m_sector, m_side);
		GetDrive()->InitWriteSector(m_sector);
		m_execDelayTimer = EXEC_DELAY;
	}
	break;
	case 0x0C: // read address
		m_cmdTransfer = Cmd::READ_ADDR;
		m_status |= ST_BUSY;
		GetDrive()->InitReadAddress();
		m_execDelayTimer = EXEC_DELAY;
		break;
	case 0x0D: // force interrupt
		break;

	case 0x0E: // read track
		dev::Log("Fdc1793: 'Read Track' command is not implemented");
		break;

	case 0x0F: // Write track
		printf("Fdc1793: 'Write Track' command is not implemented");
		break;

	default:
		break;
	}
};