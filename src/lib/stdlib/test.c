#include "string.h"
#include "stdlib.h"
#include <stdio.h>

int test_strlen_s() {
    // tests general case
    char *str;
    int exp_len;
    str = "makeopensource!";
    exp_len = 15;
    if (strnlen_s(str, 100) != exp_len) {
        printf("invalid string length");
        return 1;
    }

    // tests empty case
    str = "";
    exp_len = 0;
    if (strnlen_s(str, 100) != exp_len) {
        printf("invalid string length");
        return 1;
    }

    // tests NULL ptr
    str = NULL;
    exp_len = 0;
    if (strnlen_s(str, 100) != exp_len) {
        printf("invalid string length");
        return 1;
    }

    // tests smaller strsz
    str = "makeopensource";
    exp_len = 4;
    if (strnlen_s(str, 4) != exp_len) {
        printf("invalid string length");
        return 1;
    }

    // tests strsz == 0
    str = "makeopensource";
    exp_len = 0;
    if (strnlen_s(str, 0) != exp_len) {
        printf("invalid string length");
        return 1;
    }

    puts("all strnlen_s tests passed!");

    return 0;
}

int test_atoi() {
    char *str;
    int exp;

    // tests basic input
    str = "12345";
    exp = 12345;
    if (atoi(str) != exp) {
        puts("invalid conversion");
        return 1;
    }

    // tests discarding spaces
    str = "   42";
    exp = 42;
    if (atoi(str) != exp) {
        puts("invalid conversion");
        return 1;
    }

    // tests leading "+"
    str = "+42";
    exp = 42;
    if (atoi(str) != exp) {
        puts("invalid conversion");
        return 1;
    }

    // tests leading "-"
    str = "-42";
    exp = -42;
    if (atoi(str) != exp) {
        puts("invalid conversion");
        return 1;
    }

    // tests leading 0s are discarded
    str = "0042";
    exp = 42;
    if (atoi(str) != exp) {
        puts("invalid conversion");
        return 1;
    }

    // tests only valid characters are counted
    str = "11x42";
    exp = 11;
    if (atoi(str) != exp) {
        puts("invalid conversion");
        return 1;
    }

    // tests invalid str input
    str = "a1234";
    exp = 0;
    if (atoi(str) != exp) {
        puts("invalid conversion");
        return 1;
    }

    // tests empty input
    str = "";
    exp = 0;
    if (atoi(str) != exp) {
        puts("invalid conversion");
        return 1;
    }

    // tests NULL input
    str = NULL;
    exp = 0;
    if (atoi(str) != exp) {
        puts("invalid conversion");
        return 1;
    }

    puts("all atoi tests passed!");
    return 0;
}

int test_memcpy() {
    int x;

    int mem[2];
    x = 5;
    memcpy(mem, &x, sizeof(x));

    // tests general memory copy
    if (mem[0] != x) {
        puts("invalid memory copy");
        return 1;
    }

    // tests variable type memory copy
    int bytes_copying = 3;
    int total_bytes = 8;
    char str[] = "12345678";
    x = 0;

    memcpy(str, &x, bytes_copying);
    for (int i = 0; i < bytes_copying; i++) {
        if (str[i] != 0) {
            puts("invalid memory copy");
            return 1;
        }
    }

    for (int i = bytes_copying; i < total_bytes; i++) {
        if (str[i] == 0) {
            puts("invalid memory copy");
            return 1;
        }
    }

    puts("all memcpy tests passed!");
    return 0;
}

int test_strncmp() {
    char *a;
    char *b;
    a = "more text";
    b = "more texts";
    if (strncmp(a, b, 20) == 0) {
        puts("invalid string comparison 1");
        return 1;
    }
    else if (strncmp(a, b, 9) != 0) {
        puts("invalid string comparison 2");
        return 1;
    }
    return 0;
}

int test_strcpy_s() {
    char *status;
    size_t buf_len;

    // testing overwriting text
    char buf_a[] = "some text";
    char buf_b[] = "more text";
    buf_len = 10;

    status = strcpy_s(buf_b, buf_len, buf_a);
    if (!strncmp(buf_a, buf_b, buf_len) == 0 || status == NULL) {
        puts("invalid string copy");
        return 1;
    }

    // testing copying into an empty array
    char buf_c[] = "some text";
    char buf_d[buf_len + 10];

    status = strcpy_s(buf_d, buf_len, buf_c);
    if (!strncmp(buf_c, buf_d, buf_len) == 0 || status == NULL) {
        puts("invalid string copy 1");
        return 1;
    }

    if (buf_d[buf_len - 1] != '\0') {
        puts("invalid string copy 2");
        return 1;
    }

    if (strnlen_s(buf_d, 100) != buf_len - 1) {
        puts("invalid string copy 3");
        return 1;
    }

    puts("all strncpy tests passed!");
    return 0;
}

int main() {
    int failed = 0;
    failed += test_strlen_s();
    // failed += test_sn_printf();
    failed += test_memcpy();
    failed += test_atoi();
    failed += test_strcpy_s();
    failed += test_strncmp();

    if (failed == 0) {
        puts("all tests passed!");
        return 0;
    }

    printf("%i tests failed\n", failed);
    return -1;
}
