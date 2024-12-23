#include "key_handlers.h"

#include "video/VGA_text.h"

void specialHandler(KeyPress out) {
    if (!(out.modifiers & KEY_MOD_SHIFT)) {
        switch (out.code) {
        case Key_backspace:
            if (getCursor()->highlight_offset) {
                highlightDeletePrev(getCursor()->highlight_offset);
                getCursor()->highlight_offset = 0;
            } else
                deletePrevChar();
            break;
        case Key_delete:
            if (getCursor()->highlight_offset) {
                highlightDeleteCurrent(getCursor()->highlight_offset);
                getCursor()->highlight_offset = 0;
            } else
                deleteCurrentChar();
            break;
        case Key_left:
            if (getCursor()->highlight_offset) {
                cursorHighlightLeft(getCursor()->highlight_offset);
                getCursor()->highlight_offset = 0;
            } else
                cursorLeft();
            break;
        case Key_down:
            if (getCursor()->highlight_offset) {
                cursorHighlightDown(getCursor()->highlight_offset);
                getCursor()->highlight_offset = 0;
            } else
                cursorDown();
            break;
        case Key_up:
            if (getCursor()->highlight_offset) {
                cursorHighlightUp(getCursor()->highlight_offset);
                getCursor()->highlight_offset = 0;
            } else
                cursorUp();
            break;
        case Key_right:
            if (getCursor()->highlight_offset) {
                cursorHighlightRight(getCursor()->highlight_offset);
                getCursor()->highlight_offset = 0;
            } else
                cursorRight();
            break;
        default:
            break;
        }
    } else {
        if (((out.code == Key_up || out.code == Key_left) &&
             cursorIsAtStart()) ||
            ((out.code == Key_down || out.code == Key_right) &&
             cursorIsAtEnd()))
            return;
        if ((out.code == Key_up || out.code == Key_left ||
             out.code == Key_down || out.code == Key_right) &&
            !getCursor()->highlight_offset)
            highlightCurrentChar();
        switch (out.code) {
        case Key_up:
            for (int i = 0; i < VGA_WIDTH && !cursorIsAtStart(); i++) {
                if (getCursor()->highlight_offset > 0) {
                    highlightCurrentChar();
                    cursorLeft();
                } else {
                    cursorLeft();
                    highlightCurrentChar();
                }
                getCursor()->highlight_offset--;
            }
            break;
        case Key_down:
            for (int i = 0; i < VGA_WIDTH && !cursorIsAtEnd(); i++) {
                if (getCursor()->highlight_offset < 0) {
                    highlightCurrentChar();
                    cursorRight();
                } else {
                    cursorRight();
                    highlightCurrentChar();
                }
                getCursor()->highlight_offset++;
            }
            break;
        case Key_left:
            if (getCursor()->highlight_offset > 0) {
                highlightCurrentChar();
                cursorLeft();
            } else {
                cursorLeft();
                highlightCurrentChar();
            }
            getCursor()->highlight_offset--;
            break;
        case Key_right:
            if (getCursor()->highlight_offset < 0) {
                highlightCurrentChar();
                cursorRight();
            } else {
                cursorRight();
                highlightCurrentChar();
            }
            getCursor()->highlight_offset++;
            break;
        default:
            break;
        }
        if ((out.code == Key_up || out.code == Key_left ||
             out.code == Key_down || out.code == Key_right) &&
            !getCursor()->highlight_offset)
            highlightCurrentChar();
    }
}