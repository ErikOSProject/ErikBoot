#ifndef _PAGING_H
#define _PAGING_H

EFI_STATUS EnablePaging(void);
EFI_STATUS PreparePaging(UINTN KernelVirtualAddress, UINT8 *KernelData,
			 UINTN KernelLength);

#endif //_PAGING_H
