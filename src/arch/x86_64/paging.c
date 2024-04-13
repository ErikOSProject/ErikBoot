#include <Uefi.h>

#include <erikboot.h>
#include <paging.h>

typedef struct {
	union {
		struct {
			UINTN Present : 1;
			UINTN ReadWrite : 1;
			UINTN UserSupervisor : 1;
			UINTN PageWriteThrough : 1;
			UINTN PageCacheDisable : 1;
			UINTN Accessed : 1;
			UINTN Dirty : 1;
			UINTN PageAccessType : 1;
			UINTN Global : 1;
			UINTN Ignored2 : 3;
			UINTN PageFrameNumber : 36;
			UINTN Reserved : 4;
			UINTN Ignored3 : 7;
			UINTN ProtectionKey : 4;
			UINTN ExecuteDisable : 1;
		};
		UINTN Value;
	};
} PTE;

PTE *PML4 = NULL;

extern BootInfo *BootData;
extern EFI_SYSTEM_TABLE *ST;

void *memset(void *destination, int c, size_t num);

EFI_STATUS PageMapRange(PTE TableBase[], size_t from, size_t to, void *target)
{
	EFI_STATUS Status = EFI_SUCCESS;

	for (size_t i = from; i < to; i++) {
		TableBase[i].Value = 0;
		TableBase[i].PageFrameNumber = (UINTN)target >> 12;
		TableBase[i].ReadWrite = TRUE;
		TableBase[i].Present = TRUE;
		target = (void *)((UINTN)target + 0x1000);
	}

	return Status;
}

EFI_STATUS PageMap(PTE *PML4, void *virtual, void *physical)
{
	EFI_STATUS Status = EFI_SUCCESS;

	size_t PDPi = (size_t) virtual / 4096 / 512 / 512 / 512 % 512;
	PTE *PDP;
	if (!PML4[PDPi].Present) {
		ST->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData,
						1,
						(EFI_PHYSICAL_ADDRESS *)&PDP);
		memset(PDP, 0, 4096);
		PageMapRange(PML4, PDPi, PDPi + 1, (void *)PDP);
	} else
		PDP = (PTE *)(PML4[PDPi].PageFrameNumber << 12);

	size_t PDi = (size_t) virtual / 4096 / 512 / 512 % 512;
	PTE *PD;
	if (!PDP[PDi].Present) {
		ST->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData,
						1, (EFI_PHYSICAL_ADDRESS *)&PD);
		memset(PD, 0, 4096);
		PageMapRange(PDP, PDi, PDi + 1, (void *)PD);
	} else
		PD = (PTE *)(PDP[PDi].PageFrameNumber << 12);

	size_t PTi = (size_t) virtual / 4096 / 512 % 512;
	PTE *PT;
	if (!PD[PTi].Present) {
		ST->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData,
						1, (EFI_PHYSICAL_ADDRESS *)&PT);
		memset(PT, 0, 4096);
		PageMapRange(PD, PTi, PTi + 1, (void *)PT);
	} else
		PT = (PTE *)(PD[PTi].PageFrameNumber << 12);

	size_t Pi = (size_t) virtual / 4096 % 512;
	PageMapRange(PT, Pi, Pi + 1, (void *)physical);

	return Status;
}

EFI_STATUS PageIdentityMap(void)
{
	EFI_STATUS Status = EFI_SUCCESS;

	PTE *PDP = NULL;
	ST->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1,
					(EFI_PHYSICAL_ADDRESS *)&PDP);
	PageMapRange(PML4, 0, 1, (void *)PDP);

	PTE *PDs = NULL;
	ST->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 8,
					(EFI_PHYSICAL_ADDRESS *)&PDs);
	PageMapRange(PDP, 0, 8, (void *)PDs);

	PTE *PTs = NULL;
	ST->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData,
					512 * 8, (EFI_PHYSICAL_ADDRESS *)&PTs);
	PageMapRange(PDs, 0, 512 * 8, (void *)PTs);
	PageMapRange(PTs, 0, 512 * 512 * 8, 0);

	return Status;
}

EFI_STATUS PageHigherHalfMap(UINTN KernelVirtualAddress, UINT8 *KernelData,
			     UINTN KernelLength)
{
	EFI_STATUS Status = EFI_SUCCESS;

	UINTN Virtual = KernelVirtualAddress & ~0xfff;
	for (size_t i = 0; i < KernelLength; i++) {
		PageMap(PML4, (void *)Virtual, KernelData + 4096 * i);
		Virtual += 4096;
	}

	if (BootData->FBBase) {
		UINTN VFBBase = Virtual + ((UINTN)BootData->FBBase & 0xfff);
		for (size_t i = 0; i < 1 + BootData->FBSize / 4096; i++) {
			PageMap(PML4, (void *)Virtual,
				(void *)(((UINTN)BootData->FBBase & ~0xfff) +
					 4096 * i));
			Virtual += 4096;
		}
		BootData->FBBase = (void *)VFBBase;
	}

	if (BootData->InitrdBase) {
		UINTN VInitrdBase =
			BootData->InitrdBase ?
				Virtual +
					((UINTN)BootData->InitrdBase & 0xfff) :
				0;
		for (size_t i = 0; i < 1 + BootData->InitrdSize / 4096; i++) {
			PageMap(PML4, (void *)Virtual,
				(void *)(((UINTN)BootData->InitrdBase & ~0xfff) +
					 4096 * i));
			Virtual += 4096;
		}
		BootData->InitrdBase = (void *)VInitrdBase;
	}

	return Status;
}

EFI_STATUS EnablePaging(void)
{
	EFI_STATUS Status = EFI_SUCCESS;

	asm volatile("movq %0, %%cr3" ::"r"(PML4) : "memory");

	return Status;
}

EFI_STATUS PreparePaging(UINTN KernelVirtualAddress, UINT8 *KernelData,
			 UINTN KernelLength)
{
	EFI_STATUS Status;

	ST->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1,
					(EFI_PHYSICAL_ADDRESS *)&PML4);
	memset(PML4, 0, 4096);
	Status = PageIdentityMap();
	if (EFI_ERROR(Status))
		return Status;
	Status = PageHigherHalfMap(KernelVirtualAddress, KernelData,
				   KernelLength);
	if (EFI_ERROR(Status))
		return Status;

	return Status;
}
