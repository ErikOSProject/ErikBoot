#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

#include <elf.h>

extern EFI_SYSTEM_TABLE *ST;
extern EFI_HANDLE ImageHandle;

EFI_STATUS ValidateElf(ELF_HEADER *Header)
{
	UINT8 Magic[4] = ELF_MAGIC;
	for (UINT8 i = 0; i < 4; i++)
		if (Header->Id.Magic[i] != Magic[i])
			return EFI_NOT_FOUND;

	if (Header->Id.ABI != 0)
		return EFI_NOT_FOUND;

	if (Header->Type != ELF_EXECUTABLE)
		return EFI_NOT_FOUND;

	return EFI_SUCCESS;
}

EFI_STATUS LoadElf(EFI_FILE *File, UINT8 **Image, UINTN *ImageLength,
		   UINTN *VirtualAddress, UINTN *Entry)
{
	EFI_STATUS Status;
	ELF_HEADER Header;
	UINTN HeaderSize = sizeof(ELF_HEADER);

	Status = File->Read(File, &HeaderSize, &Header);
	if (EFI_ERROR(Status))
		return Status;

	Status = ValidateElf(&Header);
	if (EFI_ERROR(Status))
		return Status;

	ELF_PROGRAM_HEADER *PHeaders;
	Status = File->SetPosition(File, Header.PHeaderBase);
	if (EFI_ERROR(Status))
		return Status;

	UINTN PHeaderLoadSize =
		Header.PHeaderEntrySize * Header.PHeaderEntryCount;
	ST->BootServices->AllocatePool(EfiLoaderData, PHeaderLoadSize,
				       (void **)&PHeaders);
	Status = File->Read(File, &PHeaderLoadSize, PHeaders);
	if (EFI_ERROR(Status))
		return Status;

	ELF_PROGRAM_HEADER *PHeader = PHeaders;
	while (((UINTN)PHeader) < ((UINTN)PHeaders) + PHeaderLoadSize) {
		if (PHeader->Type == ELF_P_LOAD) {
			*ImageLength = (PHeader->MemorySize + 0xFFF) / 0x1000;
			ST->BootServices->AllocatePages(
				AllocateAnyPages, EfiLoaderData, *ImageLength,
				(EFI_PHYSICAL_ADDRESS *)Image);

			Status = File->SetPosition(File, PHeader->Offset);
			if (EFI_ERROR(Status))
				return Status;

			UINTN LoadSize = PHeader->FileSize;
			Status = File->Read(File, &LoadSize, (void *)*Image);
			if (EFI_ERROR(Status))
				return Status;

			*VirtualAddress = PHeader->VirtualStart;
			*Entry = Header.Entry;
			break;
		}

		PHeader = (ELF_PROGRAM_HEADER *)(((UINTN)PHeader) +
						 Header.PHeaderEntrySize);
	}

	return Status;
}
