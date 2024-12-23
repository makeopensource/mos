#include "VGA_text.h"

#include "../../os/hard/port_io.h"

VGA_Cursor cursor = {VGA_MEMORY, 0};

VGA_Cursor *getCursor(void) {
    return &cursor;
}

VGA_Char getVGAchar(unsigned char chr, VGA_Color foreground,
                    VGA_Color background) {
    VGA_Char out = {chr, ((background << 4) | foreground)};
    return out;
}

void writeText(const char *str, int x, int y, VGA_Color color) {
    // culling
    if (x < 0 || x >= VGA_WIDTH)
        return;
    if (y < 0 || y >= VGA_HEIGHT)
        return;

    VGA_Char *buffer = VGA_MEMORY + x + VGA_WIDTH * y;

    unsigned short maxLen = VGA_WIDTH - x; // also culling
    for (unsigned short i = 0; i < maxLen && *str != '\0'; ++i) {
        // preserve background
        VGA_Char chr = {*str++, color ^ (buffer[i].color & 0xf0)};
        buffer[i] = chr;
    }
}

inline bool cursorIsAtStart(void) {
    return cursor.pos == VGA_MEMORY;
}

inline bool cursorIsAtEnd(void) {
    return cursor.pos == VGA_END - 1;
}

inline VGA_Color invert(VGA_Color color) {
    return 15 - (unsigned)color;
}

void updateCursorPos(void) {

    // evil (but useful) pointer arithmetic
    uint16_t pos = (cursor.pos - VGA_MEMORY);

    // low (mostly x but some y too)
    outb(VGA_ADDR_PORT, 0x0F);
    outb(VGA_DATA_PORT, (uint8_t)(pos & 0xFF));

    // high (only y)
    outb(VGA_ADDR_PORT, 0x0E);
    outb(VGA_DATA_PORT, (uint8_t)((pos >> 8) & 0xFF));
}

// checks and fixes the cursor, may do nothing
void adjustCursor(void) {
    if (cursor.pos < VGA_MEMORY) {
        cursor.pos = VGA_MEMORY;
    } else if (cursor.pos >= VGA_END) {
        scroll();

        // set cursor to start of last line
        cursor.pos = VGA_MEMORY + VGA_SIZE - VGA_WIDTH;
    }
    updateCursorPos();
}

void highlightCurrentChar(void) {
    *cursor.pos = getVGAchar(cursor.pos->chr, invert(cursor.pos->color & 0xf),
                             invert((cursor.pos->color >> 4) & 0xf));
}

#define CLEAR_CHAR(ptr) (getVGAchar(' ', white, (ptr)->color >> 4))

#define SIGNUM(x) (((x) > 0) - ((x) < 0))

void highlightDeletePrev(int offset) {
    if (offset < 0)
        cursor.pos -= offset;
    int abs_offset = SIGNUM(offset) * offset;
    for (int i = abs_offset; i >= 0; i--) {
        highlightCurrentChar();
        *cursor.pos = CLEAR_CHAR(cursor.pos);
        cursor.pos--;
    }
    cursor.pos++;
    adjustCursor();
}

void deletePrevChar(void) {
    if (!cursorIsAtStart()) {
        // preserve background
        VGA_Char clearChar = CLEAR_CHAR(cursor.pos - 1);
        *--cursor.pos = clearChar;
    }
    adjustCursor();
}

void highlightDeleteCurrent(int offset) {
    if (offset > 0)
        cursor.pos -= offset;
    int abs_offset = SIGNUM(offset) * offset;
    for (int i = abs_offset; i >= 0; i--) {
        highlightCurrentChar();
        *cursor.pos = CLEAR_CHAR(cursor.pos);
        cursor.pos++;
    }
    cursor.pos--;
    adjustCursor();
}

void deleteCurrentChar(void) {
    if (!cursorIsAtEnd()) {
        // preserve background
        VGA_Char clearChar = CLEAR_CHAR(cursor.pos);
        *cursor.pos++ = clearChar;
    }
    adjustCursor();
}

void cursorHighlightDown(int offset) {
    cursor.pos -= offset;
    int sign = SIGNUM(offset);
    int abs_offset = sign * offset;
    for (int i = abs_offset; i >= 0; i--) {
        *cursor.pos =
            getVGAchar(cursor.pos->chr, invert(cursor.pos->color & 0xf),
                       invert((cursor.pos->color >> 4) & 0xf));
        cursor.pos += sign;
    }
    cursor.pos -= sign;
    cursorDown();
    adjustCursor();
}

void cursorDown(void) {
    if (cursor.pos < VGA_END - VGA_WIDTH)
        cursor.pos += VGA_WIDTH;
    else
        cursor.pos = VGA_END - 1;
    adjustCursor();
}

void cursorHighlightUp(int offset) {
    cursor.pos -= offset;
    int sign = SIGNUM(offset);
    int abs_offset = sign * offset;
    for (int i = abs_offset; i >= 0; i--) {
        *cursor.pos =
            getVGAchar(cursor.pos->chr, invert(cursor.pos->color & 0xf),
                       invert((cursor.pos->color >> 4) & 0xf));
        cursor.pos += sign;
    }
    cursor.pos -= sign;
    cursorUp();
    adjustCursor();
}

void cursorUp(void) {
    if (cursor.pos > VGA_MEMORY + VGA_WIDTH)
        cursor.pos -= VGA_WIDTH;
    else
        cursor.pos = VGA_MEMORY;
    adjustCursor();
}

void cursorHighlightLeft(int offset) {
    if (offset < 0)
        cursor.pos -= offset;
    int abs_offset = SIGNUM(offset) * offset;
    for (int i = abs_offset; i >= 0; i--) {
        *cursor.pos =
            getVGAchar(cursor.pos->chr, invert(cursor.pos->color & 0xf),
                       invert((cursor.pos->color >> 4) & 0xf));
        cursor.pos--;
    }
    cursor.pos++;
    adjustCursor();
}

void cursorLeft(void) {
    if (!cursorIsAtStart())
        cursor.pos--;
    adjustCursor();
}

void cursorHighlightRight(int offset) {
    if (offset > 0)
        cursor.pos -= offset;
    int abs_offset = SIGNUM(offset) * offset;
    for (int i = abs_offset; i >= 0; i--) {
        *cursor.pos =
            getVGAchar(cursor.pos->chr, invert(cursor.pos->color & 0xf),
                       invert((cursor.pos->color >> 4) & 0xf));
        cursor.pos++;
    }
    cursor.pos--;
    adjustCursor();
}

void cursorRight(void) {
    if (!cursorIsAtEnd())
        cursor.pos++;
    adjustCursor();
}

void print(const char *str, VGA_Color color) {
    while (*str != 0) {
        // preserve background
        VGA_Char chr = {*str++, color ^ (cursor.pos->color & 0xf0)};
        *cursor.pos++ = chr;

        // prevent writing out of bounds!
        adjustCursor();
    }
}

void println(const char *str, VGA_Color color) {
    print(str, color);

    // update cursor position to next line
    cursor.pos += VGA_WIDTH;
    int remain = (int)(cursor.pos - VGA_MEMORY) % (VGA_WIDTH);
    cursor.pos -= remain;

    adjustCursor(); // bounds check
}

void scroll() {
    cursor.pos -= VGA_WIDTH; // move cursor up
    adjustCursor();          // bounds check

    VGA_Char *current = VGA_MEMORY;
    VGA_Char *next = VGA_MEMORY + VGA_WIDTH;

    // move all lines down
    for (unsigned short i = 0; i < VGA_SIZE - VGA_WIDTH; ++i) {
        current[i] = next[i];
    }

    // replace bottom
    VGA_Char bottomChr = {' ', black};
    for (unsigned short i = VGA_SIZE - VGA_WIDTH; i < VGA_SIZE; ++i) {
        current[i] = bottomChr;
    }
}

void clearScreen(VGA_Color color) {
    VGA_Char clearChr = getVGAchar(' ', white, color);

    clearScreenC(clearChr);
}

void clearScreenC(VGA_Char character) {
    for (unsigned short i = 0; i < VGA_SIZE; ++i) {
        VGA_MEMORY[i] = character;
    }
    cursor.pos = VGA_MEMORY; // reset cursor
    adjustCursor();
}