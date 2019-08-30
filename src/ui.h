#pragma once

#include <EasyButton.h>

/*
WIP:
// one ui screen. Receives button events and redraws screen
class UIScreen {
public:
    enum KeyType { KT_UP, KT_DOWN, KT_PRESS};

    virtual onKey(KeyType key) = 0;
    virtual draw() = 0;
};

// just displays info, key events transition to different screens
class MainScreen : public UIScreen {
public:

};
*/

class UI {
public:
    UI(byte key, byte s1, byte s2, byte rst, byte dc)
        : display(rst, dc, /*unused*/ 0), key(key), s1(s1), s2(s2) {}

    void init() {
        display.init();
        display.flipScreenVertically();
        display.setContrast(255);
        display.clear();

        intro_screen();

        key.begin();
        s1.begin();
        s2.begin();
    }

    void update() {
        key.read();
        s1.read();
        s2.read();

        // update the position based on latest changes to buttons
        if (s1.wasPressed()) {
            int inc = s2.isPressed() ? 1 : -1;
            position += inc;
        }
    }

protected:
    void intro_screen() {
        // WHATEVER, this is just a placeholder!
        uint8_t w = display.getWidth();
        uint8_t h = display.getHeight();

        display.setFont(ArialMT_Plain_24);
        display.drawString(10, 10, "LittleBeat");
        display.setFont(ArialMT_Plain_10);

        display.drawLine(5,5, w - 6, 5);
        display.drawLine(5,5, 5, h - 6);
        display.drawLine(w - 6, 5, w - 6, h - 6);
        display.drawLine(5, h - 6, w - 6, h - 6);

        display.display();
    }

    SH1106Spi display;

    EasyButton key;
    EasyButton s1;
    EasyButton s2;

    // current state
    int position = 0;
};
