#ifndef _FILE_H
#define _FILE_H

EFI_STATUS OpenFile(EFI_FILE *Directory, CHAR16 *FileName, EFI_FILE **File);
EFI_STATUS GetFileInfo(EFI_FILE *File, EFI_FILE_INFO **FileInfo,
		       UINTN *FileInfoSize);
EFI_STATUS LoadFile(EFI_FILE *File, UINT8 **FileData, UINTN *FileLength);

#endif //_FILE_H
