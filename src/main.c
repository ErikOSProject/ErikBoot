#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#include <elf.h>
#include <erikboot.h>
#include <file.h>

EFI_SYSTEM_TABLE *ST;
EFI_HANDLE ImageHandle;
BootInfo BootData = { 0 };

void *memcpy(void *destination, const void *source, size_t num)
{
	for (size_t i = 0; i < num; i++)
		((uint8_t *)destination)[i] = ((uint8_t *)source)[i];
	return destination;
}

EFI_STATUS FindGOP(void)
{
	EFI_STATUS Status;

	EFI_GUID GOPGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP = NULL;
	Status =
		ST->BootServices->LocateProtocol(&GOPGuid, NULL, (void **)&GOP);
	if (EFI_ERROR(Status))
		return Status;

	BootData.FBBase = (void *)GOP->Mode->FrameBufferBase;
	BootData.FBSize = GOP->Mode->FrameBufferSize;
	BootData.FBWidth = GOP->Mode->Info->HorizontalResolution;
	BootData.FBHeight = GOP->Mode->Info->VerticalResolution;
	BootData.FBPixelsPerScanLine = GOP->Mode->Info->PixelsPerScanLine;

	return Status;
}

EFI_STATUS GetMemoryMap(UINTN *MapKey)
{
	EFI_STATUS Status;
	UINTN BufferSize = sizeof(EFI_MEMORY_DESCRIPTOR);
	UINTN DescriptorSize = 0;
	UINT32 DescriptorVersion = 0;

	Status = EFI_BUFFER_TOO_SMALL;
	while (Status == EFI_BUFFER_TOO_SMALL) {
		ST->BootServices->AllocatePool(EfiLoaderData, BufferSize,
					       (void **)&BootData.MMapBase);
		if (BootData.MMapBase) {
			Status = ST->BootServices->GetMemoryMap(
				&BufferSize,
				(EFI_MEMORY_DESCRIPTOR *)BootData.MMapBase,
				MapKey, &DescriptorSize, &DescriptorVersion);
			if (EFI_ERROR(Status)) {
				ST->BootServices->FreePool(BootData.MMapBase);
				BootData.MMapBase = NULL;
			}
		}
	}

	return Status;
}

EFI_STATUS Panic(EFI_STATUS Status, CHAR16 *message)
{
	EFI_INPUT_KEY Key;
	ST->ConOut->OutputString(ST->ConOut, message);
	ST->ConIn->Reset(ST->ConIn, FALSE);
	while (ST->ConIn->ReadKeyStroke(ST->ConIn, &Key) == EFI_NOT_READY)
		;

	return Status;
}

EFI_STATUS efi_main(EFI_HANDLE _ImageHandle, EFI_SYSTEM_TABLE *_ST)
{
	EFI_STATUS Status;

	ST = _ST;
	ImageHandle = _ImageHandle;

	EFI_FILE *KernelFile = NULL;
	Status = OpenFile(NULL, L"KERNEL.ERIK", &KernelFile);
	if (EFI_ERROR(Status))
		return Panic(Status, L"Cannot find the kernel!\r\n");

	UINT8 *KernelData = NULL;
	UINTN KernelLength = 0;
	UINTN KernelVirtualAddress = 0;
	UINTN KernelEntry = 0;
	Status = LoadElf(KernelFile, &KernelData, &KernelLength,
			 &KernelVirtualAddress, &KernelEntry);
	if (EFI_ERROR(Status))
		return Panic(Status, L"Cannot load the kernel!\r\n");

	EFI_FILE *InitrdFile = NULL;
	Status = OpenFile(NULL, L"INITRD.TAR", &KernelFile);
	if (!EFI_ERROR(Status) && InitrdFile) {
		Status = LoadFile(InitrdFile, (UINT8 **)&BootData.InitrdBase,
				  &BootData.InitrdSize);
		if (EFI_ERROR(Status)) {
			BootData.InitrdBase = NULL;
			BootData.InitrdSize = 0;
		}
	}

	Status = FindGOP();
	if (EFI_ERROR(Status))
		ST->ConOut->OutputString(ST->ConOut, L"Cannot initialize the framebuffer!\r\n");

	UINTN MapKey = 0;
	Status = GetMemoryMap(&MapKey);
	if (EFI_ERROR(Status))
		return Panic(Status, L"Cannot get the memory map!\r\n");

	ST->BootServices->ExitBootServices(ImageHandle, MapKey);
#if defined(__x86_64__) || defined(_M_X64)
	__attribute__((sysv_abi, noreturn)) void (*KernelMain)(BootInfo) =
		(__attribute__((sysv_abi, noreturn)) void (*)(BootInfo))(
			(UINTN)KernelData + KernelEntry - KernelVirtualAddress);
#else
	__attribute__((noreturn)) void (*KernelMain)(BootInfo) =
		(__attribute__((noreturn)) void (*)(BootInfo))(
			(UINTN)KernelData + KernelEntry - KernelVirtualAddress);
#endif
	KernelMain(BootData);
}
