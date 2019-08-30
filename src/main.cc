#include <Arduino.h>
#include <driver/i2s.h>
#include <HardwareSerial.h>
#include <MIDI.h>
#include <SH1106Spi.h>

#include "peaks-drums.h"
#include "ui.h"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, midi1);

peaks::BassDrum bass;
peaks::SnareDrum snare;
peaks::HighHat high_hat;

#define KEY_TRIGGER_PIN    13
#define S1_TRIGGER_PIN     12
#define S2_TRIGGER_PIN     14


// SPI OLED display.
#define OLED_RST_PIN 4
#define OLED_DC_PIN  2

UI ui(KEY_TRIGGER_PIN, S1_TRIGGER_PIN, S2_TRIGGER_PIN, OLED_RST_PIN, OLED_DC_PIN);

peaks::ControlBitMask bass_trigger;
peaks::ControlBitMask snare_trigger;
peaks::ControlBitMask high_hat_trigger;

bool bass_is_accented = false;
bool snare_is_accented = false;
bool high_hat_is_accented = false;

//i2s configuration
int i2s_num = 0; // i2s port number
i2s_config_t i2s_config = {
     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
     .sample_rate = 41100,
     .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
     .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB),
     .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
     .dma_buf_count = 8,
     .dma_buf_len = 64  //Interrupt level 1
    };

i2s_pin_config_t pin_config = {
    .bck_io_num = 26, //this is BCK pin
    .ws_io_num = 25, // this is LRCK pin
    .data_out_num = 22, // this is DATA output pin
    .data_in_num = -1   //Not used
};

void setup_drums()
{
    bass.Init();
    snare.Init();
    high_hat.Init();
}

constexpr unsigned PERCUSSION_CHANNEL = 10;
constexpr unsigned BASS_DRUM1 = 36;
constexpr unsigned ELECTRIC_SNARE = 40;
constexpr unsigned CLOSED_HIHAT = 42;
constexpr unsigned OPEN_HIHAT = 46;

constexpr byte ACCENT_THRESHOLD = 110;

void handleNoteOn(byte inChannel, byte inNote, byte inVelocity)
{
    switch (inNote) {
    case BASS_DRUM1:
        bass_is_accented = inVelocity > ACCENT_THRESHOLD ? true : false;
        bass_trigger = peaks::CONTROL_GATE_RISING; break;
    case ELECTRIC_SNARE:
        snare_is_accented = inVelocity > ACCENT_THRESHOLD ? true : false;
        snare_trigger = peaks::CONTROL_GATE_RISING; break;
    case CLOSED_HIHAT:
        high_hat_is_accented = inVelocity > ACCENT_THRESHOLD ? true : false;
        high_hat.set_open(false);
        high_hat_trigger = peaks::CONTROL_GATE_RISING;
        break;
    case OPEN_HIHAT:
        high_hat_is_accented = inVelocity > ACCENT_THRESHOLD ? true : false;
        high_hat.set_open(true);
        high_hat_trigger = peaks::CONTROL_GATE_RISING;
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

    setup_drums();

    midi1.setHandleNoteOn(handleNoteOn);
    midi1.setHandleNoteOff(handleNoteOff);
    // TODO: setHandleControlChange for midi mapped drum param settings
    midi1.begin(10); // we're drums, we're at channel 10

    //initialize i2s with configurations above
    i2s_driver_install((i2s_port_t)i2s_num, &i2s_config, 0, NULL);
    i2s_set_pin((i2s_port_t)i2s_num, &pin_config);
}

void next_sample(int16_t *left_sample, int16_t *right_sample)
{
    int32_t b = bass.ProcessSingleSample(bass_trigger) >> 2;
    int32_t s = snare.ProcessSingleSample(snare_trigger) >> 2;
    int32_t h = high_hat.ProcessSingleSample(high_hat_trigger) >> 2;

    if (bass_is_accented)
        b <<= 1;
    if (snare_is_accented)
        s <<= 1;
    if (high_hat_is_accented)
        h <<= 1;

    bass_trigger = peaks::CONTROL_GATE;
    snare_trigger = peaks::CONTROL_GATE;
    high_hat_trigger = peaks::CONTROL_GATE;

    *left_sample = peaks::CLIP(b + s + h);
    *right_sample = peaks::CLIP(b + s + h);
}

/* write sample data to I2S */
bool i2s_write_sample_nb(uint32_t sample) {
/*    size_t wtn = 0;
    i2s_write((i2s_port_t)i2s_num, &sample, sizeof(uint32_t), &wtn, 100);
    return wtn == sizeof(uint32_t);*/
    return i2s_write_bytes((i2s_port_t)i2s_num, &sample, sizeof(uint32_t), 100);
}

int16_t left, right;

// feed a few samples
void feed_i2s() {
    // 16bit per sample, stereo
    for (int ctr = 0; ctr < 64; ++ctr) {
        if (!i2s_write_sample_nb(right << 16 | left)) break;
        next_sample(&left, &right);
    }
}

void loop()
{
    midi1.read();
    ui.update();
    feed_i2s();
}
