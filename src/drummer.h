#include <driver/i2s.h>

#include "peaks-drums.h"
#include "mixer.h"

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
        KICK_DRUM,
        SNARE,
        HIHAT_CLOSED,
        HIHAT_OPEN,
        FM,
        CLAP
    };

    void init() {
        bass.Init();
        kick.Init();
        snare.Init();
        high_hat.Init();
        fm.Init();
        clap.Init();

        //initialize i2s with configurations above
        i2s_driver_install((i2s_port_t)i2s_num, &i2s_config, 0, NULL);
        i2s_set_pin((i2s_port_t)i2s_num, &pin_config);
    }

    void trigger(Percussion percussion, byte velocity) {
        switch (percussion) {
        case BASS_DRUM:
            bass_is_accented = accent(velocity);
            mixer.set_velocity(Mixer::BASS_DRUM, velocity);
            bass_trigger = peaks::CONTROL_GATE_RISING;
            break;
        case KICK_DRUM:
            kick_is_accented = accent(velocity);
            mixer.set_velocity(Mixer::KICK_DRUM, velocity);
            kick_trigger = peaks::CONTROL_GATE_RISING;
            break;
        case SNARE:
            snare_is_accented = accent(velocity);
            mixer.set_velocity(Mixer::SNARE, velocity);
            snare_trigger = peaks::CONTROL_GATE_RISING;
            break;
        case HIHAT_CLOSED:
            high_hat_is_accented = accent(velocity);
            mixer.set_velocity(Mixer::HI_HAT, velocity);
            high_hat.set_open(false);
            high_hat_trigger = peaks::CONTROL_GATE_RISING;
            break;
        case HIHAT_OPEN:
            high_hat_is_accented = accent(velocity);
            mixer.set_velocity(Mixer::HI_HAT, velocity);
            high_hat.set_open(true);
            high_hat_trigger = peaks::CONTROL_GATE_RISING;
            break;
        case FM:
            fm_is_accented = accent(velocity);
            mixer.set_velocity(Mixer::FM, velocity);
            fm_trigger = peaks::CONTROL_GATE_RISING;
            break;
        case CLAP:
            clap_is_accented = accent(velocity);
            mixer.set_velocity(Mixer::CLAP, velocity);
            clap_trigger = peaks::CONTROL_GATE_RISING;
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
        return 6;
    }

    /** returns name of the given percussion index */
    const char *percussion_name(unsigned idx) const {
        switch(idx) {
        case 0: return "Bass Drum";
        case 1: return "Kick Drum";
        case 2: return "Snare Drum";
        case 3: return "Hi-Hat";
        case 4: return "FM Drum";
        case 5: return "Clap";
        default: return nullptr;
        }
    }

    /** returns percussion index */
    peaks::Configurable *get_percussion(unsigned idx) {
        switch(idx) {
        case 0: return &bass;
        case 1: return &kick;
        case 2: return &snare;
        case 3: return &high_hat;
        case 4: return &fm;
        case 5: return &clap;
        default:
            return nullptr;
        }
    }

    Mixer &get_mixer() {
        return mixer;
    }

protected:
    void next_sample(int16_t *left_sample, int16_t *right_sample)
    {
        mixer[Mixer::BASS_DRUM] = bass.ProcessSingleSample(bass_trigger);
        mixer[Mixer::KICK_DRUM] = kick.ProcessSingleSample(kick_trigger);
        mixer[Mixer::SNARE]     = snare.ProcessSingleSample(snare_trigger);
        mixer[Mixer::HI_HAT]    = high_hat.ProcessSingleSample(high_hat_trigger);
        mixer[Mixer::FM]        = fm.ProcessSingleSample(fm_trigger);
        mixer[Mixer::CLAP]      = clap.ProcessSingleSample(clap_trigger);

        // reset triggers
        bass_trigger = peaks::CONTROL_GATE;
        kick_trigger = peaks::CONTROL_GATE;
        snare_trigger = peaks::CONTROL_GATE;
        high_hat_trigger = peaks::CONTROL_GATE;
        fm_trigger = peaks::CONTROL_GATE;
        clap_trigger = peaks::CONTROL_GATE;

        mixer.mix(left_sample, right_sample);
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
    bool kick_is_accented = false;
    bool snare_is_accented = false;
    bool high_hat_is_accented = false;
    bool fm_is_accented = false;
    bool clap_is_accented = false;

    peaks::ControlBitMask bass_trigger;
    peaks::ControlBitMask kick_trigger;
    peaks::ControlBitMask snare_trigger;
    peaks::ControlBitMask high_hat_trigger;
    peaks::ControlBitMask fm_trigger;
    peaks::ControlBitMask clap_trigger;

    peaks::BassDrum bass;
    peaks::KickDrum kick;
    peaks::SnareDrum snare;
    peaks::HighHat high_hat;
    peaks::FmDrum fm;
    peaks::Clap clap;

    // mixes the sounds
    Mixer mixer;
};
