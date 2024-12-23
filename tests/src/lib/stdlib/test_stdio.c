#include "stdio.h"
#include "string.h"
#include "test_helper.h"

#include <stdarg.h>

#define BUFSZ 100 // size of total buffer declared

// "expected" is the first argument because variable args need to be the last
// argument
void test_vsnprintf(char *expected, char *buffer, size_t bufsz, char *fmt,
                    int num, int argn, ...) {

    int n;
    memset(buffer, '#', BUFSZ);

    va_list args;
    va_start(args, argn);
    n = vsnprintf(buffer, bufsz, fmt, args);
    va_end(args);

    // serialWrite(COM1,  (uint8_t*)(buffer), BUFSZ);
    ASSERT(n <= bufsz);
    ASSERT(n == num);
    ASSERT(strncmp(buffer, expected, 2) == 0);
    ASSERT(buffer[bufsz] == '\0');
}

void test_main() {
    char buffer[BUFSZ + 1];
    buffer[BUFSZ] = '\0';

    test_vsnprintf("Hello!\n", buffer, BUFSZ, "%s\n", 7, 1, "Hello!");
    test_vsnprintf("i=5\n", buffer, BUFSZ, "i=%i\n", 4, 1, 5);
    test_vsnprintf("i=42\n", buffer, BUFSZ, "i=%i\n", 5, 1, 42);
    test_vsnprintf("i=-42\n", buffer, BUFSZ, "i=%i\n", 6, 1, -42);
    test_vsnprintf("just a regular string\n", buffer, BUFSZ,
                   "just a regular string\n", 22, 0);
    test_vsnprintf("i=-42%\n", buffer, BUFSZ, "i=%i%%\n", 6, 1, 42);
    test_vsnprintf("1, 2, 3, 4, 5\n", buffer, BUFSZ, "%i, %i, %i, %i, %i\n", 14,
                   5, 1, 2, 3, 4, 5);
    test_vsnprintf("i=0\n", buffer, BUFSZ, "i=%i\n", 4, 1, 0);

    char done[] = "test_stdio done\n";
    serialWrite(COM1, (uint8_t *)(done), sizeof(done) - 1);
}
