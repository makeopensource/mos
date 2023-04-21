#include "../test_helper.h"

#include <../os/paging.h>

// 1st megabyte is a safe place to read from, but not to write to
#define MiB1 0x100000

// 3rd megabyte is safe to both read and write to
#define MiB3 (MiB1 * 3)

// placed so that the test goes over the table 0 and table 1 boundary
#define IDENT_LOCATION ((MiB1 * 4) - PAGE_SIZE)

void assert_identity(PageDirectory* dir, void* addr) {
    ASSERT(vaddrToPaddr(dir, addr) == addr);
}

void test_identity(PageDirectory* idDir) {

    // test 2 whole pages (and first byte of 3rd)
    for (int i = 0; i <= PAGE_SIZE * 2; ++i) {
        assert_identity(idDir, (void*)(IDENT_LOCATION + i));
    }

    char done[] = "test_identity done\n";
    serialWrite(COM1, (uint8_t *)(done), sizeof(done) - 1);
}

void assert_compose(uint16_t didx, uint16_t tidx, uint16_t off) {
    void* addr = toVaddr(didx, tidx, off);

    ASSERT(addr == (void*)((didx * 0x400000) + (tidx * 0x1000) + off));

    ASSERT(vaddrOffset(addr) == off);
    ASSERT(vaddrEntryIdx(addr) == tidx);
    ASSERT(vaddrDirectoryIdx(addr) == didx);
}

void test_composition(void) {

    assert_compose(0, 0, 0);
    assert_compose(0, 0, 0xfff);
    assert_compose(1023, 0, 0);
    assert_compose(0, 1023, 0);
    assert_compose(1023, 1023, 0xfff);
    assert_compose(123, 456, 789);

    char done[] = "test_composition done\n";
    serialWrite(COM1, (uint8_t *)(done), sizeof(done) - 1);
}

#define BOUND ((void*)(MiB1 * 4))

void test_swap_page(PageDirectory *base) {
    // steal the 3MiB page and 3 more
    PageDirectory* newDir = (PageDirectory*)(MiB3);
    PageTable* tables = (PageTable*)(MiB3 + sizeof(PageDirectory));

    // setup identity paged dir
    addTableToDirectory(newDir, 0, tables, DEFAULT_ENTRY_FLAGS);
    addTableToDirectory(newDir, 1, tables + 1, DEFAULT_ENTRY_FLAGS);
    addTableToDirectory(newDir, 2, tables + 2, DEFAULT_ENTRY_FLAGS);

    identityMapTable(newDir, 0, DEFAULT_ENTRY_FLAGS);
    identityMapTable(newDir, 1, DEFAULT_ENTRY_FLAGS);
    identityMapTable(newDir, 2, DEFAULT_ENTRY_FLAGS);

    // first and second entry of page table at index 1
    PageTableEntry* entry1 = vaddrTableEntry(newDir, BOUND);
    PageTableEntry* entry2 = vaddrTableEntry(newDir, BOUND + PAGE_SIZE);
    ASSERT(entry1 == &tables[1].entries[0]);
    ASSERT(entry2 == &tables[1].entries[1]);

    // map 4th MiB to 4th MiB + 1 page and vice versa
    setEntryAddr(entry1, BOUND + PAGE_SIZE);
    setEntryAddr(entry2, BOUND);
    ASSERT(vaddrToPaddr(newDir, BOUND) == BOUND + PAGE_SIZE);
    ASSERT(vaddrToPaddr(newDir, BOUND + PAGE_SIZE) == BOUND);

    uint32_t* magicLocation = (uint32_t*)(BOUND);
    uint32_t* boringLocation = (uint32_t*)(BOUND + PAGE_SIZE);

    // setup data
    magicLocation[0] = 0xdeadbeef;
    boringLocation[0] = 0xc0decafe;

    // swap to new directory
    setActivePageDir(newDir);

    // currently the two locations should be flipped
    ASSERT(magicLocation[0] == 0xc0decafe);
    ASSERT(boringLocation[0] == 0xdeadbeef);
    magicLocation[0] = 0xdeadbeef;
    boringLocation[0] = 0xc0decafe;

    // swap back
    setActivePageDir(base);

    // they swapped!!!
    ASSERT(magicLocation[0] == 0xc0decafe);
    ASSERT(boringLocation[0] == 0xdeadbeef);
   
    char done[] = "test_swap done\n";
    serialWrite(COM1, (uint8_t *)(done), sizeof(done) - 1);
}

void test_main() {
    test_composition();

    PageDirectory* idDir = getActivePageDir();
    ASSERT(idDir != NULL);

    test_identity(idDir);

    test_swap_page(idDir);
    
}