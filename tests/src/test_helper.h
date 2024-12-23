#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include "device/serial.h"
#include "stdio.h"
#include "string.h"

#include <stdarg.h>

void _print_assert(const char *fmt, const char *msg, ...) {
    va_list args;
    va_start(args, msg);

    while (!serialWriteReady(COM1))
        ;
    serialWrite(COM1, (uint8_t *)msg, strnlen_s(msg, 256));

    // avoid an empty line
    // must be index 1 since a newline is appended by the macro
    if (fmt[1] != 0) {
        // buffer to write the message to
        char buff[256];
        int len = vsnprintf(buff, sizeof(buff), fmt, args);

        while (!serialWriteReady(COM1))
            ;
        serialWrite(COM1, (uint8_t *)buff, len);
    }

    va_end(args);
}

#define AS_STR(x) #x

// __LINE__ loves to expand as "__LINE__" instead of the linenum
#define _STR_LINE(line) AS_STR(line)
#define STR_LINE _STR_LINE(__LINE__)

#define ASSERT_M(condition, fmt, ...)                                          \
    if (!(condition)) {                                                        \
        _print_assert(fmt "\n", "Line " STR_LINE ": Assertion '" #condition    \
                                "' failed.\n" __VA_OPT__(, ) __VA_ARGS__);     \
    }

#define FAIL_M(fmt, ...)                                                       \
    _print_assert(fmt "\n",                                                    \
                  "Line " STR_LINE                                             \
                  ": Fail assertion reached.\n" __VA_OPT__(, ) __VA_ARGS__)

#define ASSERT(condition) ASSERT_M(condition, "")

#endif