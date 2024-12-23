#ifndef VGA_TEXT_H
#define VGA_TEXT_H

#include <stdbool.h>

// note VGA's width is equal to it's stride in text mode

#define VGA_MEMORY ((VGA_Char *)(0xB8000))
#define VGA_WIDTH (80)
#define VGA_HEIGHT (25)
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT)
#define VGA_END (VGA_MEMORY + VGA_SIZE)

#define VGA_ADDR_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

// All the VGA colors, in order
typedef enum {
    black,
    blue,
    green,
    cyan,
    red,
    magenta,
    brown,
    light_gray,
    dark_gray,
    light_blue,
    light_green,
    light_cyan,
    light_red,
    light_magenta,
    yellow,
    white
} VGA_Color;

// Struct that matches what the VGA buffer expects
typedef struct {
    // it's just the text character
    char chr;

    // upper 4-bits, background, lower 4-bits, foreground
    unsigned char color;
} VGA_Char;

typedef struct {
    VGA_Char *pos;
    int highlight_offset;
} VGA_Cursor;

VGA_Cursor *getCursor(void);

// returns a VGA_Char with the supplied attributes
VGA_Char getVGAchar(unsigned char chr, VGA_Color foreground,
                    VGA_Color background);

// prints text at location, will NOT wrap
void writeText(const char *str, int x, int y, VGA_Color color);

bool cursorIsAtStart(void);
bool cursorIsAtEnd(void);
VGA_Color invert(VGA_Color color);

void highlightCurrentChar(void);

// affect cursor

// deletion

void highlightDeletePrev(int offset);
void deletePrevChar(void);

void highlightDeleteCurrent(int offset);
void deleteCurrentChar(void);

// movement

void cursorHighlightDown(int offset);
void cursorDown(void);

void cursorHighlightUp(int offset);
void cursorUp(void);

void cursorHighlightLeft(int offset);
void cursorLeft(void);

void cursorHighlightRight(int offset);
void cursorRight(void);

// prints with wrapping, println does the same but adds a new line.
void print(const char *str, VGA_Color color);
void println(const char *str, VGA_Color color);

// shifts the entire screen up and replaces the bottom
void scroll();

// sets the background to color, foreground black, char ' '
void clearScreen(VGA_Color color);

// sets the screen to character
void clearScreenC(VGA_Char character);

#endif // end VGA_TEXT_H