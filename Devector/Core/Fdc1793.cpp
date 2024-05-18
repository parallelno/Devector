#include "Core/Fdc1793.h"

/*
////////////////////////////////////////////////////////////////////////////////
//
// DiskImage
//
////////////////////////////////////////////////////////////////////////////////

dev::DiskImage::DiskImage(const std::wstring& _path)
	: m_path(_path)
{}

dev::DiskImage::~DiskImage()
{
	Flush();
}

auto dev::DiskImage::Get(int _idx) -> uint8_t { return m_lastReadByte[_idx]; }

void dev::DiskImage::Set(int _idx, uint8_t _val)
{
	m_modified = m_lastReadByte[_idx] != _val;
	m_lastReadByte[_idx] = _val;
}

// TODO: getting size is loading the data. think of refactoring it
int dev::DiskImage::Size() 
{
	Load();
	return (int)m_lastReadByte.size();
}

void dev::DiskImage::Load()
{
	if (!m_loaded) {
		auto res = dev::LoadFile(m_path);
		m_loaded = res;
		if (m_loaded) {
			m_lastReadByte = *res;
		}
	}
}

void dev::DiskImage::Flush()
{
	if (m_modified)
	{
		m_modified = dev::SaveFile(m_path, m_lastReadByte);
	}
}
*/
////////////////////////////////////////////////////////////////////////////////
//
// FDrive
//
////////////////////////////////////////////////////////////////////////////////

dev::FDrive::FDrive(const Data& _data)
{
	m_data = _data;

	/*
	// ? Common FDD specifics ?
	m_sidesTotal = 1;
	m_sectorsTotal = 720;
	m_sectorLen = 512;
	m_sectorsPerTrack = 9;
	*/

	// Vector 06c FFD specifics
	m_sidesTotal = 2;
	m_sectorLen = 1024;
	m_sectorsPerTrack = 5;
	m_sectorsTotal = m_data.size() / m_sectorLen;
	m_tracksPerSide = m_sectorsTotal / m_sidesTotal  / m_sectorsPerTrack;

	// current state
	m_side = 0;
	m_lastReadByte = 0;
	m_track = 0;
	m_position = 0;

	// read helpers
	m_readSource = ReadSource::DISK;
	m_readOffset = 0;
	m_readLen = 0;
}

dev::FDrive::FDrive(FDrive&& _other)
	: m_data(std::move(_other.m_data))
{}

void dev::FDrive::Seek(int _track, int _sector, int _side)
{
	_sector = dev::Max(0, _sector - 1); // In CHS addressing the sector numbers always start at 1
	int sectors = m_sectorsPerTrack * (_track * m_sidesTotal + _side);

	m_position = (sectors + _sector) * m_sectorLen;
	m_track = _track;
	m_side = _side;
	m_sector = _sector;
}

void dev::FDrive::ReadSector(int _sector)
{
	m_readSource = ReadSource::DISK;
	m_readLen = m_sectorLen;
	m_readOffset = 0;

	Seek(m_track, _sector, m_side);
}

// TODO: think of using one func for both ReadSector and WriteSector
void dev::FDrive::WriteSector(int _sector)
{
	m_readSource = ReadSource::DISK;
	m_readLen = m_sectorLen;
	m_readOffset = 0;

	Seek(m_track, _sector, m_side);
}

void dev::FDrive::ReadAddress()
{
	m_readSource = ReadSource::BUFFER;
	m_readLen = sizeof(m_readBuffer);
	m_readOffset = 0;

	m_readBuffer[0] = m_track;
	m_readBuffer[1] = m_side; // TODO: investigate this comment left by the svofsi -> invert side ? not sure
	m_readBuffer[2] = m_sector;
	
	// sector lengths {128, 256, 512, 1024} are associated with {0, 1, 2, 3}
	static constexpr int sectorLenCode[] = { 0, 1, 2, 3 };

	m_readBuffer[3] = sectorLenCode[m_sectorLen];
	m_readBuffer[4] = 0;
	m_readBuffer[5] = 0;
}

// reads one byte at once
// TODO: setting the "finished" status to true requires an extra call. Check if fixing that helps to mitigate the use of LINGER_AFTER
bool dev::FDrive::Read()
{
	bool finished = m_readOffset >= m_readLen;
	if (!finished)
	{
		m_lastReadByte = m_readSource == ReadSource::BUFFER ? 
				m_readBuffer[m_readOffset] :
				m_data[m_position + m_readOffset];

		m_readOffset++;
	}
	return finished;
}

// writes one byte at once
// TODO: check if there is no perf loss flushing ~820K file every Write
bool dev::FDrive::Write(const uint8_t _data)
{
	bool finished = m_readOffset >= m_readLen;
	if (!finished)
	{
		m_data[m_position + m_readOffset] = _data;
		m_readOffset++;
	}
	return finished;
}

uint8_t dev::FDrive::GetData() const { return m_lastReadByte; }

////////////////////////////////////////////////////////////////////////////////
//
// Fdc1793
//
////////////////////////////////////////////////////////////////////////////////

void dev::Fdc1793::Attach(const FDrive::Data& _data, const int _driveIdx)
{
	m_drives[_driveIdx].reset(new FDrive(_data));
}

auto dev::Fdc1793::GetDrive(const int _driveIdx)
-> FDrive*
{
	return m_drives[_driveIdx == -1 ? m_driveSelected : _driveIdx].get();
}

uint8_t dev::Fdc1793::Read(const PortAddr _portAddr)
{
	int result = 0;

	// TODO: redundant condition
	if (GetDrive()) {
		m_status &= ~ST_NOTREADY;
	} else{
		m_status |= ST_NOTREADY;
	}

	switch (_portAddr) {
		case PortAddr::STATUS:
		{
			// to make software that waits for the controller to start happy:
			// linger -10 to 0 before setting busy flag, set busy flag
			// linger 0 to 10 before exec
			int returnStatus = m_status;
			if (m_status & ST_BUSY) {
				//if (m_lingerTime < 0) {
				//	returnStatus &= ~ST_BUSY; // pretend that we're slow
				//	++m_lingerTime;
				//} else if (m_lingerTime < LINGER_AFTER) {
				//	++m_lingerTime;
				//} else if (m_lingerTime == LINGER_AFTER) {
				//	++m_lingerTime;
				Exec();
				returnStatus = m_status;
				//}
			}

			//m_status &= (ST_BUSY | ST_NOTREADY);
			m_intRQ = 0;
			result = returnStatus;
			break;
		}
		case PortAddr::TRACK:
			result = m_track;
			break;

		case PortAddr::SECTOR:
			result = m_sector;
			break;

		case PortAddr::DATA:
			if (!(m_status & ST_DRQ)) {
				dev::Log("Fdc1793: reading too much!");
			}
			result = m_lastReadByte;
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
			printf("Fdc1793: invalid port read\n");
			result = -1;
			break;
*/
	}
	return result;
}

void dev::Fdc1793::Write(const PortAddr _portAddr, const int _val)
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
			m_lastReadByte = _val;
			m_status &= ~ST_DRQ;
			Exec();
			break;

		case PortAddr::CONTROL:
			m_paramReg = _val;
			// Kishinev v06c: 0011xSAB
			// 				A - drive A
			// 				B - drive B
			// 				S - side
			m_driveSelected = _val & 3;
			//this->_side = ((~val) >> 2) & 1; // invert side
			m_side = ((~_val) >> 2) & 1; // invert side
			break;
		default:
			dev::Log("Fdc1793: Write to unknown port address: {}", static_cast<int>(_portAddr));
			return;
	}
}

void dev::Fdc1793::Command(const int _val)
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
			if (GetDrive()) {
				m_track = 0;
				GetDrive()->Seek(m_track, 1, m_side);
			} else {
				m_status |= ST_SEEKERR;
			}
			break;

		case 0x01: // Seek
			if (GetDrive()) {
				GetDrive()->Seek(m_lastReadByte, m_sector, m_side);
				m_track = m_lastReadByte;
				m_intRQ = PRT_INTRQ;
				m_status |= ST_BUSY;
			}
			break;

		case 0x02: // step, u = 0
		case 0x03: // step, u = 1
			m_track += m_stepDir;
			if (m_track < 0) {
				m_track = 0;
			}
			m_status |= ST_BUSY;
			break;

		case 0x04: // step in, u = 0
		case 0x05: // step in, u = 1
			m_stepDir = 1;
			m_track += m_stepDir;
			m_status |= ST_BUSY;
			break;

		case 0x06: // step out, u = 0
		case 0x07: // step out, u = 1
			m_stepDir = -1;
			m_track += m_stepDir;
			if (m_track < 0) {
				m_track = 0;
			}
			m_status |= ST_BUSY;
			break;

		case 0x08: // read sector, m = 0
		case 0x09: // read sector, m = 1
			if (GetDrive()) {
				m_commandTr = Cmd::READ_SEC;
				m_status |= ST_BUSY;
				//GetDrive().Seek(m_track, m_sector, m_side);
				GetDrive()->ReadSector(m_sector);
			}
			break;

		case 0x0A: // Write sector, m = 0
		case 0x0B: // Write sector, m = 1
			if (GetDrive()) {
				m_commandTr = Cmd::WRITE_SEC;
				m_status |= ST_BUSY;
				m_status |= ST_DRQ;
				//GetDrive().Seek(m_track, m_sector, m_side);
				GetDrive()->WriteSector(m_sector);
			}
			break;

		case 0x0C: // read address
			if (GetDrive()) {
				m_commandTr = Cmd::READ_ADDR;
				m_status |= ST_BUSY;
				GetDrive()->ReadAddress();
			}
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
}

void dev::Fdc1793::Exec()
{
	bool finished;
	if (m_commandTr == Cmd::READ_SEC || m_commandTr == Cmd::READ_ADDR)
	{
		if (m_status & ST_DRQ) {
			dev::Log("Fdc1793: invalid read");
			return;
		}
		if (GetDrive()) {
			finished = GetDrive()->Read();
		}
		if (finished)
		{
			m_status &= ~ST_BUSY;
			m_intRQ = PRT_INTRQ;
		} else
		{
			m_status |= ST_DRQ;
			if (GetDrive()) {
				m_lastReadByte = GetDrive()->GetData();
			}
		}
	}
	else if (m_commandTr == Cmd::WRITE_SEC) {
		if ((m_status & ST_DRQ) == 0) {
			if (GetDrive()) 
			{
				finished = GetDrive()->Write(m_lastReadByte);
				if (finished) {
					m_status &= ~ST_BUSY;
					m_intRQ = PRT_INTRQ;
				}
				else {
					m_status |= ST_DRQ;
				}
			}
		}
	}
	else {
		// finish lingering
		m_status &= ~ST_BUSY;
	}
}