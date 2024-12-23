#include "stdlib.h"
#include "string.h"
#include "test_helper.h"

#define BUFSZ 100

void test_atoi(char *in, int exp) {
    ASSERT(atoi(in) == exp);
}

// expsz is the last argument because it is the size of the second argument
void test_itoa_s(int in, char *exp, int bufsz, int cmpsz) {
    char buf[BUFSZ + 1];
    memset(buf, '#', BUFSZ);
    buf[BUFSZ] = '\0';

    itoa_s(in, buf, bufsz);

    int i;
    for (i = 0; i < cmpsz - 1; i++) {
        ASSERT(buf[i] == exp[i]);
    }

    if (cmpsz != 0) {
        ASSERT(buf[i] == '\0');
        i++;
    }

    for (; i < BUFSZ; i++) {
        ASSERT(buf[i] == '#');
    }
}

void test_main() {

    test_atoi("12345", 12345);
    test_atoi("    42", 42);
    test_atoi(" +42", 42);
    test_atoi(" -42", -42);
    test_atoi(" 0042", 42);
    test_atoi(" 11x42", 11);
    test_atoi(" x1234", 0);
    test_atoi(" ab3", 0);

    test_itoa_s(1523, "1523\0", 100, 5);
    test_itoa_s(-1523, "-1523\0", 100, 6);
    test_itoa_s(0, "0\0", 100, 2);
    test_itoa_s(-0, "0\0", 100, 2);
    test_itoa_s(142, "", 0, 0);
    test_itoa_s(12345678, "123\0", 4, 4);

    char done[] = "test_stdlib done\n";
    serialWrite(COM1, (uint8_t *)(done), sizeof(done) - 1);
}
