#ifndef PS2_H
#define PS2_H

#include "keyboard.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PS2_DATA 0x60
#define PS2_STAT_CMD 0x64

#define ERR_PS2_BROKEN -1

enum DeviceType {
    StandardMouse = 0x00,
    ScrollMouse = 0x03,
    QuintMouse = 0x04,

    // after 0xAB
    MF2Key1 = 0x83,
    MF2Key2 = 0xC1,
    IBMShortKey = 0x84,
    NCD97Or122 = 0x85,
    Standard122 = 0x86,
    JPNGKey = 0x90,
    JPNPKey,
    JPNAKey,

    // after 0xAC
    SunKey = 0xA1,

    Ps2Unknown,
};

enum PS2KeyboardScanCode {
    Ps2None,
    SC1,
    SC2, // unimplemented
    SC3, // unimplemented
};

struct PS2MouseState {
    // unimplemented
};

struct PS2Device {
    enum DeviceType type;
    bool isKeyboard;

    union {
        struct KeyboardState kbState;
        struct PS2MouseState msState;
        void *unknownState; // NULL pointer
    };
};

enum PS2Buffer_Type { PS2_KEY_EVENT, PS2_MOUSE_EVENT, PS2_NONE_EVENT };

struct PS2Buf_t {
    enum PS2Buffer_Type type;
    union {
        KeyPress keyEvent;
        // unimplemented mouse event
        void *noneEvent; // NULL pointer
    };
};

int ps2Init();
const struct PS2Device *getPortType(int portnum);

bool ps2Present(void);
bool ps2Port1Present(void);
bool ps2Port2Present(void);

// send a command directly to one of the ports.
// response cannot be checked/retrived, beware.
bool sendPort1(uint8_t byte);
bool sendPort2(uint8_t byte);

struct PS2Buf_t peekDev1(void);
struct PS2Buf_t popDev1(void);

struct PS2Buf_t peekDev2(void);
struct PS2Buf_t popDev2(void);

#endif