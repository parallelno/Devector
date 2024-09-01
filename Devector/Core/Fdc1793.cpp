#include "Fdc1793.h"
#include "Utils/Utils.h"
#include <cstdint>

dev::FDisk::FDisk()
	: data()
{
	header[0] = 0;
	header[1] = 0;
	header[2] = 0;
	header[3] = 0x3;	// a code associated with a sectorLen=1024
	header[4] = 0;		// CRC1 is not supported
	header[5] = 0;		// CRC2 is not supported
}

void dev::FDisk::Mount(const std::vector<uint8_t>& _data, const std::wstring& _path)
{
	path = _path;
	memcpy(data, _data.data(), _data.size());
	mounted = true;
	updated = false;
	reads = writes = 0;
}

auto dev::FDisk::GetData()
-> uint8_t*
{ return mounted ? data : nullptr; };

auto dev::FDisk::GetDisk()
-> FDisk*
{ return mounted ? this : nullptr; };

//static constexpr int FDI_SAVE_FAILED    0;  // Failed saving disk image
//static constexpr int FDI_SAVE_TRUNCATED 1;  // Truncated data while saving
//static constexpr int FDI_SAVE_PADDED    2;  // Padded data while saving
//static constexpr int FDI_SAVE_OK        3;  // Succeeded saving disk image

//static constexpr int SEEK_DELETED	= 0x40000000;

static constexpr int WD1793_COMMAND = 0;
static constexpr int WD1793_STATUS	= 0;
static constexpr int WD1793_TRACK	= 1;
static constexpr int WD1793_SECTOR	= 2;
static constexpr int WD1793_DATA	= 3;
static constexpr int WD1793_SYSTEM	= 4;
static constexpr int WD1793_READY	= 4;

static constexpr int WD1793_IRQ		= 0x80;
static constexpr int WD1793_DRQ		= 0x40;

// Common status bits:
static constexpr int F_BUSY		= 0x01; // Controller is executing a command
static constexpr int F_READONLY = 0x40; // The disk is write-protected
static constexpr int F_NOTREADY = 0x80; // The drive is not ready

// Type-1 command status:
static constexpr int F_INDEX	= 0x02; // Index mark detected
static constexpr int F_TRACK0	= 0x04; // Head positioned at track #0
//static constexpr int F_CRCERR	= 0x08; // CRC error in ID field
//static constexpr int F_SEEKERR	= 0x10; // Seek error, track not verified
static constexpr int F_HEADLOAD = 0x20; // Head loaded

// Type-2 and Type-3 command status:
static constexpr int F_DRQ		= 0x02; // Data request pending
static constexpr int F_LOSTDATA	= 0x04; // Data has been lost (missed DRQ)
static constexpr int F_ERRCODE	= 0x18; // Error code bits:
//static constexpr int F_BADDATA	= 0x08; // 1 = bad data CRC
static constexpr int F_NOTFOUND	= 0x10; // 2 = sector not found
//static constexpr int F_BADID	= 0x18; // 3 = bad ID field CRC
//static constexpr int F_DELETED	= 0x20; // Deleted data mark (when reading)
//static constexpr int F_WRFAULT	= 0x20; // Write fault (when writing)

//static constexpr int C_DELMARK	= 0x01;
static constexpr int C_SIDECOMP	= 0x02;
//static constexpr int C_STEPRATE	= 0x03;
//static constexpr int C_VERIFY	= 0x04;
//static constexpr int C_WAIT15MS	= 0x04;
static constexpr int C_LOADHEAD	= 0x08;
static constexpr int C_SIDE		= 0x08;
static constexpr int C_IRQ		= 0x08;
static constexpr int C_SETTRACK	= 0x10;
//static constexpr int C_MULTIREC	= 0x10;

static constexpr int S_DRIVE	= 0x03;
static constexpr int S_RESET	= 0x04;
static constexpr int S_HALT		= 0x08;
//static constexpr int S_SIDE		= 0x10;
//static constexpr int S_DENSITY	= 0x20;


dev::Fdc1793::Fdc1793() { Reset(); }

// Seek to given side / track / sector. Returns sector address
// (data pointer) on success or nulptr on failure.
uint8_t* dev::Fdc1793::Seek(int _side, int _track, int _sideID, int _trackID, int _sectorID)
{
	if (!m_disk) return nullptr;

	int sectors = FDisk::sectorsPerTrack * (_trackID * FDisk::sidesPerDisk + _sideID);
	int sectorAdjusted = dev::Max(0, _sectorID - 1); // In CHS addressing the sector numbers always start at 1
	int m_position = (sectors + sectorAdjusted) * FDisk::sectorLen;

	// store a header for each sector
	m_disk->header[0] = _trackID;
	m_disk->header[1] = _sideID;
	m_disk->header[2] = _sectorID;
	// FDisk has variable sector numbers and sizes
	return m_disk->GetData() + m_position;
}

// Resets the state of the WD1793 FDC.
void dev::Fdc1793::Reset()
{
	m_regs[0] = 0x00;
	m_regs[1] = 0x00;
	m_regs[2] = 0x00;
	m_regs[3] = 0x00;
	m_regs[4] = S_RESET | S_HALT;
	//m_drive = 0;
	m_side = 0;
	m_track = 0;
	m_lastS = 0;
	m_irq = 0;
	m_rwLen = 0;
	m_wait = 0;
	m_cmd = 0xD0;
}


// Reads a value from a WD1793 register.
// Returns the read data on success or 0xFF on failure (bad register address).
uint8_t dev::Fdc1793::Read(Port _reg)
{
	switch (_reg)
	{
	case Port::STATUS:
		_reg = static_cast<Port>(m_regs[0]);
		// If no disk present, set F_NOTREADY
		if (!m_disk) _reg = static_cast<Port>((int)_reg | F_NOTREADY);
		if ((m_cmd < 0x80) || (m_cmd == 0xD0))
		{
			// Keep flipping F_INDEX bit as the disk rotates (Sam Coupe)
			m_regs[0] = (m_regs[0] ^ F_INDEX) & (F_INDEX | F_BUSY | F_NOTREADY | F_READONLY | F_TRACK0);
		}
		else
		{
			// When reading status, clear all bits but F_BUSY and F_NOTREADY
			m_regs[0] &= F_BUSY | F_NOTREADY | F_READONLY | F_DRQ;
		}
		return((int)_reg);
	case Port::TRACK: [[fallthrough]];
	case Port::SECTOR:
		return(m_regs[(int)_reg]); // Return track/sector numbers
	case Port::DATA:
		if (m_rwLen)
		{
			// Read data
			m_regs[(int)_reg] = *m_ptr++;
			m_disk->reads++;
			if (--m_rwLen)
			{
				m_wait = 255; // Reset timeout watchdog
				if (!(m_rwLen & (m_disk->sectorLen - 1))) ++m_regs[2]; // Advance to the next sector if needed
			}
			else
			{
				// Read completed
				m_regs[0] &= ~(F_DRQ | F_BUSY);
				m_irq = WD1793_IRQ;
			}
		}
		return(m_regs[(int)_reg]);
	case Port::READY:
		// After some idling, stop read/write operations
		if (m_wait)
			if (!--m_wait)
			{
				m_rwLen = 0;
				m_regs[0] = (m_regs[0] & ~(F_DRQ | F_BUSY)) | F_LOSTDATA;
				m_irq = WD1793_IRQ;
			}
		// Done
		return(m_irq);
	}

	return(0xFF); // Bad register case
}

// Writes a value into the WD1793 register.
// Returns WD1793_IRQ or WD1793_DRQ
uint8_t dev::Fdc1793::Write(const Port _reg, uint8_t _val)
{
	int J;

	switch (_reg)
	{
	case Port::COMMAND:
		// Reset an interrupt request
		m_irq = 0;
		// If it is FORCE-m_irq command...
		if ((_val & 0xF0) == 0xD0)
		{
			// Reset any executing command
			m_rwLen = 0;
			m_cmd = 0xD0;
			// Either reset BUSY flag or reset all flags if BUSY=0
			if (m_regs[0] & F_BUSY)
				m_regs[0] &= ~F_BUSY;
			else
				m_regs[0] = (m_track ? 0 : F_TRACK0) | F_INDEX;
			// Cause immediate interrupt if requested
			if (_val & C_IRQ) m_irq = WD1793_IRQ;
			// Done
			return(m_irq);
		}
		// If busy, drop out
		if (m_regs[0] & F_BUSY) break;
		 // Reset status register
		m_regs[0] = 0x00;
		m_cmd = _val;
		// hadling the rest commands
		switch (_val & 0xF0)
		{
		case 0x00: // RESTORE (seek track 0)
			m_track = 0;
			m_regs[0] = F_INDEX | F_TRACK0 | (_val & C_LOADHEAD ? F_HEADLOAD : 0);
			m_regs[1] = 0;
			m_irq = WD1793_IRQ;
			break;

		case 0x10: // SEEK
			// Reset any executing command
			m_rwLen = 0;
			m_track = m_regs[3];
			m_regs[0] = F_INDEX
				| (m_track ? 0 : F_TRACK0)
				| (_val & C_LOADHEAD ? F_HEADLOAD : 0);
			m_regs[1] = m_track;
			m_irq = WD1793_IRQ;
			break;

		case 0x20: [[fallthrough]]; // STEP
		case 0x30: [[fallthrough]]; // STEP-AND-UPDATE
		case 0x40: [[fallthrough]]; // STEP-IN
		case 0x50: [[fallthrough]]; // STEP-IN-AND-UPDATE
		case 0x60: [[fallthrough]]; // STEP-OUT
		case 0x70: // STEP-OUT-AND-UPDATE
			// Either store or fetch step direction
			if (_val & 0x40)
				m_lastS = _val & 0x20;
			else
				_val = (_val & ~0x20) | m_lastS;
			// Step the head, update track register if requested
			if (_val & 0x20) {
				if (m_track) --m_track;
			} else
				++m_track;
			// Update track register if requested
			if (_val & C_SETTRACK) m_regs[1] = m_track;
			// Update status register
			m_regs[0] = F_INDEX | (m_track ? 0 : F_TRACK0);
			// TODO: @@@ WHY USING J HERE?
			//                  | (J&&(V&C_VERIFY)? 0:F_SEEKERR);
					  // Generate m_irq
			m_irq = WD1793_IRQ;
			break;

		case 0x80: [[fallthrough]];
		case 0x90: // READ-SECTORS
			// Seek to the requested sector
			m_ptr = Seek(m_side, m_track,
				_val & C_SIDECOMP ? !!(_val & C_SIDE) : m_side, 
				m_regs[1], m_regs[2]);

			// If seek successful, set up reading operation
			if (!m_ptr)
			{
				m_regs[0] = (m_regs[0] & ~F_ERRCODE) | F_NOTFOUND;
				m_irq = WD1793_IRQ;
			}
			else
			{
				m_rwLen = m_disk->sectorLen
					* (_val & 0x10 ? (m_disk->sectorsPerTrack - m_regs[2] + 1) : 1);
				m_regs[0] |= F_BUSY | F_DRQ;
				m_irq = WD1793_DRQ;
				m_wait = 255;
			}
			break;

		case 0xA0: [[fallthrough]];
		case 0xB0: // WRITE-SECTORS
			// Seek to the requested sector
			m_ptr = Seek(m_side, m_track,
				_val & C_SIDECOMP ? !!(_val & C_SIDE) : m_side, m_regs[1], m_regs[2]
			);
			// If seek successful, set up writing operation
			if (!m_ptr)
			{
				m_regs[0] = (m_regs[0] & ~F_ERRCODE) | F_NOTFOUND;
				m_irq = WD1793_IRQ;
			}
			else
			{
				m_rwLen = m_disk->sectorLen
					* (_val & 0x10 ? (m_disk->sectorsPerTrack - m_regs[2] + 1) : 1);
				m_regs[0] |= F_BUSY | F_DRQ;
				m_irq = WD1793_DRQ;
				m_wait = 255;
				m_disk->updated = true;
			}
			break;

		case 0xC0: // READ-ADDRESS
			// Read first sector address from the track
			if (!m_disk) m_ptr = 0;
			else
				for (J = 0;J < 256;++J)
				{
					m_ptr = Seek(
						m_side, m_track,
						m_side, m_track, J
					);
					if (m_ptr) break;
				}
			// If address found, initiate data transfer
			if (!m_ptr)
			{
				m_regs[0] |= F_NOTFOUND;
				m_irq = WD1793_IRQ;
			}
			else
			{
				m_ptr = m_disk->header;
				m_rwLen = 6;
				m_regs[0] |= F_BUSY | F_DRQ;
				m_irq = WD1793_DRQ;
				m_wait = 255;
			}
			break;

		case 0xE0: // READ-TRACK
			break;

		case 0xF0: // WRITE-TRACK, i.e., format
			// the full protocol is not implemented (involves parsing lead-in & lead-out);
			// it only sets the track data to 0xE5
			if (m_ptr = Seek(0, m_track, 0, m_regs[1], 1))
			{
				memset(m_ptr, 0xE5, m_disk->sectorLen * m_disk->sectorsPerTrack);
				m_disk->updated = true;
			}
			if (m_disk->sidesPerDisk > 1 && (m_ptr = Seek(1, m_track, 1, m_regs[1], 1)))
			{
				memset(m_ptr, 0xE5, m_disk->sectorLen * m_disk->sectorsPerTrack);
				m_disk->updated = true;
			}
			break;

		default: // UNKNOWN
			break;
		}
		break;

	case Port::TRACK: [[fallthrough]];
	case Port::SECTOR:
		if (!(m_regs[0] & F_BUSY)) m_regs[(int)_reg] = _val;
		break;

	case Port::DATA:
		// When writing data, store value to disk
		if (m_rwLen)
		{
			// Write data
			*m_ptr++ = _val;
			m_disk->updated = true;
			m_disk->writes++;
			// Decrement length
			if (--m_rwLen)
			{
				m_wait = 255; // Reset timeout watchdog
				// Advance to the next sector as needed
				if (!(m_rwLen & (m_disk->sectorLen - 1))) ++m_regs[2];
			}
			else {
				// Write completed
				m_regs[0] &= ~(F_DRQ | F_BUSY);
				m_irq = WD1793_IRQ;
			}
		}
		// Save last written value
		m_regs[(int)_reg] = _val;
		break;

	case Port::SYSTEM:
		// Reset controller if S_RESET goes up
		if ((m_regs[4] ^ _val) & _val & S_RESET){
			// TODO: figure out if it is still required.
			//Reset();
		}

		m_drive = _val & S_DRIVE;
		m_disk = m_disks[m_drive].GetDisk();

		//m_side = !(V & S_SIDE);

		// Kishinev fdc: 0011xSDD
		// 				S - side
		// 				DD - drive index: 0, 1, 2, 3
		m_side = ((~_val) >> 2) & 1; // inverted side

		// Save the last written value
		m_regs[(int)_reg] = _val;

		break;
	}

	// Done
	return(m_irq);
}

void dev::Fdc1793::Mount(const int _driveIdx, const std::vector<uint8_t>& _data, const std::wstring& _path)
{
	m_disks[_driveIdx % DRIVES_MAX].Mount(_data, _path);
	if (_driveIdx == m_drive) Reset();
}

auto dev::Fdc1793::GetFdcInfo()
-> Info
{
	return Info(
		m_drive,
		m_side,
		m_track,
		m_lastS,
		m_irq,
		m_wait,
		m_cmd,
		m_rwLen,
		m_ptr - m_disks[m_drive].GetData()
	);
}

auto dev::Fdc1793::GetFddInfo(const int _driveIdx)
-> DiskInfo
{
	return DiskInfo(m_disks[_driveIdx].path,
		m_disks[_driveIdx].updated,
		m_disks[_driveIdx].reads,
		m_disks[_driveIdx].writes,
		m_disks[_driveIdx].mounted );
}

auto dev::Fdc1793::GetFddImage(const int _driveIdx)
-> const std::vector<uint8_t>
{
	auto data = m_disks[_driveIdx].GetData();
	return { data, data + FDisk::dataLen };
}

void dev::Fdc1793::ResetUpdate(const int _driveIdx) { m_disks[_driveIdx].updated = false; }