#include "ps2.h"

#include "../../os/hard/idt.h"
#include "../../os/hard/port_io.h"
#include "container/ring_buffer.h"

#define PS2_BUF_SIZE 64
#define PS2_TIMEOUT 100000

typedef ring_buffer_g(PS2_BUF_SIZE, struct PS2Buf_t) ps2_buffer_t;

ps2_buffer_t PS2Port1;
ps2_buffer_t PS2Port2;

static const struct PS2Buf_t initPS2BufVal = {.type = PS2_NONE_EVENT,
                                              .noneEvent = NULL};

struct PS2Buf_t peekDev(const ps2_buffer_t *dev) {
    struct PS2Buf_t out;
    InterruptState iprev = disableInterrupts();
    if (ring_buffer_empty(dev)) {
        out = initPS2BufVal;
    } else {
        out = ring_buffer_top_g(dev);
    }
    setInterrupts(iprev);
    return out;
}

struct PS2Buf_t popDev(ps2_buffer_t *dev) {
    struct PS2Buf_t out;
    InterruptState iprev = disableInterrupts();
    if (ring_buffer_empty(dev)) {
        out = initPS2BufVal;
    } else {
        ring_buffer_pop_g(dev, out);
    }
    setInterrupts(iprev);
    return out;
}

struct PS2Buf_t peekDev1(void) {
    return peekDev(&PS2Port1);
}

struct PS2Buf_t popDev1(void) {
    return popDev(&PS2Port1);
}

struct PS2Buf_t peekDev2(void) {
    return peekDev(&PS2Port2);
}

struct PS2Buf_t popDev2(void) {
    return popDev(&PS2Port2);
}

struct PS2Device dev1;
struct PS2Device dev2;

const struct PS2Device *getPortType(int portnum) {

    if (portnum == 1) {
        return &dev1;
    } else if (portnum == 2) {
        return &dev2;
    }

    return NULL;
}

// temporary include for #7
#include "video/VGA_text.h"

void ps2HandlerPort1(isr_registers_t *regs) {
    uint8_t b = inb(PS2_DATA);

    if (dev1.isKeyboard) {

        struct PS2Buf_t out;
        out.type = PS2_KEY_EVENT;
        out.keyEvent = dev1.kbState.translation(&dev1.kbState, b);

        if (out.keyEvent.code != Key_none)
            ring_buffer_push_g(&PS2Port1, out);

        // temporary to satisfy exactly what issue #7 says
        if (out.keyEvent.code != Key_none && out.keyEvent.event == KeyPressed) {
            char buf[2] = " ";
            buf[0] = keyPressToASCII(out.keyEvent);
            print(buf, white);
        }
    }
}
void ps2HandlerPort2(isr_registers_t *regs) {
    uint8_t b = inb(PS2_DATA);

    if (dev2.isKeyboard) {

        struct PS2Buf_t out;
        out.type = PS2_KEY_EVENT;
        out.keyEvent = dev2.kbState.translation(&dev2.kbState, b);

        if (out.keyEvent.code != Key_none)
            ring_buffer_push_g(&PS2Port1, out);
    }
}

enum DeviceType translateDeviceType(uint8_t b) {
    switch (b) {
    case StandardMouse:
    case ScrollMouse:
    case QuintMouse:
    case MF2Key1:
    case MF2Key2:
    case IBMShortKey:
    case NCD97Or122:
    case Standard122:
    case JPNGKey:
    case JPNPKey:
    case JPNAKey:
    case SunKey:
        return (enum DeviceType)(b);
    default:
        return Ps2Unknown;
    }
}

uint8_t readStat(void) { return inb(PS2_STAT_CMD); }

bool sendCMD(uint8_t b) {
    // await ready
    int i = 0;
    while (readStat() & 0b10) {
        if (i++ >= PS2_TIMEOUT)
            return false;
    }

    outb(PS2_STAT_CMD, b);
    return true;
}

bool dataReady(void) {
    uint8_t status = readStat();
    return status & 1;
}

uint8_t readData(void) {
    // await ready
    int i = 0;
    while (!dataReady()) {
        if (i++ >= PS2_TIMEOUT)
            return -1;
    }

    return inb(PS2_DATA);
}

bool sendData(uint8_t b) {
    int i = 0;
    while (readStat() & 0b10) {
        if (i++ >= PS2_TIMEOUT)
            return false;
    }

    outb(PS2_DATA, b);
    return true;
}

void ps2Disable(void) {
    sendCMD(0xAD);
    sendCMD(0xA7);
}

// ps2 should be disabled when this is called
void ps2Flush(void) {
    while (dataReady()) {
        // discard all data
        inb(PS2_DATA);
    }
}

uint8_t getConf(void) {
    sendCMD(0x20);
    return readData();
}

void setConf(uint8_t newconf) {
    sendCMD(0x60);
    sendData(newconf);
}

// public alias of sendData
bool sendPort1(uint8_t b) { return sendData(b); }

bool sendPort2(uint8_t b) {
    // note that short circuit eval will happen
    return sendCMD(0xD4) && sendData(b);
}

struct PS2Device detectDeviceType(uint8_t port) {
    struct PS2Device errDev = {
        .type = Ps2Unknown, .isKeyboard = false, .unknownState = NULL};

    if (port != 1 && port != 2)
        return errDev;

    bool (*send)(uint8_t);

    if (port == 1) {
        sendCMD(0xAE); // enable dev1
        sendCMD(0xA7); // disable dev2
        send = sendData;
    } else {
        sendCMD(0xA8); // enable dev2
        sendCMD(0xAD); // disable dev1
        send = sendPort2;
    }

    // disable scanning
    send(0xF5);

    if (readData() != 0xFA)
        return errDev;

    // identify command
    send(0xF2);
    if (readData() != 0xFA)
        return errDev;

    struct PS2Device outDev;
    switch (readData()) {
    case StandardMouse:
        outDev.type = StandardMouse;
        outDev.isKeyboard = false;

        break;
    case ScrollMouse:
        outDev.type = ScrollMouse;
        outDev.isKeyboard = false;

        break;
    case QuintMouse:
        outDev.type = QuintMouse;
        outDev.isKeyboard = false;

        break;
    case 0xAB:
    case 0xAC: // keyboard first bytes
        outDev.type = translateDeviceType(readData());
        outDev.isKeyboard = true;
        outDev.kbState.translation = codePointPS2SC1;
        outDev.kbState.extended = false;

        break;
    default:
        return errDev;
    }

    // enable scanning
    send(0xF4);

    return outDev;
}

bool ps2works = false;
bool port1works = false;
bool port2works = false;

bool ps2Present(void) { return ps2works; }
bool ps2Port1Present(void) { return port1works; }
bool ps2Port2Present(void) { return port2works; }

int ps2Init() {
    ring_buffer_init(&PS2Port1, initPS2BufVal);
    ring_buffer_init(&PS2Port2, initPS2BufVal);

    ps2Disable();
    ps2Flush();

    uint8_t conf = getConf();

    // see if 2nd port is potentially present
    bool port2present = conf & 0b00100000;

    setConf(conf & ~(0b01000011)); // disable IRQs

    // test controller
    sendCMD(0xAA);
    uint8_t ctest = readData();

    // controller test failed! PS2 broken!
    if (ctest == 0xFC) {
        ps2works = false;
        return ERR_PS2_BROKEN;
    }

    ps2works = true;

    setConf(getConf() & ~(0b01000011)); // set config again

    if (port2present) {
        sendCMD(0xA8); // enable port2

        // check configuration to see if port2 exists
        if (getConf() & 0b00100000) {
            port2present = false;
        } else {
            sendCMD(0xA7); // disable port2
        }
    }

    // test port 1
    sendCMD(0xAB);
    uint8_t res1 = readData();
    port1works = res1 == 0;

    // test port 2
    port2works = false;
    if (port2present) {
        sendCMD(0xA9);
        uint8_t res2 = readData();
        port2works = res2 == 0;
    }

    // set handlers and enable
    if (port1works) {
        irqSetHandler(1, ps2HandlerPort1);

        dev1 = detectDeviceType(1);
    }

    if (port2works) {
        irqSetHandler(12, ps2HandlerPort2);

        dev2 = detectDeviceType(2);

        // the detect disables dev1, so enable it
        if (port1works)
            sendCMD(0xAE);
    }

    setConf(getConf() | 0b01000011); // enable IRQs

    return 0;
}
