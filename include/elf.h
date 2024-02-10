#ifndef _ELF_H
#define _ELF_H

#define ELF_MAGIC "\177ELF"
#define ELF_EXECUTABLE 2
#define ELF_P_LOAD 1

typedef struct {
	UINT8 Magic[4];
	UINT8 Type;
	UINT8 Endianness;
	UINT8 Version;
	UINT8 ABI;
	UINT8 Unused[8];
} ELF_IDENTIFICATION;

typedef struct {
	ELF_IDENTIFICATION Id;
	UINT16 Type;
	UINT16 Machine;
	UINT32 Version;
	UINTN Entry;
	UINTN PHeaderBase;
	UINTN SHeaderBase;
	UINT32 Flags;
	UINT16 HeaderSize;
	UINT16 PHeaderEntrySize;
	UINT16 PHeaderEntryCount;
	UINT16 SHeaderEntrySize;
	UINT16 SHeaderEntryCount;
	UINT16 SHeaderIndex;
} ELF_HEADER;

typedef struct {
	UINT32 Type;
	UINT32 Flags;
	UINTN Offset;
	UINTN VirtualStart;
	UINTN Unused;
	UINTN FileSize;
	UINTN MemorySize;
} ELF_PROGRAM_HEADER;

EFI_STATUS LoadElf(EFI_FILE *File, UINT8 **Image, UINTN *ImageLength,
		   UINTN *VirtualAddress, UINTN *Entry);

#endif //_ELF_H
