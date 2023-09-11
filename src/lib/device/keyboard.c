#include "keyboard.h"

#include "../pit/pit.h"
#include "stddef.h"

bool isNumpadNumber(enum KeyCode keycode) {
    switch (keycode) {
    case Key_num0:
    case Key_num1:
    case Key_num2:
    case Key_num3:
    case Key_num4:
    case Key_num5:
    case Key_num6:
    case Key_num7:
    case Key_num8:
    case Key_num9:
        return true;
    default:
        return false;
    }
}

bool isToggleableKey(enum KeyCode keycode) {
    return keycode == Key_capsLock || keycode == Key_numLock ||
           keycode == Key_scrollLock;
}

uint8_t getActiveModifiers(const struct KeyboardState *state) {
    uint8_t mods = 0;

    if (state->keystates[(unsigned)(Key_capsLock)] == KeyToggled) {
        mods |= KEY_MOD_CAPS;
    }

    if (state->keystates[(unsigned)(Key_LShift)] == KeyPressed ||
        state->keystates[(unsigned)(Key_RShift)] == KeyPressed) {
        mods |= KEY_MOD_SHIFT;
    }

    if (state->keystates[(unsigned)(Key_Lctrl)] == KeyPressed ||
        state->keystates[(unsigned)(Key_Rctrl)] == KeyPressed) {
        mods |= KEY_MOD_CTRL;
    }

    if (state->keystates[(unsigned)(Key_numLock)] == KeyToggled) {
        mods |= KEY_MOD_NUMLOCK;
    }

    if (state->keystates[(unsigned)(Key_scrollLock)] == KeyToggled) {
        mods |= KEY_MOD_SCRLOCK;
    }

    if (state->keystates[(unsigned)(Key_Lalt)] == KeyPressed ||
        state->keystates[(unsigned)(Key_Ralt)] == KeyPressed) {
        mods |= KEY_MOD_ALT;
    }

    if (state->keystates[(unsigned)(Key_Lcmd)] == KeyPressed ||
        state->keystates[(unsigned)(Key_Rcmd)] == KeyPressed) {
        mods |= KEY_MOD_CMD;
    }

    return mods;
}

// designated initializers! Lesser known C (not C++) feature.
static const char asciiLUT[] = {
    [Key_grave] = '`',      [Key_1] = '1',          [Key_2] = '2',
    [Key_3] = '3',          [Key_4] = '4',          [Key_5] = '5',
    [Key_6] = '6',          [Key_7] = '7',          [Key_8] = '8',
    [Key_9] = '9',          [Key_0] = '0',          [Key_minus] = '-',
    [Key_equal] = '=',      [Key_backspace] = 8,    [Key_tab] = '\t',
    [Key_q] = 'q',          [Key_w] = 'w',          [Key_e] = 'e',
    [Key_r] = 'r',          [Key_t] = 't',          [Key_y] = 'y',
    [Key_u] = 'u',          [Key_i] = 'i',          [Key_o] = 'o',
    [Key_p] = 'p',          [Key_openSquare] = '[', [Key_closeSquare] = ']',
    [Key_backSlash] = '\\', [Key_a] = 'a',          [Key_s] = 's',
    [Key_d] = 'd',          [Key_f] = 'f',          [Key_g] = 'g',
    [Key_h] = 'h',          [Key_j] = 'j',          [Key_k] = 'k',
    [Key_l] = 'l',          [Key_semicolon] = ';',  [Key_apostrophe] = '\'',
    [Key_enter] = '\n',     [Key_z] = 'z',          [Key_x] = 'x',
    [Key_c] = 'c',          [Key_v] = 'v',          [Key_b] = 'b',
    [Key_n] = 'n',          [Key_m] = 'm',          [Key_comma] = ',',
    [Key_period] = '.',     [Key_slash] = '/',      [Key_space] = ' ',
    [Key_esc] = 27,         [Key_numDiv] = '/',     [Key_numMult] = '*',
    [Key_numSub] = '-',     [Key_numAdd] = '+',     [Key_num7] = '7',
    [Key_num8] = '8',       [Key_num9] = '9',       [Key_num4] = '4',
    [Key_num5] = '5',       [Key_num6] = '6',       [Key_num1] = '1',
    [Key_num2] = '2',       [Key_num3] = '3',       [Key_numEnter] = '\n',
    [Key_num0] = '0',       [Key_numDecimal] = '.'};

// also note the correct usage of what Dr. Blanton tells you not to do.
static const size_t asciiLUTsize = sizeof(asciiLUT) / sizeof(asciiLUT[0]);

#define CAP_DELTA ('a' - 'A')

char keyPressToASCII(KeyPress press) {

    // out of bounds somehow
    if (press.code >= asciiLUTsize)
        return '\0';

    bool numLock = press.modifiers & KEY_MOD_NUMLOCK;
    bool numPadNum = isNumpadNumber(press.code);

    // numpad but numlock off
    if (!numLock && numPadNum)
        return '\0';

    // lookup char
    bool shifted = press.modifiers & KEY_MOD_SHIFT;
    char LUTchr = asciiLUT[(unsigned)(press.code)];

    // letters
    if (LUTchr >= 'a' && LUTchr <= 'z') {
        bool capitalize = (bool)(press.modifiers & KEY_MOD_CAPS) ^ shifted;
        if (capitalize) {
            return LUTchr - CAP_DELTA;
        }
        return LUTchr;
    }

    // non-letter
    if (shifted) {
        // numpad shifteds aren't ASCII
        if (numPadNum)
            return '\0';

        // LUT difficult for symbols
        switch (LUTchr) {
        case '`':
            return '~';
        case '1':
            return '!';
        case '2':
            return '@';
        case '3':
            return '#';
        case '4':
            return '$';
        case '5':
            return '%';
        case '6':
            return '^';
        case '7':
            return '&';
        case '8':
            return '*';
        case '9':
            return '(';
        case '0':
            return ')';
        case '-':
            return '_';
        case '=':
            return '+';
        case '[':
            return '{';
        case ']':
            return '}';
        case '\\':
            return '|';
        case ';':
            return ':';
        case '\'':
            return '"';
        case ',':
            return '<';
        case '.':
            return '>';
        case '/':
            return '?';
        default:
            return LUTchr;
        }
    }

    // not special, return it
    return LUTchr;
}

static const KeyPress nonePress = {0, Key_none, KeyReleased, 0};

KeyPress codePointDiscard(struct KeyboardState *, uint8_t) { return nonePress; }

static const enum KeyCode sc1LUT[] = {
    [0x1] = Key_esc, Key_1,          Key_2,           Key_3,
    Key_4,           Key_5,          Key_6,           Key_7,
    Key_8,           Key_9,          Key_0,           Key_minus,
    Key_equal,       Key_backspace,  Key_tab,         Key_q,
    Key_w,           Key_e,          Key_r,           Key_t,
    Key_y,           Key_u,          Key_i,           Key_o,
    Key_p,           Key_openSquare, Key_closeSquare, Key_enter,
    Key_Lctrl,       Key_a,          Key_s,           Key_d,
    Key_f,           Key_g,          Key_h,           Key_j,
    Key_k,           Key_l,          Key_semicolon,   Key_apostrophe,
    Key_grave,       Key_LShift,     Key_backSlash,   Key_z,
    Key_x,           Key_c,          Key_v,           Key_b,
    Key_n,           Key_m,          Key_comma,       Key_period,
    Key_slash,       Key_RShift,     Key_numMult,     Key_Lalt,
    Key_space,       Key_capsLock,   Key_f1,          Key_f2,
    Key_f3,          Key_f4,         Key_f5,          Key_f6,
    Key_f7,          Key_f8,         Key_f9,          Key_f10,
    Key_numLock,     Key_scrollLock, Key_num7,        Key_num8,
    Key_num9,        Key_numSub,     Key_num4,        Key_num5,
    Key_num6,        Key_numAdd,     Key_num1,        Key_num2,
    Key_num3,        Key_num0,       Key_numDecimal,  [0x57] = Key_f11,
    Key_f12};

static const size_t sc1LUTsize = sizeof(sc1LUT) / sizeof(sc1LUT[0]);

KeyPress codePointPS2SC1(struct KeyboardState *state, uint8_t code) {
    if (state == NULL)
        return nonePress;

    if (code == 0xE0) {
        state->extended = 1;
        return nonePress;
    }

    if (code == 0xE1) {
        state->extended = 3;
        return nonePress;
    }

    bool released = code & 0b10000000; // highest bit is set on release
    KeyPress out = {get_ticks(), Key_none, KeyReleased, 0};
    if (!released)
        out.event = KeyPressed;

    bool numLock = state->keystates[(unsigned)(Key_numLock)] == KeyToggled;

    uint8_t adjcode = code & 0b01111111; // code without release

    if (state->extended == 3) { // 1st extended byte of 0xE1
        state->extended = 2;
        return nonePress;
    } else if (state->extended == 2) { // 2nd extended byte of 0xE1
        // we are ignoring the pause key since it is extremely different
        state->extended = 0;
        return nonePress;
    } else if (state->extended == 1) { // 0xE0 multibyte
        switch (adjcode) {
        case 0x1C:
            out.code = Key_numEnter;
            break;
        case 0x1D:
            out.code = Key_Rctrl;
            break;
        case 0x35:
            out.code = Key_numDiv;
            break;
        case 0x38:
            out.code = Key_Ralt;
            break;
        case 0x47:
            out.code = Key_home;
            break;
        case 0x48:
            out.code = Key_up;
            break;
        case 0x49:
            out.code = Key_pgUp;
            break;
        case 0x4B:
            out.code = Key_left;
            break;
        case 0x4D:
            out.code = Key_right;
            break;
        case 0x4F:
            out.code = Key_end;
            break;
        case 0x50:
            out.code = Key_down;
            break;
        case 0x51:
            out.code = Key_pgDown;
            break;
        case 0x52:
            out.code = Key_insert;
            break;
        case 0x53:
            out.code = Key_delete;
            break;
        case 0x5B:
            out.code = Key_Lcmd;
            break;
        case 0x5C:
            out.code = Key_Rcmd;
            break;
        case 0x5D:
            out.code = Key_app;
            break;
        case 0x9C:
            out.code = Key_numEnter;
            break;
        case 0x9D:
            out.code = Key_Rctrl;
            break;
        case 0xB5:
            out.code = Key_numDiv;
            break;
        case 0xB8:
            out.code = Key_Ralt;
            break;
        case 0x2A: // technically printscreen sends 4 bytes, but we can safely
                   // ignore some
            out.code = Key_printScreen;
            out.event = KeyPressed;
            break;
        case 0xB7: // printscreen is the only bad key we handle
            out.code = Key_printScreen;
            out.event = KeyReleased;
            break;
        default: // multimedia or non-existant
            break;
        }
        state->extended = 0;
    } else if (state->extended == 0) {

        // outside LUT bounds, abort early
        if (adjcode >= sc1LUTsize)
            return out;

        out.code = sc1LUT[adjcode];

        // handle toggleables
        if (isToggleableKey(out.code)) {
            if (state->keystates[(unsigned)(out.code)] != KeyToggled) {
                if (!released) { // set toggle on press
                    out.event = KeyToggled;
                }
            } else if (!released) { // untoggle toggled
                out.event = KeyPressed;
            } else { // keep it toggled on release
                out.event = KeyToggled;
            }
        }

        // when numlock is on, numlock nums have different meaning
        if (!numLock && isNumpadNumber(out.code)) {
            switch (out.code) {
            case Key_num7:
                out.code = Key_home;
                break;
            case Key_num8:
                out.code = Key_up;
                break;
            case Key_num9:
                out.code = Key_pgUp;
                break;
            case Key_num4:
                out.code = Key_left;
                break;
            case Key_num6:
                out.code = Key_right;
                break;
            case Key_num1:
                out.code = Key_end;
                break;
            case Key_num2:
                out.code = Key_down;
                break;
            case Key_num3:
                out.code = Key_pgDown;
                break;
            default: // some don't have another meaning
                out.code = Key_none;
                break;
            }
        }
    } else { // odd state
        state->extended = 0;
        return nonePress;
    }

    out.modifiers = getActiveModifiers(state);

    state->keystates[(unsigned)(out.code)] = out.event;

    if (out.event == KeyToggled) {
        if (released) {
            out.event = KeyReleased;
        } else {
            out.event = KeyPressed;
        }
    }

    return out;
}