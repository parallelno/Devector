#include "Fdcc1793.h"
#include "Utils/Utils.h"
#include <cstdint>



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



/** EjectFDI() ***********************************************/
/** Eject disk image. Free all allocated memory.            **/
/*************************************************************/
void dev::Fdc1793::EjectFDI(FDIDisk* D)
{
	if (D->data) free(D->data);
	InitFDI(D);
}

/** InitFDI() ************************************************/
/** Clear all data structure fields.                        **/
/*************************************************************/
void dev::Fdc1793::InitFDI(FDIDisk* D)
{
	D->format = FDIDisk::Format::VECTOR;
	D->data = 0;
	D->dataLen = 0;
	D->sidesPerDisk = 0;
	D->tracksPerSide = 0;
	D->sectorsPerTrack = 0;
	D->sectorLen = 0;
	D->updated = false;
}

static const int SecSizes[] =
{ 128,256,512,1024,4096,0 };

#define FDI_DATA(P)       ((P)+(P)[10]+((int)((P)[11])<<8))
#define FDI_TRACKS(P)     ((P)[4]+((int)((P)[5])<<8))
#define FDI_DIR(P)        ((P)+(P)[12]+((int)((P)[13])<<8)+14)
#define FDI_SECTORS(T)    ((T)[6])
#define FDI_SECSIZE(S)    (SecSizes[(S)[3]<=4? (S)[3]:4])
#define FDI_TRACK(P,T)    (FDI_DATA(P)+(T)[0]+((int)((T)[1])<<8)+((int)((T)[2])<<16)+((int)((T)[3])<<24))
#define FDI_SECTOR(P,T,S) (FDI_TRACK(P,T)+(S)[5]+((int)((S)[6])<<8))

/** SeekFDI() ************************************************/
/** Seek to given side/track/sector. Returns sector address **/
/** on success or 0 on failure.                             **/
/*************************************************************/
uint8_t* dev::Fdc1793::SeekFDI(FDIDisk* D, int Side, int Track, int SideID, int TrackID, int SectorID)
{
	/* Have to have disk mounted */
	if (!D || !D->data) return(0);


	switch (D->format)
	{
	case FDIDisk::Format::VECTOR:
	{
		int sectors = FDIDisk::VECTOR_SECTORS_PER_TRACK * (TrackID * FDIDisk::VECTOR_SIDES_PER_DISK + SideID);
		int sectorAdjusted = dev::Max(0, SectorID - 1); // In CHS addressing the sector numbers always start at 1
		int m_position = (sectors + sectorAdjusted) * FDIDisk::VECTOR_SECTOR_LEN;

		uint8_t* P = D->data + m_position;

		/* FDI stores a header for each sector */
		D->header[0] = TrackID;
		D->header[1] = SideID;
		D->header[2] = SectorID;
		D->header[3] = 0x3; // sec len 1024
		D->header[4] = 0x00;
		D->header[5] = 0x00;
		/* FDI has variable sector numbers and sizes */
		D->sectorsPerTrack = FDIDisk::VECTOR_SECTORS_PER_TRACK; // ??? maybe it expects a curent sector?
		D->sectorLen = FDIDisk::VECTOR_SECTOR_LEN;
		return D->data + m_position;
	}
	case FDIDisk::Format::TRD:
	case FDIDisk::Format::DSK:
	case FDIDisk::Format::SCL:
	case FDIDisk::Format::FDI:
	case FDIDisk::Format::MGT:
	case FDIDisk::Format::IMG:
	case FDIDisk::Format::DDP:
	case FDIDisk::Format::SAD:
	case FDIDisk::Format::CPCDSK:
	case FDIDisk::Format::SAMDSK:
	case FDIDisk::Format::ADMDSK:
	case FDIDisk::Format::MSXDSK:
	case FDIDisk::Format::SF7000:
		{
			//uint8_t* P, * T;
			//int J;

			///* May need to search for deleted sectors */
			//int Deleted = (SectorID >= 0) && (SectorID & SEEK_DELETED) ? 0x80 : 0x00;
			//if (Deleted) SectorID &= ~SEEK_DELETED;

			///* Track directory */
			//P = FDI_DIR(D->Data);
			///* Find current track entry */
			//for (J = Track * D->Sides + Side % D->Sides;J;--J) P += (FDI_SECTORS(P) + 1) * 7;
			///* Find sector entry */
			//for (J = FDI_SECTORS(P), T = P + 7;J;--J, T += 7)
			//	if ((T[0] == TrackID) || (TrackID < 0))
			//		if ((T[1] == SideID) || (SideID < 0))
			//			if (((T[2] == SectorID) && ((T[4] & 0x80) == Deleted)) || (SectorID < 0))
			//				break;
			///* Fall out if not found */
			//if (!J) return(0);
			///* FDI stores a header for each sector */
			//D->Header[0] = T[0];
			//D->Header[1] = T[1];
			//D->Header[2] = T[2];
			//D->Header[3] = T[3] <= 3 ? T[3] : 3;
			//D->Header[4] = T[4];
			//D->Header[5] = 0x00;
			///* FDI has variable sector numbers and sizes */
			//D->Sectors = FDI_SECTORS(P);
			//D->SecSize = FDI_SECSIZE(T);
			//return(FDI_SECTOR(D->Data, P, T));
		}
	}

	/* Unknown format */
	return(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Reset1793() **********************************************/
/** Reset WD1793. When Disks=WD1793_INIT, also initialize   **/
/** disks. When Disks=WD1793_EJECT, eject inserted disks,   **/
/** freeing memory.                                         **/
/*************************************************************/
void dev::Fdc1793::Reset1793(WD1793* D, FDIDisk* Disks, uint8_t Eject)
{
	int J;

	D->R[0] = 0x00;
	D->R[1] = 0x00;
	D->R[2] = 0x00;
	D->R[3] = 0x00;
	D->R[4] = S_RESET | S_HALT;
	D->Drive = 0;
	D->Side = 0;
	D->LastS = 0;
	D->IRQ = 0;
	D->WRLength = 0;
	D->RDLength = 0;
	D->Wait = 0;
	D->Cmd = 0xD0;
	D->Rsrvd2 = 0;

	/* For all drives... */
	for (J = 0;J < DRIVES_MAX;++J)
	{
		/* Reset drive-dependent state */
		D->Disk[J] = Disks ? &Disks[J] : 0;
		D->Track[J] = 0;
		D->Rsrvd1[J] = 0;
		/* Initialize disk structure, if requested */
		if ((Eject == WD1793_INIT) && D->Disk[J])  InitFDI(D->Disk[J]);
		/* Eject disk image, if requested */
		if ((Eject == WD1793_EJECT) && D->Disk[J]) EjectFDI(D->Disk[J]);
	}
}

/** Read1793() ***********************************************/
/** Read value from a WD1793 register A. Returns read data  **/
/** on success or 0xFF on failure (bad register address).   **/
/*************************************************************/
uint8_t dev::Fdc1793::Read1793(WD1793* _driveP, Port _port)
{
	switch (_port)
	{
	case Port::STATUS:
		_port = static_cast<Port>(_driveP->R[0]);
		/* If no disk present, set F_NOTREADY */
		if (!_driveP->Disk[_driveP->Drive] || !_driveP->Disk[_driveP->Drive]->data) _port = static_cast<Port>((int)_port | F_NOTREADY);
		if ((_driveP->Cmd < 0x80) || (_driveP->Cmd == 0xD0))
		{
			/* Keep flipping F_INDEX bit as the disk rotates (Sam Coupe) */
			_driveP->R[0] = (_driveP->R[0] ^ F_INDEX) & (F_INDEX | F_BUSY | F_NOTREADY | F_READONLY | F_TRACK0);
		}
		else
		{
			/* When reading status, clear all bits but F_BUSY and F_NOTREADY */
			_driveP->R[0] &= F_BUSY | F_NOTREADY | F_READONLY | F_DRQ;
		}
		return((int)_port);
	case Port::TRACK:
	case Port::SECTOR:
		/* Return track/sector numbers */
		return(_driveP->R[(int)_port]);
	case Port::DATA:
		/* When reading data, load value from disk */
		if (_driveP->RDLength)
		{
			/* Read data */
			_driveP->R[(int)_port] = *_driveP->Ptr++;
			/* Decrement length */
			if (--_driveP->RDLength)
			{
				/* Reset timeout watchdog */
				_driveP->Wait = 255;
				/* Advance to the next sector as needed */
				if (!(_driveP->RDLength & (_driveP->Disk[_driveP->Drive]->sectorLen - 1))) ++_driveP->R[2];
			}
			else
			{
				/* Read completed */
				_driveP->R[0] &= ~(F_DRQ | F_BUSY);
				_driveP->IRQ = WD1793_IRQ;
			}
		}
		return(_driveP->R[(int)_port]);
	case Port::READY:
		/* After some idling, stop read/write operations */
		if (_driveP->Wait)
			if (!--_driveP->Wait)
			{
				_driveP->RDLength = _driveP->WRLength = 0;
				_driveP->R[0] = (_driveP->R[0] & ~(F_DRQ | F_BUSY)) | F_LOSTDATA;
				_driveP->IRQ = WD1793_IRQ;
			}
		/* Done */
		return(_driveP->IRQ);
	}

	/* Bad register */
	return(0xFF);
}

/** Write1793() **********************************************/
/** Write value V into WD1793 register A. Returns DRQ/IRQ   **/
/** values.                                                 **/
/*************************************************************/
uint8_t dev::Fdc1793::Write1793(WD1793* D, Port A, uint8_t V)
{
	int J;

	switch (A)
	{
	case Port::COMMAND:
		/* Reset interrupt request */
		D->IRQ = 0;
		/* If it is FORCE-IRQ command... */
		if ((V & 0xF0) == 0xD0)
		{
			/* Reset any executing command */
			D->RDLength = D->WRLength = 0;
			D->Cmd = 0xD0;
			/* Either reset BUSY flag or reset all flags if BUSY=0 */
			if (D->R[0] & F_BUSY) D->R[0] &= ~F_BUSY;
			else               D->R[0] = (D->Track[D->Drive] ? 0 : F_TRACK0) | F_INDEX;
			/* Cause immediate interrupt if requested */
			if (V & C_IRQ) D->IRQ = WD1793_IRQ;
			/* Done */
			return(D->IRQ);
		}
		/* If busy, drop out */
		if (D->R[0] & F_BUSY) break;
		/* Reset status register */
		D->R[0] = 0x00;
		D->Cmd = V;
		/* Depending on the command... */
		switch (V & 0xF0)
		{
		case 0x00: /* RESTORE (seek track 0) */
			D->Track[D->Drive] = 0;
			D->R[0] = F_INDEX | F_TRACK0 | (V & C_LOADHEAD ? F_HEADLOAD : 0);
			D->R[1] = 0;
			D->IRQ = WD1793_IRQ;
			break;

		case 0x10: /* SEEK */
			/* Reset any executing command */
			D->RDLength = D->WRLength = 0;
			D->Track[D->Drive] = D->R[3];
			D->R[0] = F_INDEX
				| (D->Track[D->Drive] ? 0 : F_TRACK0)
				| (V & C_LOADHEAD ? F_HEADLOAD : 0);
			D->R[1] = D->Track[D->Drive];
			D->IRQ = WD1793_IRQ;
			break;

		case 0x20: /* STEP */
		case 0x30: /* STEP-AND-UPDATE */
		case 0x40: /* STEP-IN */
		case 0x50: /* STEP-IN-AND-UPDATE */
		case 0x60: /* STEP-OUT */
		case 0x70: /* STEP-OUT-AND-UPDATE */
			/* Either store or fetch step direction */
			if (V & 0x40) D->LastS = V & 0x20; else V = (V & ~0x20) | D->LastS;
			/* Step the head, update track register if requested */
			if (V & 0x20) { if (D->Track[D->Drive]) --D->Track[D->Drive]; }
			else ++D->Track[D->Drive];
			/* Update track register if requested */
			if (V & C_SETTRACK) D->R[1] = D->Track[D->Drive];
			/* Update status register */
			D->R[0] = F_INDEX | (D->Track[D->Drive] ? 0 : F_TRACK0);
			// @@@ WHY USING J HERE?
			//                  | (J&&(V&C_VERIFY)? 0:F_SEEKERR);
					  /* Generate IRQ */
			D->IRQ = WD1793_IRQ;
			break;

		case 0x80:
		case 0x90: /* READ-SECTORS */
			/* Seek to the requested sector */
			D->Ptr = SeekFDI(
				D->Disk[D->Drive], D->Side, D->Track[D->Drive],
				V & C_SIDECOMP ? !!(V & C_SIDE) : D->Side, D->R[1], D->R[2]
			);
			/* If seek successful, set up reading operation */
			if (!D->Ptr)
			{
				D->R[0] = (D->R[0] & ~F_ERRCODE) | F_NOTFOUND;
				D->IRQ = WD1793_IRQ;
			}
			else
			{
				D->RDLength = D->Disk[D->Drive]->sectorLen
					* (V & 0x10 ? (D->Disk[D->Drive]->sectorsPerTrack - D->R[2] + 1) : 1);
				D->R[0] |= F_BUSY | F_DRQ;
				D->IRQ = WD1793_DRQ;
				D->Wait = 255;
			}
			break;

		case 0xA0:
		case 0xB0: /* WRITE-SECTORS */
			/* Seek to the requested sector */
			D->Ptr = SeekFDI(
				D->Disk[D->Drive], D->Side, D->Track[D->Drive],
				V & C_SIDECOMP ? !!(V & C_SIDE) : D->Side, D->R[1], D->R[2]
			);
			/* If seek successful, set up writing operation */
			if (!D->Ptr)
			{
				D->R[0] = (D->R[0] & ~F_ERRCODE) | F_NOTFOUND;
				D->IRQ = WD1793_IRQ;
			}
			else
			{
				D->WRLength = D->Disk[D->Drive]->sectorLen
					* (V & 0x10 ? (D->Disk[D->Drive]->sectorsPerTrack - D->R[2] + 1) : 1);
				D->R[0] |= F_BUSY | F_DRQ;
				D->IRQ = WD1793_DRQ;
				D->Wait = 255;
				D->Disk[D->Drive]->updated = true;
			}
			break;

		case 0xC0: /* READ-ADDRESS */
			/* Read first sector address from the track */
			if (!D->Disk[D->Drive]) D->Ptr = 0;
			else
				for (J = 0;J < 256;++J)
				{
					D->Ptr = SeekFDI(
						D->Disk[D->Drive],
						D->Side, D->Track[D->Drive],
						D->Side, D->Track[D->Drive], J
					);
					if (D->Ptr) break;
				}
			/* If address found, initiate data transfer */
			if (!D->Ptr)
			{
				D->R[0] |= F_NOTFOUND;
				D->IRQ = WD1793_IRQ;
			}
			else
			{
				D->Ptr = D->Disk[D->Drive]->header;
				D->RDLength = 6;
				D->R[0] |= F_BUSY | F_DRQ;
				D->IRQ = WD1793_DRQ;
				D->Wait = 255;
			}
			break;

		case 0xE0: /* READ-TRACK */
			break;

		case 0xF0: /* WRITE-TRACK, i.e., format */
			// not implementing the full protocol (involves parsing lead-in & lead-out); simply setting track data to 0xe5
			if (D->Ptr = SeekFDI(D->Disk[D->Drive], 0, D->Track[D->Drive], 0, D->R[1], 1))
			{
				memset(D->Ptr, 0xe5, D->Disk[D->Drive]->sectorLen * D->Disk[D->Drive]->sectorsPerTrack);
				D->Disk[D->Drive]->updated = 1;
			}
			if (D->Disk[D->Drive]->sidesPerDisk > 1 && (D->Ptr = SeekFDI(D->Disk[D->Drive], 1, D->Track[D->Drive], 1, D->R[1], 1)))
			{
				memset(D->Ptr, 0xe5, D->Disk[D->Drive]->sectorLen * D->Disk[D->Drive]->sectorsPerTrack);
				D->Disk[D->Drive]->updated = 1;
			}
			break;

		default: /* UNKNOWN */
			break;
		}
		break;

	case Port::TRACK:
	case Port::SECTOR:
		if (!(D->R[0] & F_BUSY)) D->R[(int)A] = V;
		break;

	case Port::DATA:
		/* When writing data, store value to disk */
		if (D->WRLength)
		{
			/* Write data */
			*D->Ptr++ = V;
			D->Disk[D->Drive]->updated = 1;
			/* Decrement length */
			if (--D->WRLength)
			{
				/* Reset timeout watchdog */
				D->Wait = 255;
				/* Advance to the next sector as needed */
				if (!(D->WRLength & (D->Disk[D->Drive]->sectorLen - 1))) ++D->R[2];
			}
			else
			{
				/* Write completed */
				D->R[0] &= ~(F_DRQ | F_BUSY);
				D->IRQ = WD1793_IRQ;
			}
		}
		/* Save last written value */
		D->R[(int)A] = V;
		break;

	case Port::SYSTEM:
		// Reset controller if S_RESET goes up
		//if ((D->R[4] ^ V) & V & S_RESET) Reset1793(D, D->Disk[0], WD1793_KEEP);

		// Set disk #, side #, ignore the density (@@@)
		D->Drive = V & S_DRIVE;

		//D->Side = !(V & S_SIDE);
		// Kishinev fdc: 0011xSAB
		// 				A - drive A
		// 				B - drive B
		// 				S - side
		D->Side = ((~V) >> 2) & 1; // inverted side


		// Save last written value
		D->R[4] = V;

		break;
	}

	/* Done */
	return(D->IRQ);
}

//////////////////////////////////////////////////////////////////////////
//
// class interface
//
//////////////////////////////////////////////////////////////////////////


dev::FDIDisk::FDIDisk(const Format _format)
	: header()
{
	switch (_format)
	{
	case Format::VECTOR:
	{
		format = _format;
		sidesPerDisk = VECTOR_SIDES_PER_DISK;
		tracksPerSide = VECTOR_TRACKS_PER_SIDE;
		sectorsPerTrack = VECTOR_SECTORS_PER_TRACK;
		sectorLen = VECTOR_SECTOR_LEN;
		break;
	}
	case Format::TRD:
	case Format::DSK:
	case Format::SCL:
	case Format::FDI:
	case Format::MGT:
	case Format::IMG:
	case Format::DDP:
	case Format::SAD:
	case Format::CPCDSK:
	case Format::SAMDSK:
	case Format::ADMDSK:
	case Format::MSXDSK:
	case Format::SF7000:
	defualt:
		break;
	}
}

void dev::Fdc1793::Attach(const std::wstring& _path)
{
	auto res = dev::LoadFile(_path);
	auto data = *res;
	if (data.size() > sizeof(m_data)) return;

	memcpy(m_data, data.data(), data.size());

	for (int i = 0; i < DRIVES_MAX; ++i)
	{
		m_disks[i].format = FDIDisk::Format::VECTOR;
	}

	Reset1793(&fdd, m_disks, WD1793_INIT);
	fdd.Disk[0]->data = m_data;
}

auto dev::Fdc1793::Read(const Port _port)
-> uint8_t
{
	return Read1793(&fdd, _port);
}
void dev::Fdc1793::Write(const Port _port, const uint8_t _val)
{
	Write1793(&fdd, _port, _val);
}
