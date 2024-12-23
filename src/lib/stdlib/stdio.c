#include "stdio.h"

#include "stdlib.h"
#include "string.h"

#define MAX_SNPRINTF_STRING 100

int snprintf(char *restrict buffer, size_t bufsz, const char *format, ...);
int vsnprintf(char *restrict buffer, size_t bufsz, const char *format,
              va_list ap);

// Inspired by chapter 7.3 of The C Programming Language
int snprintf(char *restrict buffer, size_t bufsz, const char *format, ...) {
    va_list ap;
    int retval;
    va_start(ap, format);
    retval = vsnprintf(buffer, bufsz, format, ap);
    va_end(ap);

    return retval;
}

int vsnprintf(char *restrict buffer, size_t bufsz, const char *format,
              va_list ap) {
    const char *p;

    // valid types
    unsigned char c; // printf("%c\n", 'c');
    char *s;         // printf("%s\n", "hello, world!");
    int i;           // printf("%i\n", 42);

    int n = 0;
    for (p = format; *p != '\0' && n < bufsz - 1; n++, p++) {
        if (*p != '%') {
            *buffer = *p;
            buffer++;
        } else {
            p++;
            char fmtchar = *p;
            size_t len;
            switch (fmtchar) {

            // print "%"
            case '%':
                *buffer = *p;
                buffer++;
                break;

            // string formatting
            case 's':
                s = va_arg(ap, char *);
                len = strnlen_s(s, MAX_SNPRINTF_STRING - n);

                // checks if there is space for the entire string
                // plus the null-terminating byte
                if (len + n + 1 < bufsz) {
                    memcpy(buffer, s, len);
                    buffer += len;
                    n += len - 1;
                } else {
                    return n;
                }

                break;

            // character formatting
            case 'c':
                c = (char)va_arg(ap, int);
                memcpy(buffer, &c, sizeof(char));
                buffer++;
                break;

            default:
                *buffer = *p;
                buffer++;
                break;

            case 'i':
                i = va_arg(ap, int);
                int temp = i;
                len = 0;
                if (i < 0) {
                    len++;
                    temp *= -1;
                }
                for (; temp >= 1; temp = temp / 10, len++)
                    ;
                if (i == 0)
                    len++;

                // checks if there is space for the entire string
                // plus the null-terminating byte
                if (len + n + 1 < bufsz) {
                    itoa_s(i, buffer, MAX_SNPRINTF_STRING - n);
                    buffer += len;
                    n += len - 1;
                } else {
                    return n;
                }
                break;
            }
        }
    }

    if (bufsz != 0) {
        *buffer = '\0';
    }
    return n;
}
