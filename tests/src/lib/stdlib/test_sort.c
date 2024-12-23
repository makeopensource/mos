#include "stdlib.h"
#include "string.h"
#include "test_helper.h"

#include <stddef.h>
#include <stdint.h>

void verify_sorted(void *base, size_t nel, size_t width,
                   int (*compar)(const void *a, const void *b)) {
    ASSERT(nel >= 0);
    ASSERT(width > 0);

    if (nel == 0 || nel == 1)
        return;

    for (int i = 0; i < nel - 1; i++) {
        void *a = base + i * width;
        void *b = base + (i + 1) * width;
        ASSERT(compar(a, b) < 1);
    }
}

void verify_expected(void *base_a, void *base_b, size_t nel, size_t width) {
    char *a = (char *)base_a;
    char *b = (char *)base_b;
    for (int i = 0; i < nel * width; i++) {
        if (a[i] != b[i])
            ASSERT(0);
        return;
    }
}

int intcmpfunc(const void *a, const void *b) {
    int val_a = *(int *)a;
    int val_b = *(int *)b;
    if (val_a < val_b)
        return -1;
    else if (val_a == val_b)
        return 0;
    return 1;
}

typedef struct {
    char c;
    uint32_t i;
    double d;
} Type;

int cmpfunc(const void *a, const void *b) {
    Type val_a = *(Type *)a;
    Type val_b = *(Type *)b;
    if (val_a.c < val_b.c)
        return -1;
    else if (val_a.c > val_b.c)
        return 1;

    else if (val_a.i > val_b.i)
        return -1;
    else if (val_a.i < val_b.i)
        return 1;

    else if (val_a.d < val_b.d)
        return -1;
    else if (val_a.d > val_b.d)
        return 1;
    return 0;
}

void test_integer_sort() {
    int empty_arr[1] = {0};
    int empty_arr_expected[1] = {0};
    isort(&empty_arr, 0, sizeof(int), intcmpfunc);
    verify_sorted(empty_arr, 0, sizeof(int), intcmpfunc);
    verify_expected(empty_arr, empty_arr_expected, 0, sizeof(int));

    int single_arr[1] = {4};
    int single_arr_expected[1] = {4};
    isort(&single_arr, 1, sizeof(int), intcmpfunc);
    verify_sorted(single_arr, 1, sizeof(int), intcmpfunc);
    verify_expected(single_arr, single_arr_expected, 1, sizeof(int));

    int double_arr[2] = {-1, -2};
    int double_arr_expected[2] = {-2, -1};
    isort(&double_arr, 2, sizeof(int), intcmpfunc);
    verify_sorted(double_arr, 2, sizeof(int), intcmpfunc);
    verify_expected(double_arr, double_arr_expected, 2, sizeof(int));

    int multiple_arr[5] = {0, -4, 7, 5, 2};
    int multiple_arr_expected[5] = {-4, 0, 2, 5, 7};
    isort(&multiple_arr, 5, sizeof(int), intcmpfunc);
    verify_sorted(multiple_arr, 5, sizeof(int), intcmpfunc);
    verify_expected(multiple_arr, multiple_arr_expected, 5, sizeof(int));

    char done[] = "test_integer_sort done\n";
    serialWrite(COM1, (uint8_t *)(done), sizeof(done) - 1);
}

void test_struct_sort() {
    Type case_a[2] = {{.c = 'c', .i = 5, .d = 0.3},
                      {.c = 'a', .i = 5, .d = 0.3}};
    Type case_a_expected[2] = {{.c = 'a', .i = 5, .d = 0.3},
                               {.c = 'c', .i = 5, .d = 0.3}};

    isort(case_a, 2, sizeof(Type), cmpfunc);
    verify_sorted(case_a, 2, sizeof(Type), cmpfunc);
    verify_expected(case_a, case_a_expected, 2, sizeof(Type));

    Type case_b[5] = {
        {.c = 'b', .i = 8, .d = -0.3}, {.c = 'b', .i = 8, .d = 0.3},
        {.c = 'a', .i = 5, .d = 0.3},  {.c = 'b', .i = 8, .d = 2.7},
        {.c = 'b', .i = 5, .d = 0.3},
    };
    Type case_b_expected[5] = {
        {.c = 'a', .i = 5, .d = -0.3}, {.c = 'b', .i = 5, .d = 0.3},
        {.c = 'b', .i = 8, .d = -0.3}, {.c = 'b', .i = 8, .d = 0.3},
        {.c = 'b', .i = 8, .d = 2.7},
    };

    isort(case_b, 5, sizeof(Type), cmpfunc);
    verify_sorted(case_b, 5, sizeof(Type), cmpfunc);
    verify_expected(case_b, case_b_expected, 5, sizeof(Type));

    char done[] = "test_struct_sort done\n";
    serialWrite(COM1, (uint8_t *)(done), sizeof(done) - 1);
}

void test_main() {
    test_integer_sort();
    test_struct_sort();

    char done[] = "test_sort done\n";
    serialWrite(COM1, (uint8_t *)(done), sizeof(done) - 1);
}
