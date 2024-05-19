// Emulation of Soviet KR1818WG93 (КР1818ВГ93) Floppy Disk Controller (WD1793 analog)

// based on:
// https://en.wikipedia.org/wiki/Western_Digital_FD1771
// https://github.com/EremusOne/ESPectrum/blob/master/src/wd1793.cpp
// https://github.com/svofski/vector06sdl
// https://github.com/teki/jstvc
// https://github.com/libretro/fmsx-libretro/blob/master/EMULib/WD1793.c

#pragma once
#ifndef DEV_FDC1793_H
#define DEV_FDC1793_H

#include <vector>


#define FDI_SAVE_FAILED    0  /* Failed saving disk image    */
#define FDI_SAVE_TRUNCATED 1  /* Truncated data while saving */
#define FDI_SAVE_PADDED    2  /* Padded data while saving    */
#define FDI_SAVE_OK        3  /* Succeeded saving disk image */

                           /* Supported disk image formats:  */
#define FMT_AUTO   0       /* Determine format automatically */                   
#define FMT_IMG    1       /* ZX Spectrum disk               */             
#define FMT_MGT    2       /* ZX Spectrum disk, same as .DSK */             
#define FMT_TRD    3       /* ZX Spectrum TRDOS disk         */
#define FMT_FDI    4       /* Generic FDI image              */ 
#define FMT_SCL    5       /* ZX Spectrum TRDOS disk         */
#define FMT_HOBETA 6       /* ZX Spectrum HoBeta disk        */
#define FMT_MSXDSK 7       /* MSX disk                       */          
#define FMT_CPCDSK 8       /* CPC disk                       */          
#define FMT_SF7000 9       /* Sega SF-7000 disk              */ 
#define FMT_SAMDSK 10      /* Sam Coupe disk                 */    
#define FMT_ADMDSK 11      /* Coleco Adam disk               */  
#define FMT_DDP    12      /* Coleco Adam tape               */  
#define FMT_SAD    13      /* Sam Coupe disk                 */
#define FMT_DSK    14      /* Generic raw disk image         */
#define FMT_MEMORY 15      /* In-memory (RetroArch SRAM)     */

#define SEEK_DELETED (0x40000000)

#define NUM_FDI_DRIVES 4

extern uint8_t* DiskData;
extern int DiskSize;

/** FDIDisk **************************************************/
/** This structure contains all disk image information and  **/
/** also the result of the last SeekFDI() call.             **/
/*************************************************************/
typedef struct
{
    uint8_t Format;     /* Original disk format (FMT_*) */
    int  Sides;      /* Sides per disk */
    int  Tracks;     /* Tracks per side */
    int  Sectors;    /* Sectors per track */
    int  SecSize;    /* Bytes per sector */

    uint8_t* Data;      /* Disk data */
    int  DataSize;   /* Disk data size */

    uint8_t Header[6];  /* Current header, result of SeekFDI() */
    uint8_t Dirty;      /* 1: Data to be flushed */
} FDIDisk;

/** InitFDI() ************************************************/
/** Clear all data structure fields.                        **/
/*************************************************************/
void InitFDI(FDIDisk* D);

/** EjectFDI() ***********************************************/
/** Eject disk image. Free all allocated memory.            **/
/*************************************************************/
void EjectFDI(FDIDisk* D);

/** SeekFDI() ************************************************/
/** Seek to given side/track/sector. Returns sector address **/
/** on success or 0 on failure.                             **/
/*************************************************************/
uint8_t* SeekFDI(FDIDisk* D, int Side, int Track, int SideID, int TrackID, int SectorID);


#define WD1793_KEEP    0
#define WD1793_INIT    1
#define WD1793_EJECT   2

#define WD1793_COMMAND 0
#define WD1793_STATUS  0
#define WD1793_TRACK   1
#define WD1793_SECTOR  2
#define WD1793_DATA    3
#define WD1793_SYSTEM  4
#define WD1793_READY   4

#define WD1793_IRQ     0x80
#define WD1793_DRQ     0x40

    /* Common status bits:               */
#define F_BUSY     0x01    /* Controller is executing a command */
#define F_READONLY 0x40    /* The disk is write-protected       */
#define F_NOTREADY 0x80    /* The drive is not ready            */

                           /* Type-1 command status:            */
#define F_INDEX    0x02    /* Index mark detected               */
#define F_TRACK0   0x04    /* Head positioned at track #0       */
#define F_CRCERR   0x08    /* CRC error in ID field             */
#define F_SEEKERR  0x10    /* Seek error, track not verified    */
#define F_HEADLOAD 0x20    /* Head loaded                       */

                           /* Type-2 and Type-3 command status: */
#define F_DRQ      0x02    /* Data request pending              */
#define F_LOSTDATA 0x04    /* Data has been lost (missed DRQ)   */
#define F_ERRCODE  0x18    /* Error code bits:                  */
#define F_BADDATA  0x08    /* 1 = bad data CRC                  */
#define F_NOTFOUND 0x10    /* 2 = sector not found              */
#define F_BADID    0x18    /* 3 = bad ID field CRC              */
#define F_DELETED  0x20    /* Deleted data mark (when reading)  */
#define F_WRFAULT  0x20    /* Write fault (when writing)        */

#define C_DELMARK  0x01
#define C_SIDECOMP 0x02
#define C_STEPRATE 0x03
#define C_VERIFY   0x04
#define C_WAIT15MS 0x04
#define C_LOADHEAD 0x08
#define C_SIDE     0x08
#define C_IRQ      0x08
#define C_SETTRACK 0x10
#define C_MULTIREC 0x10

#define S_DRIVE    0x03
#define S_RESET    0x04
#define S_HALT     0x08
#define S_SIDE     0x10
#define S_DENSITY  0x20

#pragma pack(4)
    typedef struct
    {
        int  Rsrvd1[4];   /* Reserved, do not touch */

        uint8_t R[5];        /* Registers */
        uint8_t Drive;       /* Current disk # */
        uint8_t Side;        /* Current side # */
        uint8_t Track[4];    /* Current track # */
        uint8_t LastS;       /* Last STEP direction */
        uint8_t IRQ;         /* 0x80: IRQ pending, 0x40: DRQ pending */
        uint8_t Wait;        /* Expiration counter */
        uint8_t Cmd;         /* Last command */

        int  WRLength;    /* Data left to write */
        int  RDLength;    /* Data left to read */
        int  Rsrvd2;      /* Reserved, do not touch */

        /*--- Save1793() will save state above this line ----*/

        uint8_t* Ptr;        /* Pointer to data */
        FDIDisk* Disk[NUM_FDI_DRIVES]; /* Disk images */
    } WD1793;
#pragma pack()

    /** Reset1793() **********************************************/
    /** Reset WD1793. When Disks=WD1793_INIT, also initialize   **/
    /** disks. When Disks=WD1793_EJECT, eject inserted disks,   **/
    /** freeing memory.                                         **/
    /*************************************************************/
    void Reset1793(WD1793* D, FDIDisk* Disks, uint8_t Eject);

    /** Read1793() ***********************************************/
    /** Read value from a WD1793 register A. Returns read data  **/
    /** on success or 0xFF on failure (bad register address).   **/
    /*************************************************************/
    uint8_t Read1793(WD1793* D, uint8_t A);

    /** Write1793() **********************************************/
    /** Write value V into WD1793 register A. Returns DRQ/IRQ   **/
    /** values.                                                 **/
    /*************************************************************/
    uint8_t Write1793(WD1793* D, uint8_t A, uint8_t V);

    /** Save1793() ***********************************************/
    /** Save WD1793 state to a given buffer of given maximal    **/
    /** size. Returns number of bytes saved or 0 on failure.    **/
    /*************************************************************/
    unsigned int Save1793(const WD1793* D, uint8_t* Buf, unsigned int Size);

    /** Load1793() ***********************************************/
    /** Load WD1793 state from a given buffer of given maximal  **/
    /** size. Returns number of bytes loaded or 0 on failure.   **/
    /*************************************************************/
    unsigned int Load1793(WD1793* D, uint8_t* Buf, unsigned int Size);


#endif // DEV_FDC1793_H