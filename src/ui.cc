#include "ui.h"
#include "peaks-drums.h"
#include "drummer.h"

namespace {

constexpr byte CURSOR_SIZE = 5;

// renders a gauge of 16bit number
void draw_gauge(Display &display, byte x, byte y, byte length, byte height,
                uint16_t value, bool cursor = false)
{
    byte pos = (uint32_t)value * (uint32_t)length / (1<<16);
    display.drawRect(x,y, length, height);
    display.fillRect(x,y + 1, pos, height - 2);

    // TODO: explore using drawProgressBar instead

    if (cursor) {
        display.drawLine(x + pos, y + height, x + pos - CURSOR_SIZE / 2,
                         y + height + CURSOR_SIZE);
        display.drawLine(x + pos, y + height, x + pos + CURSOR_SIZE / 2,
                         y + height + CURSOR_SIZE);
        display.drawLine(x + pos - CURSOR_SIZE / 2, y + height + CURSOR_SIZE,
                         x + pos + CURSOR_SIZE / 2, y + height + CURSOR_SIZE);
    }
}

void draw_cursor_horizonal(Display &display, byte x, byte y) {
    display.drawLine(x, y,
                     x + CURSOR_SIZE, y + CURSOR_SIZE/2);

    display.drawLine(x, y + CURSOR_SIZE,
                     x + CURSOR_SIZE, y + CURSOR_SIZE/2);

    display.drawLine(x, y,
                     x, y + CURSOR_SIZE);
}

constexpr uint16_t ROTENCODER_STEP = 1024;

/// safely increments the value by N steps
void safe_incr(uint16_t &tgt, uint16_t steps = ROTENCODER_STEP) {
    if (tgt <= (0xFFFF - steps))
        tgt+=steps;
    else
        tgt=0xFFFF;
}

void safe_decr(uint16_t &tgt, uint16_t steps = ROTENCODER_STEP) {
    if (tgt >= steps)
        tgt -= steps;
    else
        tgt = 0;
}

/// safely increments the value by N steps
void safe_incr(int16_t &tgt, uint16_t steps = ROTENCODER_STEP) {
    if (tgt <= (INT16_MAX - steps))
        tgt+=steps;
    else
        tgt=INT16_MAX;
}

void safe_decr(int16_t &tgt, uint16_t steps = ROTENCODER_STEP) {
    if (tgt >= INT16_MIN + steps)
        tgt -= steps;
    else
        tgt = INT16_MIN;
}

} // namespace

UIScreen::UIScreen(UI &ui) : ui(ui), display(ui.get_display()) {}

const MainScreen::Choice MainScreen::choices[CHOICE_COUNT] = {
    {ST_PERC,  "Percussions"},
    {ST_PARAM, "Tuning"},
    {ST_MIXER, "Mixer"},
};

void MainScreen::onKey(KeyType key) {
    //
    switch (key) {
    case KT_UP: index++; break;
    case KT_DOWN: index--; break;
    case KT_PRESS:
        // transition to the screen represented by the first
        ui.set_screen(choices[index].screen);
        return;
    }

    // wraparound
    if (index < 0) index = CHOICE_COUNT - 1;
    if (index >= CHOICE_COUNT) index = 0;

    // schedule redraw
    mark_dirty();
}

void MainScreen::draw() {
    // display the drum name and all parameters
    uint8_t w = display.getWidth();
    uint8_t h = display.getHeight();

    display.clear();
    display.setFont(ArialMT_Plain_10);

    display.drawString(0, 0, "Main Menu");
    display.drawLine(0, 12, w, 12);

    for (unsigned id = 0; id < CHOICE_COUNT; ++id) {
        display.drawString(10, 15*(id+1), choices[id].text);
    }

    draw_cursor_horizonal(display, 3, 4 + 15*(index+1));

    display.display();
}


void PercussionScreen::onKey(KeyType key) {
    //
    switch (key) {
    case KT_UP: index++; break;
    case KT_DOWN: index--; break;
    case KT_BACK: ui.set_screen(ST_MAIN); break;
    case KT_PRESS:
        // transition to the parameter selection screen
        ParamScreen *ps =
            static_cast<ParamScreen *>(ui.get_screen(ST_PARAM));
        ps->set_percussion(index);
        ui.set_screen(ST_PARAM);
        return;
    }

    // wraparound
    int pc = ui.get_drummer().percussion_count();
    if (index < 0) index = pc - 1;
    if (index >= pc) index = 0;

    // schedule redraw
    mark_dirty();
}

void PercussionScreen::draw() {
    // display the drum name and all parameters
    uint8_t w = display.getWidth();
    uint8_t h = display.getHeight();

    const char *drum_name = ui.get_drummer().percussion_name(index);

    display.clear();
    display.setFont(ArialMT_Plain_10);

    if (drum_name) display.drawString(0, 0, drum_name);
    display.drawLine(0, 12, w, 12);

    // TODO: get all drum params and render them
    // TODO: With two rotencoders we could directly influnence the parameters
    peaks::Configurable *percussion = ui.get_drummer().get_percussion(index);
    unsigned pc = percussion->param_count();

    uint16_t params[peaks::PARAM_MAX];
    percussion->params_fetch_current(params);

    for (unsigned id = 0; id < pc; ++id) {
        draw_gauge(display, 5, 25 + id * 7, 118, 5, params[id]);
    }

    display.display();
}


void ParamScreen::set_percussion(int idx) {
    perc_index = idx;
    percussion = ui.get_drummer().get_percussion(perc_index);
    name = ui.get_drummer().percussion_name(perc_index);
    index = 0;
}

void ParamScreen::prev_percussion() {
    int pc = ui.get_drummer().percussion_count();

    if (--perc_index < 0) perc_index = pc - 1;

    set_percussion(perc_index);
}

void ParamScreen::next_percussion() {
    int pc = ui.get_drummer().percussion_count();
    if (++perc_index >= pc) perc_index = 0;
    set_percussion(perc_index);
}

void ParamScreen::onKey(KeyType key) {
    //
    int incr = 0;
    switch (key) {
    case KT_UP: incr = 1; break;
    case KT_DOWN: incr = -1; break;
    case KT_BACK:
        if (set_mode) {
            set_mode = false;
            mark_dirty();
            return;
        }
        ui.set_screen(ST_PERC);
        break;
    case KT_PRESS:
        set_mode = !set_mode;
        break;
    }

    if (set_mode) {
        uint16_t params[peaks::PARAM_MAX];
        percussion->params_fetch_current(params);
        if (incr > 0) safe_incr(params[index]);
        if (incr < 0) safe_decr(params[index]);
        percussion->params_set(params);
    } else {
        index += incr;
        if (index < 0) {
            prev_percussion();
            index = percussion->param_count() - 1;
        } else if (index >= percussion->param_count()) {
            next_percussion();
            index = 0;
        }
    }

    // schedule redraw
    mark_dirty();
}

void ParamScreen::draw() {
    // display the drum name and all parameters
    uint8_t w = display.getWidth();
    uint8_t h = display.getHeight();

    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, name);
    display.drawLine(0, 12, w, 12);
    display.drawString(5, 20, percussion->param_name(index));

    // render the parameter value
    uint16_t params[peaks::PARAM_MAX];
    percussion->params_fetch_current(params);

    draw_gauge(display, 5, 35, 118, 10, params[index], set_mode);

    char str_val[8];
    itoa(params[index], str_val, 10);
    display.drawString(5, 50, str_val);

    itoa(int(100) * params[index] / 65535, str_val, 10);
    int sl = strlen(str_val);
    str_val[sl] = '%';
    str_val[sl + 1] = 0;

    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 50, str_val);
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    display.display();
}

MixerScreen::MixerScreen(UI &ui)
    : UIScreen(ui), idx_max(Mixer::get_channel_count() * SUBCHOICE_COUNT) {}

void MixerScreen::onKey(KeyType key) {
    int incr = 0;

    switch (key) {
    case KT_UP: incr = 1; break;
    case KT_DOWN: incr = -1; break;
    case KT_BACK:
        if (set_mode) {
            set_mode = false;
            mark_dirty();
            return;
        }

        ui.set_screen(ST_MAIN);
        break;
    case KT_PRESS:
        set_mode = !set_mode;
        mark_dirty();
        return;
    }

    if (set_mode) {
        // modify current value
        modify_current_setting(incr);
    } else {
        index += incr;

        // wraparound
        if (index < 0) index = idx_max - 1;
        if (index >= idx_max) index = 0;
    }

    // schedule redraw
    mark_dirty();
}

void MixerScreen::modify_current_setting(int increment) {
    unsigned channel = index / SUBCHOICE_COUNT;
    unsigned sub_choice = index % SUBCHOICE_COUNT;

    Mixer::ChannelSettings &chs =
        ui.get_drummer().get_mixer().get_channel_settings(
            (Mixer::Channel)channel);

    switch (sub_choice) {
    case 0:
        if (increment > 0) safe_incr(chs.volume);
        if (increment < 0) safe_decr(chs.volume);
        break;
    case 1:
        if (increment > 0) safe_incr(chs.panning);
        if (increment < 0) safe_decr(chs.panning);
        break;
    }
}

void MixerScreen::draw() {
    // display the drum name and all parameters
    uint8_t w = display.getWidth();
    uint8_t h = display.getHeight();

    display.clear();
    display.setFont(ArialMT_Plain_10);

    display.drawString(0, 0, "Mixer");
    display.drawLine(0, 12, w, 12);

    unsigned channel = index / SUBCHOICE_COUNT;
    unsigned sub_choice = index % SUBCHOICE_COUNT;
    auto chan = (Mixer::Channel)channel;
    Mixer::ChannelSettings &chs =
        ui.get_drummer().get_mixer().get_channel_settings(
            chan);

    // add channel name to status line
    display.drawString(64, 0, Mixer::get_channel_name(chan));

    if (!set_mode) draw_cursor_horizonal(display, 3, 19 + sub_choice * 15);

    // render the settings for current channel
    display.drawString(10, 15, "Volume");
    draw_gauge(display, 64, 17, 60, 8, chs.volume, set_mode && sub_choice == 0);
    display.drawString(10, 30, "Panning");
    draw_gauge(display, 64, 32, 60, 8, chs.panning, set_mode && sub_choice == 1);


    display.display();
}
