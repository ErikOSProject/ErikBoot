// SPDX-License-Identifier: BSD-2-Clause
// Author: Erik Inkinen <erik.inkinen@erikinkinen.fi>

#include <efi.h>
#include <efilib.h>
#include <elf.h>

#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04

typedef struct {
	uint16_t Magic;
	uint8_t Mode;
	uint8_t CharSize;
} PSF1Header;

typedef struct {
	PSF1Header PSF1Header;
	void* GlyphBuffer;
} PSF1Font;

typedef struct {
	void* BaseAddress;
	size_t BufferSize;
	unsigned int Width;
	unsigned int Height;
	unsigned int PixelsPerScanLine;
} Framebuffer;

typedef struct {
	EFI_MEMORY_DESCRIPTOR *Base;
	UINTN DescCount;
	UINTN DescSize;
} MemMapPtr;

struct {
	Framebuffer FrameBuffer;
	MemMapPtr MemoryMap;
	PSF1Font Font;
	void *Initrd;
	unsigned long long InitrdSize;
} BootInfo;

int memcmp(const void *a, const void* b, size_t n) {
	for (size_t i = 0; i < n; ++i)
		if (((char*)a)[i] < ((char*)b)[i]) return -1;
		else if (((char*)a)[i] > ((char*)b)[i]) return 1;
	return 0;
}

EFI_FILE_INFO *GetFileInfo(EFI_SYSTEM_TABLE *SystemTable, EFI_FILE *File) {
	if (!File) return NULL;

	EFI_FILE_INFO *FileInfo = NULL;
	UINTN FileInfoSize = 0;
	EFI_GUID FileInfoGuid = EFI_FILE_INFO_ID;

	EFI_STATUS Status = File->GetInfo(File, &FileInfoGuid,
		&FileInfoSize, NULL);

	if (Status == EFI_BUFFER_TOO_SMALL) {
		SystemTable->BootServices->AllocatePool(EfiLoaderData, 
			FileInfoSize, (VOID**)&FileInfo);
		if (FileInfo) {
			Status = File->GetInfo(File, &FileInfoGuid,
				&FileInfoSize, FileInfo);
			if (EFI_ERROR(Status)) {
				SystemTable->BootServices->FreePool(FileInfo);
				FileInfo = NULL;
			}
		}
	}

	return FileInfo;
}

EFI_MEMORY_DESCRIPTOR *GetMemoryMap(EFI_SYSTEM_TABLE *SystemTable, 
	UINTN *EntryCount, UINTN *MapKey, 
	UINTN *DescriptorSize, UINT32 *DescriptorVersion) {
	EFI_STATUS Status = EFI_BUFFER_TOO_SMALL;
	EFI_MEMORY_DESCRIPTOR *Buffer = NULL;
	UINTN BufferSize = sizeof(EFI_MEMORY_DESCRIPTOR);

	while (Status == EFI_BUFFER_TOO_SMALL) {
		if (Buffer) SystemTable->BootServices->FreePool(Buffer);
		SystemTable->BootServices->AllocatePool(EfiLoaderData, BufferSize, (VOID**)&Buffer);
		Status = SystemTable->BootServices->GetMemoryMap(&BufferSize, Buffer, MapKey, DescriptorSize, DescriptorVersion);
	}

	if (EFI_ERROR(Status)) Buffer = NULL;
	else {
		*EntryCount = BufferSize / *DescriptorSize;
	}

	return Buffer;
}

EFI_STATUS FindGOP(EFI_SYSTEM_TABLE *SystemTable) {
	EFI_GUID GOPGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;

	EFI_STATUS Status = SystemTable->BootServices->LocateProtocol(&GOPGuid, 
		NULL, (void **)&GOP);
	if (EFI_ERROR(Status)) return Status;
	
	BootInfo.FrameBuffer.BaseAddress = (void*)GOP->Mode->FrameBufferBase;
	BootInfo.FrameBuffer.BufferSize = GOP->Mode->FrameBufferSize;
	BootInfo.FrameBuffer.Width = GOP->Mode->Info->HorizontalResolution;
	BootInfo.FrameBuffer.Height = GOP->Mode->Info->VerticalResolution;
	BootInfo.FrameBuffer.PixelsPerScanLine = GOP->Mode->Info->PixelsPerScanLine;

	return EFI_SUCCESS;
}

EFI_FILE *OpenFile(EFI_FILE *Directory, CHAR16 *Path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	EFI_FILE *OpenedFile;

	EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
	EFI_GUID LoadedImageProtocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	SystemTable->BootServices->HandleProtocol(ImageHandle, 
		&LoadedImageProtocol, (void **)&LoadedImage);

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FS;
	EFI_GUID FSProtocol = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	SystemTable->BootServices->HandleProtocol(LoadedImage->DeviceHandle, 
		&FSProtocol, (void **)&FS);

	if (Directory == NULL)
		FS->OpenVolume(FS, &Directory);
	EFI_STATUS Status = Directory->Open(Directory, 
		&OpenedFile, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if (Status != EFI_SUCCESS) return NULL;
	return OpenedFile;
}

void *LoadKernel(EFI_FILE *Directory, CHAR16 *Path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	EFI_FILE *KernelFile = OpenFile(Directory, Path, ImageHandle, SystemTable);
	if (KernelFile == NULL) return NULL;

	Elf64_Ehdr Header;
	UINTN HeaderSize = sizeof(Elf64_Ehdr);
	KernelFile->Read(KernelFile, &HeaderSize, &Header);

	if (
		memcmp(&Header.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
		Header.e_ident[EI_CLASS] != ELFCLASS64 ||
		Header.e_type != ET_EXEC ||
		Header.e_version != EV_CURRENT
	) return NULL;

	Elf64_Phdr *PHeaderList;
	KernelFile->SetPosition(KernelFile, Header.e_phoff);
	UINTN PHeaderSize = Header.e_phnum * Header.e_phentsize;
	SystemTable->BootServices->AllocatePool(EfiLoaderData, PHeaderSize, (void **)&PHeaderList);
	KernelFile->Read(KernelFile, &PHeaderSize, PHeaderList);

	for (
		Elf64_Phdr* PHeader = PHeaderList; 
		(char *)PHeader < (char *)PHeaderList + Header.e_phnum * Header.e_phentsize;
		PHeader = (Elf64_Phdr *)((char *)PHeader + Header.e_phentsize)
	) {
		switch (PHeader->p_type) {
			case PT_LOAD: {
				int Pages = (PHeader->p_memsz + 0xFFF) / 0x1000;
				Elf64_Addr Segment = PHeader->p_paddr;
				SystemTable->BootServices->AllocatePages(AllocateAddress, 
					EfiLoaderData, Pages, &Segment);
				KernelFile->SetPosition(KernelFile, PHeader->p_offset);
				UINTN PHeaderSize = PHeader->p_filesz;
				KernelFile->Read(KernelFile, &PHeaderSize, (void *)Segment);
			} break;
		}
	}

	return (void *)Header.e_entry;
}

EFI_STATUS LoadInitrd(EFI_FILE *Directory, CHAR16 *Path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	EFI_FILE *InitrdFile = OpenFile(Directory, Path, ImageHandle, SystemTable);
	BootInfo.Initrd = NULL;
	if (InitrdFile == NULL) return EFI_LOAD_ERROR;

	EFI_FILE_INFO *InitrdFileInfo = GetFileInfo(SystemTable, InitrdFile);
	BootInfo.InitrdSize = InitrdFileInfo->FileSize;
	SystemTable->BootServices->AllocatePool(EfiLoaderData, InitrdFileInfo->FileSize, (void **)&(BootInfo.Initrd));
	InitrdFile->Read(InitrdFile, &InitrdFileInfo->FileSize, BootInfo.Initrd);
	SystemTable->BootServices->FreePool(InitrdFileInfo);

	return EFI_SUCCESS;
}

EFI_STATUS LoadPSF1Font(EFI_FILE *Directory, CHAR16 *Path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_FILE* FontFile = OpenFile(Directory, Path, ImageHandle, SystemTable);
	if (FontFile == NULL) return EFI_LOAD_ERROR;

	UINTN HeaderSize = sizeof(PSF1Header);
	FontFile->Read(FontFile, &HeaderSize, &BootInfo.Font.PSF1Header);
	if (BootInfo.Font.PSF1Header.Magic != ((uint16_t)PSF1_MAGIC0 | ((uint16_t)PSF1_MAGIC1 << 8)))
		return EFI_LOAD_ERROR;

	UINTN GlyphBufferSize = BootInfo.Font.PSF1Header.CharSize * 256;
	if (BootInfo.Font.PSF1Header.Mode == 1)
		GlyphBufferSize = BootInfo.Font.PSF1Header.CharSize * 512;

	FontFile->SetPosition(FontFile, sizeof(PSF1Header));
	SystemTable->BootServices->AllocatePool(EfiLoaderData, 
		GlyphBufferSize, (void**)&(BootInfo.Font.GlyphBuffer));
	FontFile->Read(FontFile, &GlyphBufferSize, BootInfo.Font.GlyphBuffer);

	return EFI_SUCCESS;
}

[[noreturn]]
EFI_STATUS efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	void *KernelEntry = LoadKernel(NULL, L"KERNEL.ERIK", ImageHandle, SystemTable);
	if (KernelEntry == NULL) return EFI_LOAD_ERROR;

	if (EFI_ERROR(LoadInitrd(NULL, L"INITRD.TAR", ImageHandle, SystemTable)))
		return EFI_LOAD_ERROR;

	if (EFI_ERROR(FindGOP(SystemTable)))
		return EFI_LOAD_ERROR;

	LoadPSF1Font(NULL, L"FONT.PSF", ImageHandle, SystemTable);

	UINTN MapKey = 0;
	UINT32 descVersion = 0;
	BootInfo.MemoryMap.Base = GetMemoryMap(SystemTable, 
		&(BootInfo.MemoryMap.DescCount), &MapKey, &(BootInfo.MemoryMap.DescSize), &descVersion);

	SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey);
	asm volatile ("jmp *%0":: "m"(KernelEntry), "D"(&BootInfo));
	__builtin_unreachable();
}
