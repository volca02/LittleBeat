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

constexpr uint16_t ROTENCODER_STEP = 1024;

/// safely increments the value by N steps
void safe_incr(uint16_t &tgt, uint16_t steps = ROTENCODER_STEP) {
    if (tgt < (0xFFFF-steps+1)) tgt+=steps;
}

void safe_decr(uint16_t &tgt, uint16_t steps = ROTENCODER_STEP) {
    if (tgt > steps) tgt-=steps;
}

} // namespace

UIScreen::UIScreen(UI &ui) : ui(ui), display(ui.get_display()) {}

void PercussionScreen::onKey(KeyType key) {
    //
    switch (key) {
    case KT_UP: index++; break;
    case KT_DOWN: index--; break;
    case KT_PRESS:
        // transition to the parameter selection screen
        ParamScreen *ps =
            static_cast<ParamScreen *>(ui.get_screen(UI::ST_PARAM));
        const char *drum_name = ui.get_drummer().percussion_name(index);
        ps->set_percussion(index, drum_name);
        ui.set_screen(UI::ST_PARAM);
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
    if (drum_name) display.drawString(5, 5, drum_name);

    // TODO: get all drum params and render them
    // TODO: With two rotencoders we could directly influnence the parameters
    peaks::Configurable *percussion = ui.get_drummer().get_percussion(index);
    unsigned pc = percussion->param_count();

    uint16_t params[peaks::PARAM_MAX];
    percussion->params_fetch_current(params);

    for (unsigned id = 0; id < pc; ++id) {
        draw_gauge(display, 5, 25 + id * 7, 100, 5, params[id]);
    }

    display.display();
}


void ParamScreen::set_percussion(int idx, const char *nm) {
    percussion = ui.get_drummer().get_percussion(idx);
    name = nm;
    index = 0;
}

void ParamScreen::onKey(KeyType key) {
    //
    int incr = 0;
    switch (key) {
    case KT_UP: incr = 1; break;
    case KT_DOWN: incr = -1; break;
    case KT_BACK:
        ui.set_screen(UI::ST_PERC);
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
        // wraparound
        index += incr;
        int pc = percussion->param_count();
        if (index < 0) index = pc - 1;
        if (index >= pc) index = 0;
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
    display.drawString(5, 5, name);
    display.drawString(5, 20, percussion->param_name(index));

    // render the parameter value
    uint16_t params[peaks::PARAM_MAX];
    percussion->params_fetch_current(params);

    draw_gauge(display, 5, 35, 100, 10, params[index], set_mode);

    char str_val[8];
    itoa(params[index], str_val, 10);
    display.drawString(5, 50, str_val);

    itoa(int(100) * params[index] / 65535, str_val, 10);
    int sl = strlen(str_val);
    str_val[sl] = '%';
    str_val[sl + 1] = 0;
    display.drawString(64, 50, str_val);

    display.display();
}
