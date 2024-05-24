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
	header[4] = 0;		// CRC1, not supported
	header[5] = 0;		// CRC2, not supported
}

void dev::FDisk::Attach(const std::wstring& _path)
{
	auto res = dev::LoadFile(_path);
	if (!res) return;

	auto resData = *res;
	if (resData.size() > sizeof(data)) return;

	memcpy(data, resData.data(), resData.size());
	loaded = true;
}

auto dev::FDisk::GetData() 
-> uint8_t* 
{ return loaded ? data : nullptr; };

auto dev::FDisk::GetDisk() 
-> FDisk*
{ return loaded ? this : nullptr; };

//static constexpr int FDI_SAVE_FAILED    0;  /* Failed saving disk image    */
//static constexpr int FDI_SAVE_TRUNCATED 1;  /* Truncated data while saving */
//static constexpr int FDI_SAVE_PADDED    2;  /* Padded data while saving    */
//static constexpr int FDI_SAVE_OK        3;  /* Succeeded saving disk image */

static constexpr int SEEK_DELETED	= 0x40000000;

static constexpr int WD1793_KEEP	= 0;
static constexpr int WD1793_INIT	= 1;
static constexpr int WD1793_EJECT	= 2;

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
static constexpr int F_CRCERR	= 0x08; // CRC error in ID field             
static constexpr int F_SEEKERR	= 0x10; // Seek error, track not verified    
static constexpr int F_HEADLOAD = 0x20; // Head loaded                       

// Type-2 and Type-3 command status:
static constexpr int F_DRQ		= 0x02; // Data request pending              
static constexpr int F_LOSTDATA	= 0x04; // Data has been lost (missed DRQ)
static constexpr int F_ERRCODE	= 0x18; // Error code bits:               
static constexpr int F_BADDATA	= 0x08; // 1 = bad data CRC               
static constexpr int F_NOTFOUND	= 0x10; // 2 = sector not found           
static constexpr int F_BADID	= 0x18; // 3 = bad ID field CRC           
static constexpr int F_DELETED	= 0x20; // Deleted data mark (when reading)
static constexpr int F_WRFAULT	= 0x20; // Write fault (when writing)      

static constexpr int C_DELMARK	= 0x01;
static constexpr int C_SIDECOMP	= 0x02;
static constexpr int C_STEPRATE	= 0x03;
static constexpr int C_VERIFY	= 0x04;
static constexpr int C_WAIT15MS	= 0x04;
static constexpr int C_LOADHEAD	= 0x08;
static constexpr int C_SIDE		= 0x08;
static constexpr int C_IRQ		= 0x08;
static constexpr int C_SETTRACK	= 0x10;
static constexpr int C_MULTIREC	= 0x10;

static constexpr int S_DRIVE	= 0x03;
static constexpr int S_RESET	= 0x04;
static constexpr int S_HALT		= 0x08;
static constexpr int S_SIDE		= 0x10;
static constexpr int S_DENSITY	= 0x20;


/** Seek() ************************************************/
/** Seek to given side/track/sector. Returns sector address **/
/** on success or 0 on failure.                             **/
/*************************************************************/
uint8_t* dev::Fdc1793::Seek(int Side, int Track, int SideID, int TrackID, int SectorID)
{
	/* Have to have disk mounted */
	if (!fdd.Disk) return(0);

	int sectors = FDisk::sectorsPerTrack * (TrackID * FDisk::sidesPerDisk + SideID);
	int sectorAdjusted = dev::Max(0, SectorID - 1); // In CHS addressing the sector numbers always start at 1
	int m_position = (sectors + sectorAdjusted) * FDisk::sectorLen;

	/* FDisk stores a header for each sector */
	fdd.Disk->header[0] = TrackID;
	fdd.Disk->header[1] = SideID;
	fdd.Disk->header[2] = SectorID;
	/* FDisk has variable sector numbers and sizes */
	return fdd.Disk->GetData() + m_position;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Reset() **********************************************/
/** Reset WD1793. When Disks=WD1793_INIT, also initialize   **/
/** disks. When Disks=WD1793_EJECT, eject inserted disks,   **/
/** freeing memory.                                         **/
/*************************************************************/
void dev::Fdc1793::Reset()
{
	int J;

	fdd.R[0] = 0x00;
	fdd.R[1] = 0x00;
	fdd.R[2] = 0x00;
	fdd.R[3] = 0x00;
	fdd.R[4] = S_RESET | S_HALT;
	fdd.Drive = 0;
	fdd.Side = 0;
	fdd.LastS = 0;
	fdd.IRQ = 0;
	fdd.WRLength = 0;
	fdd.RDLength = 0;
	fdd.Wait = 0;
	fdd.Cmd = 0xD0;
	fdd.Rsrvd2 = 0;

	/* For all drives... */
	for (J = 0;J < DRIVES_MAX;++J)
	{
		/* Reset drive-dependent state */
		fdd.Track[J] = 0;
		fdd.Rsrvd1[J] = 0;
	}
}

/** Read1793() *********************************************/
/** Read value from a WD1793 register. Returns read data  **/
/** on success or 0xFF on failure (bad register address). **/
/***********************************************************/
uint8_t dev::Fdc1793::Read(Port _reg)
{
	switch (_reg)
	{
	case Port::STATUS:
		_reg = static_cast<Port>(fdd.R[0]);
		/* If no disk present, set F_NOTREADY */
		if (!fdd.Disk) _reg = static_cast<Port>((int)_reg | F_NOTREADY);
		if ((fdd.Cmd < 0x80) || (fdd.Cmd == 0xD0))
		{
			/* Keep flipping F_INDEX bit as the disk rotates (Sam Coupe) */
			fdd.R[0] = (fdd.R[0] ^ F_INDEX) & (F_INDEX | F_BUSY | F_NOTREADY | F_READONLY | F_TRACK0);
		}
		else
		{
			/* When reading status, clear all bits but F_BUSY and F_NOTREADY */
			fdd.R[0] &= F_BUSY | F_NOTREADY | F_READONLY | F_DRQ;
		}
		return((int)_reg);
	case Port::TRACK:
	case Port::SECTOR:
		/* Return track/sector numbers */
		return(fdd.R[(int)_reg]);
	case Port::DATA:
		/* When reading data, load value from disk */
		if (fdd.RDLength)
		{
			/* Read data */
			fdd.R[(int)_reg] = *fdd.Ptr++;
			/* Decrement length */
			if (--fdd.RDLength)
			{
				/* Reset timeout watchdog */
				fdd.Wait = 255;
				/* Advance to the next sector as needed */
				if (!(fdd.RDLength & (fdd.Disk->sectorLen - 1))) ++fdd.R[2];
			}
			else
			{
				/* Read completed */
				fdd.R[0] &= ~(F_DRQ | F_BUSY);
				fdd.IRQ = WD1793_IRQ;
			}
		}
		return(fdd.R[(int)_reg]);
	case Port::READY:
		/* After some idling, stop read/write operations */
		if (fdd.Wait)
			if (!--fdd.Wait)
			{
				fdd.RDLength = fdd.WRLength = 0;
				fdd.R[0] = (fdd.R[0] & ~(F_DRQ | F_BUSY)) | F_LOSTDATA;
				fdd.IRQ = WD1793_IRQ;
			}
		/* Done */
		return(fdd.IRQ);
	}

	/* Bad register */
	return(0xFF);
}

/** Write1793() ******************************************/
/** Write value into WD1793 register. Returns DRQ/IRQ   **/
/** values.                                             **/
/*********************************************************/
uint8_t dev::Fdc1793::Write(const Port _reg, uint8_t _val)
{
	int J;

	switch (_reg)
	{
	case Port::COMMAND:
		/* Reset interrupt request */
		fdd.IRQ = 0;
		/* If it is FORCE-IRQ command... */
		if ((_val & 0xF0) == 0xD0)
		{
			/* Reset any executing command */
			fdd.RDLength = fdd.WRLength = 0;
			fdd.Cmd = 0xD0;
			/* Either reset BUSY flag or reset all flags if BUSY=0 */
			if (fdd.R[0] & F_BUSY) fdd.R[0] &= ~F_BUSY;
			else               fdd.R[0] = (fdd.Track[fdd.Drive] ? 0 : F_TRACK0) | F_INDEX;
			/* Cause immediate interrupt if requested */
			if (_val & C_IRQ) fdd.IRQ = WD1793_IRQ;
			/* Done */
			return(fdd.IRQ);
		}
		/* If busy, drop out */
		if (fdd.R[0] & F_BUSY) break;
		/* Reset status register */
		fdd.R[0] = 0x00;
		fdd.Cmd = _val;
		/* Depending on the command... */
		switch (_val & 0xF0)
		{
		case 0x00: /* RESTORE (seek track 0) */
			fdd.Track[fdd.Drive] = 0;
			fdd.R[0] = F_INDEX | F_TRACK0 | (_val & C_LOADHEAD ? F_HEADLOAD : 0);
			fdd.R[1] = 0;
			fdd.IRQ = WD1793_IRQ;
			break;

		case 0x10: /* SEEK */
			/* Reset any executing command */
			fdd.RDLength = fdd.WRLength = 0;
			fdd.Track[fdd.Drive] = fdd.R[3];
			fdd.R[0] = F_INDEX
				| (fdd.Track[fdd.Drive] ? 0 : F_TRACK0)
				| (_val & C_LOADHEAD ? F_HEADLOAD : 0);
			fdd.R[1] = fdd.Track[fdd.Drive];
			fdd.IRQ = WD1793_IRQ;
			break;

		case 0x20: /* STEP */
		case 0x30: /* STEP-AND-UPDATE */
		case 0x40: /* STEP-IN */
		case 0x50: /* STEP-IN-AND-UPDATE */
		case 0x60: /* STEP-OUT */
		case 0x70: /* STEP-OUT-AND-UPDATE */
			/* Either store or fetch step direction */
			if (_val & 0x40) fdd.LastS = _val & 0x20; else _val = (_val & ~0x20) | fdd.LastS;
			/* Step the head, update track register if requested */
			if (_val & 0x20) { if (fdd.Track[fdd.Drive]) --fdd.Track[fdd.Drive]; }
			else ++fdd.Track[fdd.Drive];
			/* Update track register if requested */
			if (_val & C_SETTRACK) fdd.R[1] = fdd.Track[fdd.Drive];
			/* Update status register */
			fdd.R[0] = F_INDEX | (fdd.Track[fdd.Drive] ? 0 : F_TRACK0);
			// @@@ WHY USING J HERE?
			//                  | (J&&(V&C_VERIFY)? 0:F_SEEKERR);
					  /* Generate IRQ */
			fdd.IRQ = WD1793_IRQ;
			break;

		case 0x80:
		case 0x90: /* READ-SECTORS */
			/* Seek to the requested sector */
			fdd.Ptr = Seek(fdd.Side, fdd.Track[fdd.Drive],
				_val & C_SIDECOMP ? !!(_val & C_SIDE) : fdd.Side, fdd.R[1], fdd.R[2]
			);
			/* If seek successful, set up reading operation */
			if (!fdd.Ptr)
			{
				fdd.R[0] = (fdd.R[0] & ~F_ERRCODE) | F_NOTFOUND;
				fdd.IRQ = WD1793_IRQ;
			}
			else
			{
				fdd.RDLength = fdd.Disk->sectorLen
					* (_val & 0x10 ? (fdd.Disk->sectorsPerTrack - fdd.R[2] + 1) : 1);
				fdd.R[0] |= F_BUSY | F_DRQ;
				fdd.IRQ = WD1793_DRQ;
				fdd.Wait = 255;
			}
			break;

		case 0xA0:
		case 0xB0: /* WRITE-SECTORS */
			/* Seek to the requested sector */
			fdd.Ptr = Seek(fdd.Side, fdd.Track[fdd.Drive],
				_val & C_SIDECOMP ? !!(_val & C_SIDE) : fdd.Side, fdd.R[1], fdd.R[2]
			);
			/* If seek successful, set up writing operation */
			if (!fdd.Ptr)
			{
				fdd.R[0] = (fdd.R[0] & ~F_ERRCODE) | F_NOTFOUND;
				fdd.IRQ = WD1793_IRQ;
			}
			else
			{
				fdd.WRLength = fdd.Disk->sectorLen
					* (_val & 0x10 ? (fdd.Disk->sectorsPerTrack - fdd.R[2] + 1) : 1);
				fdd.R[0] |= F_BUSY | F_DRQ;
				fdd.IRQ = WD1793_DRQ;
				fdd.Wait = 255;
				fdd.Disk->updated = true;
			}
			break;

		case 0xC0: /* READ-ADDRESS */
			/* Read first sector address from the track */
			if (!fdd.Disk) fdd.Ptr = 0;
			else
				for (J = 0;J < 256;++J)
				{
					fdd.Ptr = Seek(
						fdd.Side, fdd.Track[fdd.Drive],
						fdd.Side, fdd.Track[fdd.Drive], J
					);
					if (fdd.Ptr) break;
				}
			/* If address found, initiate data transfer */
			if (!fdd.Ptr)
			{
				fdd.R[0] |= F_NOTFOUND;
				fdd.IRQ = WD1793_IRQ;
			}
			else
			{
				fdd.Ptr = fdd.Disk->header;
				fdd.RDLength = 6;
				fdd.R[0] |= F_BUSY | F_DRQ;
				fdd.IRQ = WD1793_DRQ;
				fdd.Wait = 255;
			}
			break;

		case 0xE0: /* READ-TRACK */
			break;

		case 0xF0: /* WRITE-TRACK, i.e., format */
			// not implementing the full protocol (involves parsing lead-in & lead-out); simply setting track data to 0xe5
			if (fdd.Ptr = Seek(0, fdd.Track[fdd.Drive], 0, fdd.R[1], 1))
			{
				memset(fdd.Ptr, 0xe5, fdd.Disk->sectorLen * fdd.Disk->sectorsPerTrack);
				fdd.Disk->updated = 1;
			}
			if (fdd.Disk->sidesPerDisk > 1 && (fdd.Ptr = Seek(1, fdd.Track[fdd.Drive], 1, fdd.R[1], 1)))
			{
				memset(fdd.Ptr, 0xe5, fdd.Disk->sectorLen * fdd.Disk->sectorsPerTrack);
				fdd.Disk->updated = 1;
			}
			break;

		default: /* UNKNOWN */
			break;
		}
		break;

	case Port::TRACK:
	case Port::SECTOR:
		if (!(fdd.R[0] & F_BUSY)) fdd.R[(int)_reg] = _val;
		break;

	case Port::DATA:
		/* When writing data, store value to disk */
		if (fdd.WRLength)
		{
			/* Write data */
			*fdd.Ptr++ = _val;
			fdd.Disk->updated = true;
			/* Decrement length */
			if (--fdd.WRLength)
			{
				/* Reset timeout watchdog */
				fdd.Wait = 255;
				/* Advance to the next sector as needed */
				if (!(fdd.WRLength & (fdd.Disk->sectorLen - 1))) ++fdd.R[2];
			}
			else
			{
				/* Write completed */
				fdd.R[0] &= ~(F_DRQ | F_BUSY);
				fdd.IRQ = WD1793_IRQ;
			}
		}
		/* Save last written value */
		fdd.R[(int)_reg] = _val;
		break;

	case Port::SYSTEM:
		// Reset controller if S_RESET goes up
		if ((fdd.R[4] ^ _val) & _val & S_RESET)
		{
			// TODO: figure out if it is still required.
			//Reset(_disk, fdd.Disk[fdd.FDisk], WD1793_KEEP);
		}

		// Set disk #, side #, ignore the density (@@@)
		fdd.Drive = _val & S_DRIVE;
		fdd.Disk = m_drives[fdd.Drive].GetDisk();

		//fdd.Side = !(V & S_SIDE);
		// Kishinev fdc: 0011xSAB
		// 				A - drive A
		// 				B - drive B
		// 				S - side
		fdd.Side = ((~_val) >> 2) & 1; // inverted side


		// Save last written value
		fdd.R[(int)_reg] = _val;

		break;
	}

	/* Done */
	return(fdd.IRQ);
}

void dev::Fdc1793::Attach(const int _driveIdx, const std::wstring& _path)
{
	m_drives[_driveIdx & DRIVES_MAX].Attach(_path);
	Reset();
}