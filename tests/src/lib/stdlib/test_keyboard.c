#include "../../test_helper.h"
#include "../../../../src/lib/device/keyboard.h"
#include "../../../../src/lib/device/ps2.h"
#include "../../../../src/lib/video/VGA_text.h"
#include "stdio.h"
#include "string.h"

#include <stdarg.h>

extern int highlight_offset;
extern VGA_Char *cursor;

int init_pos;

enum CMD {keyPress, checkOffset, checkPosition, end};

struct TestCMD {
    enum CMD cmd;
    union {
        struct PS2Buf_t kb;
        int offset;
    } data;
};

struct TestCMD kbCMD(enum KeyCode code, enum KeyState event, uint8_t modifiers) {
    KeyPress kp = (KeyPress){0, code, event, modifiers};
    struct PS2Buf_t b = (struct PS2Buf_t){PS2_KEY_EVENT, {.keyEvent = kp}};
    return (struct TestCMD){keyPress, {.kb = b}};
}

struct TestCMD chkOffCMD(int expected) {
    return (struct TestCMD){checkOffset, {.offset = expected}};
}

// expected position is relative to the beginning of vga memory
struct TestCMD chkPosCMD(unsigned expected) {
    return (struct TestCMD){checkPosition, {.offset = expected}};
}

struct TestCMD endCMD() {
    return (struct TestCMD){end, {}};
}

void execCMD(struct TestCMD cmd) {
    switch (cmd.cmd) {
    case checkOffset:
        ASSERT_M(highlight_offset == cmd.data.offset, "Highlight difference | ");
        break;
    case checkPosition:
        ASSERT_M(cursor - VGA_MEMORY == cmd.data.offset, "Position difference | ");
        break;
    case keyPress:
        vgaEditor(cmd.data.kb);
        break;
    case end:
        break;
    }
}

void test_main() {
    init_pos = cursor - VGA_MEMORY;
    struct TestCMD b[] = {
        chkPosCMD(init_pos),
        kbCMD(Key_a, KeyPressed, 0),
        chkOffCMD(0),
        chkPosCMD(init_pos + 1),
        kbCMD(Key_left, KeyPressed, KEY_MOD_SHIFT),
        chkOffCMD(-1),
        chkPosCMD(init_pos),
        kbCMD(Key_up, KeyPressed, KEY_MOD_SHIFT),
        chkOffCMD(-81),
        chkPosCMD(init_pos - 80),
        kbCMD(Key_up, KeyPressed, KEY_MOD_SHIFT),
        chkOffCMD(-161),
        chkPosCMD(0),
        kbCMD(Key_b, KeyPressed, 0),
        chkOffCMD(0),
        kbCMD(Key_down, KeyPressed, KEY_MOD_SHIFT),
        chkOffCMD(80),
        kbCMD(Key_up, KeyPressed, 0),
        chkOffCMD(0),
        // Remember to end command sequence!
        endCMD(),
    };
    for (int i = 0; b[i].cmd != end; i++) {
        execCMD(b[i]);
    }
    char done[] = "test_keyboard done\n";
    serialWrite(COM1, (uint8_t *)(done), sizeof(done) - 1);
}
