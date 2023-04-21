#include "device/serial.h"
#include "hard/idt.h"
#include "paging.h"
#include "pit/pit.h"
#include "test.h"
#include "video/VGA_text.h"

int os_main() {
    makeInterruptTable();
    initPaging();
    init_pit();
    serialInit();
    clearScreen(black);

    writeText("Welcome To mOS!", (80 - 15) / 2, 5, red);

    println("It booted!!!", green);

    VGA_Color colour = light_cyan;
    const char *string = "Hello, World!";
    println(string, colour);

    static const char test_str[] = "test";
    size_t test_idx = 0;

    while (1 + 2 == 3) {

        uint8_t chr = serialReadByteBlocking(COM1);

        if (chr == test_str[test_idx]) {
            ++test_idx;

            if (test_idx >= sizeof(test_str))
                enterTesting();
        } else {
            test_idx = 0;
        }
    }

    return 0;
}
