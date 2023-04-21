#ifndef PAGING_H
#define PAGING_H

#include <stdbool.h>
#include <stdint.h>

// unfortunately C's bitfields can't provide the packing we need
typedef uint32_t PageDirectoryEntry;
typedef uint32_t PageTableEntry;

#define PAGE_ENTRY_COUNT 1024
#define PAGE_SIZE 0x1000

// The entries share flags!
#define ENTRY_PRESENT 0b000000001
#define ENTRY_RW 0b000000010
#define ENTRY_US 0b000000100
#define ENTRY_PWT 0b000001000
#define ENTRY_PCD 0b000010000
#define ENTRY_ACESSED 0b000100000

// keep this flag should always be 0
#define ENTRY_PS 0b010000000

// only for page table entries, available for OS use in page directory
#define ENTRY_GLOBAL 0b100000000
#define ENTRY_DIRTY 0b001000000

// these bits are also available for the OS
#define ENTRY_AVL 0b111000000000

// bits 12-31
#define ENTRY_ADDR 0xfffff000

// default entry is present, read/writable, and accessable by user and
// supervisor
#define DEFAULT_ENTRY_FLAGS (ENTRY_PRESENT | ENTRY_RW | ENTRY_US)

// note: these structures are lacking attrib packed
typedef struct {
    PageDirectoryEntry entries[PAGE_ENTRY_COUNT];
} PageDirectory;

// note, this structure must be 4KiB aligned
typedef struct {
    PageTableEntry entries[PAGE_ENTRY_COUNT];
} PageTable;

bool pageTablePresent(PageDirectoryEntry tableEntry);
bool pageEntryPresent(PageTableEntry entry);

// sets the entry's physical page to that of addr
void setEntryAddr(PageTableEntry* entry, void* addr);

// NOTE: IF PageDirectory* IS NULL IT USES THE CURRENT DIRECTORY (unless otherwise specified)

// sets the active page directory, if NULL uses the identity directory
void setActivePageDir(PageDirectory *dir);

// gets the current page dir from cr3
PageDirectory *getActivePageDir(void);

/*
 *  resets the translation lookaside buffer
 *  the TLB needs to be reset whenever an entry is modified
 */
void resetTLB(void);

// adds a table to a directory, TLB must be reset manually if directory is the current page directory
void addTableToDirectory(PageDirectory* directory, uint16_t idx, PageTable *table, uint32_t flags);

// translation helpers
uint16_t vaddrDirectoryIdx(void *vaddr);
uint16_t vaddrEntryIdx(void *vaddr);
uint16_t vaddrOffset(void* vaddr);

// translates table indexes and offset to virtual address
void* toVaddr(uint16_t dirIdx, uint16_t tableIdx, uint16_t offset);

// returns the associated directory entry of vaddr, never null
PageDirectoryEntry* vaddrDirEntry(PageDirectory* directory, void *vaddr);

// returns the associated table entry of vaddr, null if invalid/unmapped address
PageTableEntry* vaddrTableEntry(PageDirectory* directory, void* vaddr);

// identity maps the PageTable at directory index idx
void identityMapTable(PageDirectory* directory, uint16_t idx, uint32_t flags);

/*
 * Converts virtual address to physical address
 * (according to the current page table/directory)
 * returns NULL when the address is invalid/unmapped
 */
void *vaddrToPaddr(PageDirectory *dir, void *vaddr);

/*
 * enables paging and identity maps the kernel (1st MiB)
 * as well as identity mapping 1MiB - 12MiB
 */
void initPaging(void);

#endif