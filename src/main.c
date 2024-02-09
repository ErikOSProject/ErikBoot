#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#include <erikboot.h>
#include <file.h>

EFI_SYSTEM_TABLE *ST;
EFI_HANDLE ImageHandle;
BootInfo BootData;

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

EFI_STATUS efi_main(EFI_HANDLE _ImageHandle, EFI_SYSTEM_TABLE *_ST)
{
	EFI_STATUS Status;
	EFI_INPUT_KEY Key;

	ST = _ST;
	ImageHandle = _ImageHandle;

	EFI_FILE *KernelFile = NULL;
	Status = OpenFile(NULL, L"KERNEL.ERIK", &KernelFile);
	if (EFI_ERROR(Status))
		return Status;

	UINT8 *KernelData = NULL;
	UINTN KernelLength = 0;
	Status = LoadFile(KernelFile, &KernelData, &KernelLength);
	if (EFI_ERROR(Status))
		return Status;

	Status = FindGOP();
	if (EFI_ERROR(Status))
		return Status;

	Status = ST->ConIn->Reset(ST->ConIn, FALSE);
	if (EFI_ERROR(Status))
		return Status;

	while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) ==
	       EFI_NOT_READY)
		;

	return Status;
}
