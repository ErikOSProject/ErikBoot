#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#include <file.h>

extern EFI_SYSTEM_TABLE *ST;
extern EFI_HANDLE ImageHandle;

EFI_STATUS OpenFile(EFI_FILE *Directory, CHAR16 *FileName, EFI_FILE **File)
{
	EFI_STATUS Status;

	if (!Directory) {
		EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
		EFI_GUID LoadedImageProtocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;
		ST->BootServices->HandleProtocol(ImageHandle,
						 &LoadedImageProtocol,
						 (void **)&LoadedImage);

		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFS;
		EFI_GUID SimpleFSProtocol =
			EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
		ST->BootServices->HandleProtocol(LoadedImage->DeviceHandle,
						 &SimpleFSProtocol,
						 (void **)&SimpleFS);

		Status = SimpleFS->OpenVolume(SimpleFS, &Directory);
		if (EFI_ERROR(Status))
			return Status;
	}

	Status = Directory->Open(Directory, File, FileName, EFI_FILE_MODE_READ,
				 EFI_FILE_READ_ONLY);
	return Status;
}

EFI_STATUS GetFileInfo(EFI_FILE *File, EFI_FILE_INFO **FileInfo,
		       UINTN *FileInfoSize)
{
	EFI_STATUS Status;

	EFI_GUID FileInfoGuid = EFI_FILE_INFO_ID;
	Status = File->GetInfo(File, &FileInfoGuid, FileInfoSize, NULL);

	if (Status == EFI_BUFFER_TOO_SMALL) {
		ST->BootServices->AllocatePool(EfiLoaderData, *FileInfoSize,
					       (void **)FileInfo);
		if (*FileInfo) {
			Status = File->GetInfo(File, &FileInfoGuid,
					       FileInfoSize, *FileInfo);
			if (EFI_ERROR(Status)) {
				ST->BootServices->FreePool(*FileInfo);
				*FileInfo = NULL;
			}
		}
	}

	return Status;
}

EFI_STATUS LoadFile(EFI_FILE *File, UINT8 **FileData, UINTN *FileLength)
{
	EFI_STATUS Status;

	EFI_FILE_INFO *FileInfo = NULL;
	UINTN FileInfoSize = 0;
	Status = GetFileInfo(File, &FileInfo, &FileInfoSize);
	if (EFI_ERROR(Status))
		return Status;

	*FileLength = FileInfo->FileSize;
	ST->BootServices->FreePool(FileInfo);

	ST->BootServices->AllocatePool(EfiLoaderData, *FileLength,
				       (void **)FileData);
	Status = File->Read(File, FileLength, *FileData);
	if (EFI_ERROR(Status)) {
		ST->BootServices->FreePool(*FileData);
		*FileData = NULL;
	}

	return Status;
}
