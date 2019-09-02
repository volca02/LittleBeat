#include <driver/i2s.h>

#include "peaks-drums.h"

constexpr byte ACCENT_THRESHOLD = 110;

//i2s configuration
constexpr int i2s_num = 0; // i2s port number
extern i2s_config_t i2s_config;
extern i2s_pin_config_t pin_config;

class Drummer {
public:
    // Percussion ID for triggering
    enum Percussion {
        BASS_DRUM = 0,
        SNARE,
        HIHAT_CLOSED,
        HIHAT_OPEN,
        FM
    };

    void init() {
        bass.Init();
        snare.Init();
        high_hat.Init();
        fm.Init();

        //initialize i2s with configurations above
        i2s_driver_install((i2s_port_t)i2s_num, &i2s_config, 0, NULL);
        i2s_set_pin((i2s_port_t)i2s_num, &pin_config);
    }

    void trigger(Percussion percussion, byte velocity) {
        switch (percussion) {
        case BASS_DRUM:
            bass_is_accented = accent(velocity);
            bass_trigger = peaks::CONTROL_GATE_RISING; break;
        case SNARE:
            snare_is_accented = accent(velocity);
            snare_trigger = peaks::CONTROL_GATE_RISING; break;
        case HIHAT_CLOSED:
            high_hat_is_accented = accent(velocity);
            high_hat.set_open(false);
            high_hat_trigger = peaks::CONTROL_GATE_RISING;
            break;
        case HIHAT_OPEN:
            high_hat_is_accented = accent(velocity);
            high_hat.set_open(true);
            high_hat_trigger = peaks::CONTROL_GATE_RISING;
            break;
        case FM:
            fm_is_accented = accent(velocity);
            fm_trigger = peaks::CONTROL_GATE_RISING;
            break;
        }
    }

    bool accent(byte velocity) const {
        return velocity > ACCENT_THRESHOLD;
    }

    void update() {
        feed_i2s();
    }

    /** percussion sound counter. Some are deduplicated (i.e. hihat) */
    unsigned percussion_count() const {
        return 4;
    }

    /** returns name of the given percussion index */
    const char *percussion_name(unsigned idx) const {
        switch(idx) {
        case 0: return "Bass Drum";
        case 1: return "Snare Drum";
        case 2: return "Hi-Hat";
        case 3: return "FM Drum";
        default: return nullptr;
        }
    }

    /** returns percussion index */
    peaks::Configurable *get_percussion(unsigned idx) {
        switch(idx) {
        case 0: return &bass;
        case 1: return &snare;
        case 2: return &high_hat;
        case 3: return &fm;
        default:
            return nullptr;
        }
    }

protected:
    void next_sample(int16_t *left_sample, int16_t *right_sample)
    {
        int32_t b = bass.ProcessSingleSample(bass_trigger) >> 2;
        int32_t s = snare.ProcessSingleSample(snare_trigger) >> 2;
        int32_t h = high_hat.ProcessSingleSample(high_hat_trigger) >> 2;
        int32_t f = fm.ProcessSingleSample(fm_trigger) >> 2;

        if (bass_is_accented) b <<= 1;
        if (snare_is_accented) s <<= 1;
        if (high_hat_is_accented) h <<= 1;
        if (fm_is_accented) f <<= 1;

        bass_trigger = peaks::CONTROL_GATE;
        snare_trigger = peaks::CONTROL_GATE;
        high_hat_trigger = peaks::CONTROL_GATE;
        fm_trigger = peaks::CONTROL_GATE;

        *left_sample = peaks::CLIP(b + s + h + f);
        *right_sample = peaks::CLIP(b + s + h + f);
    }

    /* write sample data to I2S */
    bool i2s_write_sample_nb(uint32_t sample) {
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

    bool bass_is_accented = false;
    bool snare_is_accented = false;
    bool high_hat_is_accented = false;
    bool fm_is_accented = false;

    peaks::ControlBitMask bass_trigger;
    peaks::ControlBitMask snare_trigger;
    peaks::ControlBitMask high_hat_trigger;
    peaks::ControlBitMask fm_trigger;

    peaks::BassDrum bass;
    peaks::SnareDrum snare;
    peaks::HighHat high_hat;
    peaks::FmDrum fm;
};
