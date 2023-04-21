#include "paging.h"

#include "stdlib/string.h"

/*The page directory and it's tables must be 4KiB aligned (0x1000)
 * 0x90000 is the start of the stack, in other words,
 * the areas from 0x1000 to 0x90000 are in use!
 * But there is a nice open region we can use from 0x91000 - 0x9f000
 * (We technically have until 0x9fc00 before we enter ExBIOS data)
 */

#define PAGE_DIRECTORY_BASE 0x91000

// room for 3 page tables (12 MiB of mapped memory)
#define KERNEL_PT_BASE 0x92000
#define IDENTITY_PT_LIMIT 0x95000
#define TABLE_COUNT ((IDENTITY_PT_LIMIT - KERNEL_PT_BASE) / 0x1000)

PageDirectory *directory = (PageDirectory *)(PAGE_DIRECTORY_BASE);

bool pageTablePresent(PageDirectoryEntry tableEntry) {
    return tableEntry & ENTRY_PRESENT;
}

bool pageEntryPresent(PageTableEntry entry) { return entry & ENTRY_PRESENT; }

void setActiveDirectory(PageDirectory *dir) {
    if (dir == NULL)
        dir = directory;

    __asm__ volatile("mov cr3, %0" : : "a"(dir));
}

PageDirectory *getActivePageDir(void) {
    PageDirectory *dir = NULL;

    __asm__ volatile("mov %0, cr3" : "=r"(dir));

    return dir;
}

void resetTLB(void) { setActiveDirectory(getActivePageDir()); }

#define PAGE_TABLE_OFFSET 22
#define PAGE_ENTRY_OFFSET 12

void *vaddrToPaddr(void *vaddr) {

    PageDirectory *dir = getActivePageDir();

    // 10 bits of table (2^10 = 1024)
    uint16_t tableidx = (uint32_t)(vaddr) >> PAGE_TABLE_OFFSET;

    // there are 10 bits of entry
    uint16_t entryidx = ((uint32_t)(vaddr) >> PAGE_ENTRY_OFFSET) & 0b1111111111;

    // 12 bits of offset
    uint32_t paddr = (uint32_t)(vaddr)&0xfff;

    // get and verify page table
    PageDirectoryEntry tableEntry = dir->entries[tableidx];
    if (!pageTablePresent(tableEntry))
        return NULL;

    PageTable *table = (PageTable *)(tableEntry & ENTRY_ADDR);

    // get and verify page entry
    PageTableEntry entry = table->entries[entryidx];
    if (!pageEntryPresent(entry))
        return NULL;

    // add offset
    return (void *)(paddr + (entry & ENTRY_ADDR));
}

// identity maps the table at directory entry idx
void identityMapTable(uint16_t idx, uint32_t flags) {
    PageTable *table = (PageTable *)(directory->entries[idx] & ENTRY_ADDR);

    // 4GiB per directory
    // 4MiB per table
    uint32_t baseAddr = idx * 0x400000;

    for (uint32_t page_idx = 0; page_idx < PAGE_ENTRY_COUNT; ++page_idx) {
        PageTableEntry entry = flags & ~(ENTRY_ADDR);

        // 4KiB per entry
        entry |= (baseAddr + page_idx * 0x1000) & ENTRY_ADDR;
        table->entries[page_idx] = entry;
    }
}

// preconditions, idx < PAGE_ENTRY_COUNT, table is 4KiB aligned
void addDirectoryEntry(uint16_t idx, PageTable *table, uint32_t flags) {
    PageDirectoryEntry entry = flags & ~(ENTRY_ADDR);
    entry |= (uint32_t)(table)&ENTRY_ADDR;
    directory->entries[idx] = entry;
}

void initPaging(void) {
    // clear the memory (essentially say no page tables exist)
    memset(directory, 0, PAGE_ENTRY_COUNT * sizeof(PageDirectoryEntry));

    // identity map 12MiB and setup directory
    for (uint16_t idx = 0; idx < TABLE_COUNT; ++idx) {
        PageTable *addr = (PageTable *)((idx * 0x1000) + KERNEL_PT_BASE);
        addDirectoryEntry(idx, addr, DEFAULT_ENTRY_FLAGS);
        identityMapTable(idx, DEFAULT_ENTRY_FLAGS);
    }

    setActiveDirectory(directory);

    // enable paging flags in cr0
    __asm__ volatile("mov eax, cr0\n\t"
                     "or eax, 0x80000001\n\t"
                     "mov cr0, eax");
}