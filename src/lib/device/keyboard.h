#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>

// scancodes get translated into keycodes
enum KeyCode {
    Key_none,

    // row 1
    Key_grave, // the ` key
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,
    Key_0,
    Key_minus,
    Key_equal,
    Key_backspace,

    // row 2
    Key_tab,
    Key_q,
    Key_w,
    Key_e,
    Key_r,
    Key_t,
    Key_y,
    Key_u,
    Key_i,
    Key_o,
    Key_p,
    Key_openSquare,
    Key_closeSquare,
    Key_backSlash,

    // row 3
    Key_capsLock,
    Key_a,
    Key_s,
    Key_d,
    Key_f,
    Key_g,
    Key_h,
    Key_j,
    Key_k,
    Key_l,
    Key_semicolon,
    Key_apostrophe,
    Key_enter,

    // row 4
    Key_LShift,
    Key_z,
    Key_x,
    Key_c,
    Key_v,
    Key_b,
    Key_n,
    Key_m,
    Key_comma,
    Key_period,
    Key_slash,
    Key_RShift,

    // row 5
    Key_Lctrl,
    Key_Lcmd, // left "Windows" key, referred to left GUI on osdev
    Key_Lalt,
    Key_space,
    Key_Ralt,
    Key_Rcmd,
    Key_app, // not present on many newer keyboards, replaced by function key
    Key_Rctrl,

    // row 0 (function keys)
    Key_esc,
    Key_f1,
    Key_f2,
    Key_f3,
    Key_f4,
    Key_f5,
    Key_f6,
    Key_f7,
    Key_f8,
    Key_f9,
    Key_f10,
    Key_f11,
    Key_f12,

    /* Extended Utility Keys */

    // row 0
    Key_printScreen,
    Key_scrollLock,
    Key_pause,

    // row 1
    Key_insert,
    Key_home,
    Key_pgUp,

    // row 3
    Key_delete,
    Key_end,
    Key_pgDown,

    // directional (DDR order)
    Key_left,
    Key_down,
    Key_up,
    Key_right,

    /* numpad */

    // row 1
    Key_numLock,
    Key_numDiv,
    Key_numMult,
    Key_numSub,

    // row 2
    Key_num7,
    Key_num8,
    Key_num9,
    Key_numAdd,

    // row 3
    Key_num4,
    Key_num5,
    Key_num6,

    // row 4
    Key_num1,
    Key_num2,
    Key_num3,

    Key_numEnter,

    // row 5
    Key_num0,
    Key_numDecimal,

    // NOTE: multimedia is NOT supported

    Key_Code_Count // very important that this is the last and there is no
                   // higher value
};

bool isNumpadNumber(enum KeyCode keycode);

bool isToggleableKey(enum KeyCode keycode);

// note: toggleable keys == KeyToggled when toggled, otherwise could be either
enum KeyState {
    KeyReleased,
    KeyPressed,
    KeyToggled, // used for keys such as caps lock
};

// Flags for modifiers
#define KEY_MOD_CAPS 1
#define KEY_MOD_SHIFT 2
#define KEY_MOD_CTRL 4
#define KEY_MOD_NUMLOCK 8
#define KEY_MOD_SCRLOCK 16
#define KEY_MOD_ALT 32
#define KEY_MOD_CMD 64

typedef struct {
    uint32_t time; // time of press, taken from PIT
    enum KeyCode code;
    enum KeyState
        event; // programs may want to monitor when any key event occurs
    uint8_t modifiers;
} KeyPress;

/*
 * Code point functions should return Key_none when no translation occurs,
 * When a key is pressed, the event should reflect such
 * It is recommended that they also update keystates and extended.
 */

struct KeyboardState; // forward declaration for typedef
typedef KeyPress(codePointFunc)(struct KeyboardState *, uint8_t);

struct KeyboardState {
    uint8_t extended; // for use in multi-byte scancodes
    enum KeyState keystates[(unsigned)(Key_Code_Count)];
    codePointFunc *translation;
};

// returns the modifiers of the current state (not thread safe)
uint8_t getActiveModifiers(const struct KeyboardState *state);

// returns 0 (NUL) when the press has no ASCII equivalent
char keyPressToASCII(KeyPress press);

// the default translation function, discards ALL input.
KeyPress codePointDiscard(struct KeyboardState *, uint8_t);

// translation for ps2 scancode set 1
KeyPress codePointPS2SC1(struct KeyboardState *, uint8_t);

#endif