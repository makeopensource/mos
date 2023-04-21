#include "paging.h"

#include "stdlib/string.h"

/*The page directory and it's tables must be 4KiB aligned (0x1000)
 * 0x90000 is the start of the stack, in other words,
 * the areas from 0x1000 to 0x90000 are in use!
 * But there is a nice open region we can use from 0x91000 - 0x9f000
 * (We technically have until 0x9fc00 before we enter ExBIOS data)
 */

#define ID_PAGE_DIRECTORY_BASE 0x91000

// room for 3 page tables (12 MiB of mapped memory)
#define IDENTITY_PT_BASE 0x92000
#define IDENTITY_PT_LIMIT 0x95000
#define TABLE_COUNT ((IDENTITY_PT_LIMIT - IDENTITY_PT_BASE) / 0x1000)

PageDirectory *idendirectory = (PageDirectory *)(ID_PAGE_DIRECTORY_BASE);

bool pageTablePresent(PageDirectoryEntry tableEntry) {
    return tableEntry & ENTRY_PRESENT;
}

bool pageEntryPresent(PageTableEntry entry) { return entry & ENTRY_PRESENT; }

void setEntryAddr(PageTableEntry *entry, void *addr) {
    if (entry == NULL)
        return;

    *entry = ((uint32_t)(addr)&ENTRY_ADDR) | (*entry & ~(ENTRY_ADDR));
}

void setActivePageDir(PageDirectory *dir) {
    if (dir == NULL)
        dir = idendirectory;

    __asm__ volatile("mov cr3, %0" : : "a"(dir));
}

PageDirectory *getActivePageDir(void) {
    PageDirectory *dir = NULL;

    __asm__ volatile("mov %0, cr3" : "=r"(dir));

    return dir;
}

void resetTLB(void) { setActivePageDir(getActivePageDir()); }

#define PAGE_TABLE_OFFSET 22
#define PAGE_ENTRY_OFFSET 12

// highest 10 bits
uint16_t vaddrDirectoryIdx(void *vaddr) {
    return (uint32_t)(vaddr) >> PAGE_TABLE_OFFSET;
}

// middle 10 bits
uint16_t vaddrEntryIdx(void *vaddr) {
    return ((uint32_t)(vaddr) >> PAGE_ENTRY_OFFSET) & 0b1111111111;
}

// low 12 bits
uint16_t vaddrOffset(void *vaddr) { return (uint32_t)(vaddr)&0xfff; }

void *toVaddr(uint16_t dirIdx, uint16_t entryIdx, uint16_t offset) {
    uint32_t vaddr = offset;
    vaddr |= (uint32_t)(entryIdx) << PAGE_ENTRY_OFFSET;
    vaddr |= (uint32_t)(dirIdx) << PAGE_TABLE_OFFSET;
    return (void *)(vaddr);
}

PageDirectoryEntry *vaddrDirEntry(PageDirectory *directory, void *vaddr) {
    if (directory == NULL)
        directory = getActivePageDir();

    uint16_t tableidx = vaddrDirectoryIdx(vaddr);
    return &directory->entries[tableidx];
}

PageTableEntry *vaddrTableEntry(PageDirectory *directory, void *vaddr) {
    // this will never be null (unless something really bad happened)
    PageDirectoryEntry *dirEntry = vaddrDirEntry(directory, vaddr);
    PageTable *table = (PageTable *)((*dirEntry) & ENTRY_ADDR);

    if (table == NULL)
        return NULL;

    uint16_t entryidx = vaddrEntryIdx(vaddr);
    return &table->entries[entryidx];
}

void *vaddrToPaddr(PageDirectory *dir, void *vaddr) {

    if (dir == NULL)
        dir = getActivePageDir();

    // get and verify page entry
    PageTableEntry *entry = vaddrTableEntry(dir, vaddr);
    if (entry == NULL)
        return NULL;

    uint32_t paddr = vaddrOffset(vaddr);

    // apply offset
    return (void *)(paddr + ((*entry) & ENTRY_ADDR));
}

// identity maps the entire table at directory entry idx
void identityMapTable(PageDirectory *directory, uint16_t idx, uint32_t flags) {
    PageTable *table = (PageTable *)(directory->entries[idx] & ENTRY_ADDR);

    // 4GiB per directory
    // 4MiB per table
    uint32_t baseAddr = idx * 0x400000;

    for (uint32_t page_idx = 0; page_idx < PAGE_ENTRY_COUNT; ++page_idx) {
        PageTableEntry entry = flags & ~(ENTRY_ADDR);

        // 4KiB per entry
        entry |= (baseAddr + page_idx * PAGE_SIZE) & ENTRY_ADDR;
        table->entries[page_idx] = entry;
    }
}

// preconditions, idx < PAGE_ENTRY_COUNT, table is 4KiB aligned
void addTableToDirectory(PageDirectory *directory, uint16_t idx,
                         PageTable *table, uint32_t flags) {
    PageDirectoryEntry entry = flags & ~(ENTRY_ADDR);
    entry |= (uint32_t)(table)&ENTRY_ADDR;
    directory->entries[idx] = entry;
}

void initPaging(void) {
    // clear the memory (essentially say no page tables exist)
    memset(idendirectory, 0, PAGE_ENTRY_COUNT * sizeof(PageDirectoryEntry));

    // identity map 12MiB and setup directory
    for (uint16_t idx = 0; idx < TABLE_COUNT; ++idx) {
        PageTable *addr = (PageTable *)((idx * PAGE_SIZE) + IDENTITY_PT_BASE);
        memset(addr, 0, PAGE_ENTRY_COUNT * sizeof(PageTableEntry));
        addTableToDirectory(idendirectory, idx, addr, DEFAULT_ENTRY_FLAGS);
        identityMapTable(idendirectory, idx, DEFAULT_ENTRY_FLAGS);
    }

    setActivePageDir(idendirectory);

    // enable paging flags in cr0
    __asm__ volatile("mov eax, cr0\n\t"
                     "or eax, 0x80000001\n\t"
                     "mov cr0, eax");
}