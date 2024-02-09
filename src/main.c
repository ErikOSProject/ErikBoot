#include <Uefi.h>

EFI_SYSTEM_TABLE *ST;

EFI_STATUS efi_main([[maybe_unused]] EFI_HANDLE ImageHandle,
                    EFI_SYSTEM_TABLE *_ST) {
  EFI_STATUS Status;
  EFI_INPUT_KEY Key;

  ST = _ST;
  Status = ST->ConOut->OutputString(ST->ConOut, L"Hello, world!\n\r");
  if (EFI_ERROR(Status))
    return Status;

  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if (EFI_ERROR(Status))
    return Status;

  while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY)
    ;

  return Status;
}
