#ifndef _ERIKBOOT_H
#define _ERIKBOOT_H

#include <stdint.h>

typedef struct {
  uint32_t Type;
  void *PhysicalStart;
  void *VirtualStart;
  uint64_t NumberOfPages;
  uint64_t Attribute;
} MMapEntry;

typedef struct {
    void* FBBase;
	size_t FBSize;
	unsigned int FBWidth;
	unsigned int FBHeight;
	unsigned int FBPixelsPerScanLine;
	MMapEntry *MMapBase;
    size_t MMapEntryCount;
	size_t MMapEntrySize;
	void *InitrdBase;
	size_t InitrdSize;
	void *EfiConfigurationTable;
	size_t EFIConfigurationTableEntryCount;
} BootInfo;

#endif //_ERIKBOOT_H
