#include <Arduino.h>
#include <HardwareSerial.h>
#include <MIDI.h>

#include "drummer.h"
#include "ui.h"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, midi1);

#define KEY_TRIGGER_PIN    13
#define S1_TRIGGER_PIN     12
#define S2_TRIGGER_PIN     14
#define BACK_TRIGGER_PIN   33

// SPI OLED display.
#define OLED_RST_PIN 4
#define OLED_DC_PIN  2

Drummer drummer;
UI ui(drummer, KEY_TRIGGER_PIN, S1_TRIGGER_PIN, S2_TRIGGER_PIN,
      BACK_TRIGGER_PIN, OLED_RST_PIN, OLED_DC_PIN);

void setup_drums()
{
    drummer.init();
}

constexpr unsigned PERCUSSION_CHANNEL = 10;

constexpr unsigned BASS_DRUM1 = 36;
constexpr unsigned ACCOUSTIC_SNARE = 38;
constexpr unsigned HAND_CLAP = 39;
constexpr unsigned ELECTRIC_SNARE = 40;
constexpr unsigned CLOSED_HIHAT = 42;
constexpr unsigned OPEN_HIHAT = 46;

void handleNoteOn(byte inChannel, byte inNote, byte inVelocity)
{
    switch (inNote) {
    case BASS_DRUM1:
        drummer.trigger(Drummer::BASS_DRUM, inVelocity);
        return;
    case ACCOUSTIC_SNARE:
        drummer.trigger(Drummer::SNARE, inVelocity);
        return;
    case ELECTRIC_SNARE:
        drummer.trigger(Drummer::FM, inVelocity);
        break;
    case CLOSED_HIHAT:
        drummer.trigger(Drummer::HIHAT_CLOSED, inVelocity);
        break;
    case OPEN_HIHAT:
        drummer.trigger(Drummer::HIHAT_OPEN, inVelocity);
        break;
    }
}

void handleNoteOff(byte inChannel, byte inNote, byte inVelocity)
{
    // nothing... we let the drum play for as long as needed - trigger only
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Init");

    ui.init();

    drummer.init();

    // Init the midi bindings.
    midi1.setHandleNoteOn(handleNoteOn);
    midi1.setHandleNoteOff(handleNoteOff);
    // TODO: setHandleControlChange for midi mapped drum param settings
    midi1.begin(10); // we're drums, we're at channel 10
}

void loop()
{
    midi1.read();
    drummer.update();
    ui.update();
}
