#include "string.h"
#include "test_helper.h"

#define BUFSZ                                                                  \
    100 // the size of string compare buffers (to detect buffer overflow)

void test_strnlen_s(char *in, int sz, int exp) {
    ASSERT(sz <= BUFSZ);
    ASSERT(strnlen_s(in, sz) == exp);
}

// memcpy from void *in to char buf[]
void test_memcpy(void *in, int sz) {

    ASSERT(sz <= BUFSZ);

    char buf[BUFSZ + 1];
    memset(buf, '#', BUFSZ);
    buf[BUFSZ] = '\0';

    memcpy(buf, in, sz);

    int i;

    // check that copied memory is copied correctly
    for (i = 0; i < sz; i++) {
        ASSERT(((char *)in)[i] == ((char *)buf)[i]);
    }

    // check that memory copies up to sz (NO buffer overflow)
    for (i = sz; i < BUFSZ; i++) {
        ASSERT(((char *)buf)[i] == '#');
    }
}

void test_strncmp(char *s1, char *s2, int sz, int exp) {
    ASSERT(sz <= BUFSZ);
    ASSERT(strncmp(s1, s2, sz) == exp);
}

// first two arguments are direct arguments to strcpy_s
// cmpsz is the expected size (if sz is larger than the
// actual strlen)
void test_strcpy_s(void *in, int sz, int cmpsz) {
    ASSERT(sz <= BUFSZ && cmpsz <= sz);

    char buf[BUFSZ + 1];
    memset(buf, '#', BUFSZ);
    buf[BUFSZ] = '\n';

    strcpy_s(buf, sz, in);

    int i;
    // tests that memory is copied appropriately
    for (i = 0; i < cmpsz - 1; i++) {
        ASSERT(((char *)in)[i] == ((char *)buf)[i]);
    }

    // test that last byte is `\0` IF it fits in sz
    if (cmpsz != 0) {
        ASSERT(((char *)buf)[i] == '\0');
        i++;
    }

    // tests that bytes *after* '\0' are not overwritten
    for (; i < BUFSZ; i++) {
        ASSERT(buf[i] == '#');
    }
}

void test_memswap(void *in_a, void *in_b, int sz) {
    ASSERT(sz > 0);
    char *a = (char *)in_a;
    char *b = (char *)in_b;
    char swap_a[BUFSZ + 1];
    char swap_b[BUFSZ + 1];
    swap_a[sz] = 'a';
    swap_b[sz] = 'b';

    memcpy(swap_a, a, BUFSZ);
    memcpy(swap_b, b, BUFSZ);

    memswap(swap_a, swap_b, sz);

    for (int i = 0; i < sz; i++) {
        ASSERT(a[i] == swap_b[i]);
        ASSERT(b[i] == swap_a[i]);
    }

    for (int i = sz; i < BUFSZ; i++) {
        ASSERT(swap_a[i] == a[i]);
        ASSERT(swap_b[i] == b[i]);
    }
}

int test_main() {
    test_strnlen_s("makeopensource!", 100, 15);
    test_strnlen_s("", 100, 0);
    test_strnlen_s("", 1, 0);
    test_strnlen_s(NULL, 100, 0);
    test_strnlen_s("makeopensource!", 4, 4);
    test_strnlen_s("makeopensource!", 0, 0);

    char c = '4';
    int i = 5;
    long l = 6;

    test_memcpy("", 0);
    test_memcpy("Hello, world!", 13);
    test_memcpy(&c, sizeof(char));
    test_memcpy(&i, sizeof(int));
    test_memcpy(&l, sizeof(long));

    test_strncmp("Hello there", "General Kenobi", 100, 'H' - 'G');
    test_strncmp("Hello", "Hello, world!", 100, '\0' - ',');
    test_strncmp("Hello", "Hello, world!", 5, 'o' - 'o');
    test_strncmp("Hello\0something", "Hello\0different", 100, 'o' - 'o');
    test_strncmp("Hello world", "Hello There!", 100, 'w' - 'T');

    test_strcpy_s("\0", 100, 1);
    test_strcpy_s("copy\0", 100, 5);
    test_strcpy_s("makeopensource\0", 8, 8);
    test_strcpy_s("don't copy me", 0, 0);

    test_memswap("12345678", "87654321", 8);

    char done[] = "test_string done\n";
    serialWrite(COM1, (uint8_t *)(done), sizeof(done) - 1);

    return 0;
}
