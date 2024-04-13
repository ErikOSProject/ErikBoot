#include <Uefi.h>

#include <erikboot.h>
#include <paging.h>

typedef struct {
	union {
		struct {
			UINTN Present : 1;
			UINTN TableBlock : 1;
			UINTN LowerAttributes : 10;
			UINTN Address : 36;
			UINTN Reserved : 4;
			UINTN UpperAttributes : 12;
		};
		UINTN Value;
	};
} PTE;

PTE *PGD = NULL;

extern BootInfo *BootData;
extern EFI_SYSTEM_TABLE *ST;

void *memset(void *destination, int c, size_t num);

EFI_STATUS PageMapRange(PTE TableBase[], UINTN LowerAttributes, size_t from,
			size_t to, void *target)
{
	EFI_STATUS Status = EFI_SUCCESS;

	for (size_t i = from; i < to; i++) {
		TableBase[i].Value = 0;
		TableBase[i].Address = (UINTN)target >> 12;
		TableBase[i].LowerAttributes = LowerAttributes;
		TableBase[i].TableBlock = 1;
		TableBase[i].Present = TRUE;
		target = (void *)((UINTN)target + 0x1000);
	}

	return Status;
}

EFI_STATUS PageMap(PTE *PGD, void *virtual, void *physical)
{
	EFI_STATUS Status = EFI_SUCCESS;

	size_t PTEi = (size_t) virtual / 4096 / 512 % 512;
	PTE *PT;
	if (!PGD[PTEi].Present) {
		ST->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData,
						1, (EFI_PHYSICAL_ADDRESS *)&PT);
		memset(PT, 0, 4096);
		PageMapRange(PGD, 0, PTEi, PTEi + 1, (void *)PT);
	} else
		PT = (PTE *)(PGD[PTEi].Address << 12);

	size_t Pi = (size_t) virtual / 4096 % 512;
	PageMapRange(PT, 0x1C3, Pi, Pi + 1, (void *)physical);

	return Status;
}

EFI_STATUS PageHigherHalfMap(UINTN KernelVirtualAddress, UINT8 *KernelData,
			     UINTN KernelLength)
{
	EFI_STATUS Status = EFI_SUCCESS;

	UINTN Virtual = KernelVirtualAddress & ~0xfff;
	for (size_t i = 0; i < KernelLength; i++) {
		PageMap(PGD, (void *)(Virtual & 0x7fff000),
			KernelData + 4096 * i);
		Virtual += 4096;
	}

	UINTN VFBBase = Virtual + ((UINTN)BootData->FBBase & 0xfff);
	for (size_t i = 0; i < 1 + BootData->FBSize / 4096; i++) {
		PageMap(PGD, (void *)(Virtual & 0x7fff000),
			(void *)(((UINTN)BootData->FBBase & ~0xfff) +
				 4096 * i));
		Virtual += 4096;
	}

	UINTN VInitrdBase =
		BootData->InitrdBase ?
			Virtual + ((UINTN)BootData->InitrdBase & 0xfff) :
			0;
	if (BootData->InitrdBase)
		for (size_t i = 0; i < 1 + BootData->InitrdSize / 4096; i++) {
			PageMap(PGD, (void *)(Virtual & 0x7fff000),
				(void *)(((UINTN)BootData->InitrdBase & ~0xfff) +
					 4096 * i));
			Virtual += 4096;
		}

	BootData->FBBase = (void *)VFBBase;
	BootData->InitrdBase = (void *)VInitrdBase;

	return Status;
}

EFI_STATUS EnablePaging(void)
{
	EFI_STATUS Status = EFI_SUCCESS;

	const UINTN tcr_el1 = 0x4B5253514;
	asm volatile("mrs x0, sctlr_el1;"
		     "and x0, x0, ~1;"
		     "msr sctlr_el1, x0;"
		     "isb;");
	asm volatile("msr ttbr1_el1, %0;"
		     "msr tcr_el1, %1;"
		     "isb;" ::"r"(PGD),
		     "r"(tcr_el1));
	asm volatile("mrs x0, sctlr_el1;"
		     "orr x0, x0, 1;"
		     "msr sctlr_el1, x0;"
		     "isb;");

	return Status;
}

EFI_STATUS PreparePaging(UINTN KernelVirtualAddress, UINT8 *KernelData,
			 UINTN KernelLength)
{
	EFI_STATUS Status = EFI_SUCCESS;

	ST->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1,
					(EFI_PHYSICAL_ADDRESS *)&PGD);
	memset(PGD, 0, 4096);
	Status = PageHigherHalfMap(KernelVirtualAddress, KernelData,
				   KernelLength);
	if (EFI_ERROR(Status))
		return Status;

	return Status;
}
