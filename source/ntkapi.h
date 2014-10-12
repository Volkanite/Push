#ifndef _INC_IMDISK_
#define _INC_IMDISK_

#ifndef __T
#if defined(_NTDDK_) || defined(UNICODE) || defined(_UNICODE)
#define __T(x)  L ## x
#else
#define __T(x)  x
#endif
#endif

#ifndef _T
#define _T(x)   __T(x)
#endif




#pragma pack(push)
#pragma pack(1)
typedef struct _FAT_BPB
{
  USHORT BytesPerSector;
  UCHAR  SectorsPerCluster;
  USHORT ReservedSectors;
  UCHAR  NumberOfFileAllocationTables;
  USHORT NumberOfRootEntries;
  USHORT NumberOfSectors;
  UCHAR  MediaDescriptor;
  USHORT SectorsPerFileAllocationTable;
  USHORT SectorsPerTrack;
  USHORT NumberOfHeads;
  union
  {
    struct
    {
      USHORT NumberOfHiddenSectors;
      USHORT TotalNumberOfSectors;
    } DOS_320;
    struct
    {
      ULONG  NumberOfHiddenSectors;
      ULONG  TotalNumberOfSectors;
    } DOS_331;
  };
} FAT_BPB, *PFAT_BPB;

typedef struct _FAT_VBR
{
  UCHAR   JumpInstruction[3];
  CHAR    OEMName[8];
  FAT_BPB BPB;
  UCHAR   FillData[512-3-8-sizeof(FAT_BPB)-1-2];
  UCHAR   PhysicalDriveNumber;
  UCHAR   Signature[2];
} FAT_VBR, *PFAT_VBR;


typedef struct _PROCESS_MONITOR_INPUT
{
    unsigned char Activate;
    unsigned long FunctionAddress;

}   PROCESS_MONITOR_INPUT;









#pragma pack(pop)


#endif // _INC_IMDISK_

#ifndef __PHYMEM_H
#define __PHYMEM_H
//#include <Windows.h>
//-----------------------------------------------------------------------------
//
// The Device type codes form 32768 to 65535 are for customer use.
//
//-----------------------------------------------------------------------------

#define OLS_TYPE 40000
//-----------------------------------------------------------------------------
//
// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
//
//-----------------------------------------------------------------------------




typedef LARGE_INTEGER PHYSICAL_ADDRESS;

/*typedef struct tagPHYMEM_MEM
{
    //PVOID pvAddr; //physical addr when mapping, virtual addr when unmapping
    PHYSICAL_ADDRESS pvAddr;
    ULONG dwSize;   //memory size to map or unmap
} PHYMEM_MEM, *PPHYMEM_MEM;*/


typedef struct  _OLS_READ_MEMORY_INPUT {
    PHYSICAL_ADDRESS Address;
    ULONG UnitSize;
    ULONG Count;
}   OLS_READ_MEMORY_INPUT;

#endif  //__PHYMEM_H
