#pragma once

#include <EasyButton.h>
#include <SH1106Spi.h>

// fwds
class Drummer;
class UI;

using Display = SH1106Spi;

// all screen types
enum ScreenType { ST_MAIN, ST_PERC, ST_MIXER, ST_PARAM };

namespace peaks {
class Configurable;
} // namespace peaks

// one ui screen. Receives button events and redraws screen
class UIScreen {
public:
    enum KeyType { KT_UP, KT_DOWN, KT_PRESS, KT_BACK };

    UIScreen(UI &ui);

    virtual void onKey(KeyType key) = 0;

    virtual void update() {
        if (dirty) {
            draw();
            dirty = false;
        }
    }

    void mark_dirty() { dirty = true; }

protected:
    virtual void draw() = 0;
    bool dirty = true;

    UI &ui;
    Display &display;
};

// main status screen. should later show midi channel, note status... currently only menu
class MainScreen : public UIScreen {
public:
    struct Choice {
        ScreenType screen;
        const char *text;
    };

    static constexpr unsigned CHOICE_COUNT = 3;
    static const Choice choices[CHOICE_COUNT];

    MainScreen(UI &ui) : UIScreen(ui) {}

    void onKey(KeyType key) override;

    void draw() override;

    void set_index(int idx) {
        index = idx < CHOICE_COUNT ? idx : 0;
        mark_dirty();
    }

protected:
    int index = 0;
};

// For now this is a percussion selection screen
class PercussionScreen : public UIScreen {
public:
    PercussionScreen(UI &ui) : UIScreen(ui) {}
    void onKey(KeyType key) override;
    void draw() override;
    void set_index(int idx) { index = idx; mark_dirty(); }

protected:
    int index = 0;
};

// parameter selector/modifier for drums
class ParamScreen : public UIScreen {
public:
    ParamScreen(UI &ui) : UIScreen(ui) {
        // be sure to have precussion set...
        set_percussion(0);
    }

    void onKey(KeyType key) override;
    void draw() override;

    void set_percussion(int idx);
    void prev_percussion();
    void next_percussion();
protected:
    peaks::Configurable *percussion;
    const char *name; // name of the current percussion
    int perc_index = 0;
    int index = 0; // index of configurable parameter
    bool set_mode; // if true, we're setting the parameter, not selecting one
};

class MixerScreen : public UIScreen {
public:
    MixerScreen(UI &ui);

    void onKey(KeyType key) override;

    void draw() override;

    void set_index(int idx) {
        index = idx;
        mark_dirty();
    }

protected:
    void modify_current_setting(int increment);

    // this is volume+panning. If other option gets added we have to increase this
    static constexpr int SUBCHOICE_COUNT = 3;

    int idx_max;
    int index = 0;
    bool set_mode = false;
};

class UI {
public:
    UI(Drummer &drummer, byte key, byte s1, byte s2, byte back, byte rst, byte dc)
        : drummer(drummer), display(rst, dc, /*unused*/ 0), key(key), s1(s1),
          s2(s2), back(back), scrMain(*this), scrPerc(*this), scrParam(*this),
          scrMixer(*this)
    {
        active_screen = &scrMain;
    }

    void init() {
        display.init();
        display.flipScreenVertically();
        display.setContrast(255);
        display.clear();

        intro_screen();

        key.begin();
        s1.begin();
        s2.begin();
        back.begin();

        // TODO: Move the update loop to the other core via xTaskCreatePinnedToCore
    }

    // should be called when parameters change outside of UI (ie. CC changes)
    void mark_dirty() {
        if (active_screen) active_screen->mark_dirty();
    }

    void update() {
        key.read();
        s1.read();
        s2.read();
        back.read();

        // update the position based on latest changes to buttons
        if (s1.wasPressed()) {
            active_screen->onKey(s2.isPressed() ? UIScreen::KT_UP : UIScreen::KT_DOWN);
        }

        if (key.wasPressed()) {
            active_screen->onKey(UIScreen::KT_PRESS);
        }

        if (back.wasPressed()) {
            active_screen->onKey(UIScreen::KT_BACK);
        }

        active_screen->update();
    }

    void set_screen(ScreenType st) {
        UIScreen *new_scr = get_screen(st);
        if (new_scr) active_screen = new_scr;
        if (active_screen) active_screen->mark_dirty();
    }

    UIScreen *get_screen(ScreenType st) {
        switch (st) {
        case ST_MAIN:  return &scrMain;
        case ST_PERC:  return &scrPerc;
        case ST_PARAM: return &scrParam;
        case ST_MIXER: return &scrMixer;
        default: return nullptr;
        }
    }

    Drummer &get_drummer() {
        return drummer;
    }

    Display &get_display() {
        return display;
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

        delay(500);
    }

    Drummer &drummer;
    Display display;

    EasyButton key;
    EasyButton s1;
    EasyButton s2;
    EasyButton back;

    MainScreen scrMain;
    PercussionScreen scrPerc;
    ParamScreen scrParam;
    MixerScreen scrMixer;

    UIScreen *active_screen;
};
