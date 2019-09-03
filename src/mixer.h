#pragma once

#include <Arduino.h>

#include "peaks-drums.h"

class Mixer {
public:
    enum Channel {
        BASS_DRUM = 0,
        SNARE,
        HI_HAT,
        FM,
        CLAP,

        CHANNEL_MAX // effectively mixer channel count
    };

    // this will definitely clip on more than one sound!
    static constexpr uint16_t VOL_MAX = 65535 >> 3;

    // configurable parameters
    struct ChannelSettings {
        uint16_t volume = VOL_MAX; // max volume, we shift to avoid clipping
        uint16_t panning = 32768; // panning, 32768 is center
    };

    // values set by the playback, not directly configurable
    struct ChannelStatus {
        byte velocity   = 120;  // 0-127 - ie. 7 bit
        int16_t sample;         // current sample value
    };

    void set_volume(Channel chan, uint16_t vol) {
        settings[chan].volume = vol > VOL_MAX ? VOL_MAX : vol;
    }

    void set_panning(Channel chan, int16_t pan) {
        settings[chan].panning = pan;
    }

    static unsigned get_channel_count() {
        return CHANNEL_MAX;
    }

    static const char *get_channel_name(Channel arg) {
        switch (arg) {
        case BASS_DRUM: return "Bass Drum";
        case SNARE: return "Snare Drum";
        case HI_HAT: return "Hi-Hat";
        case FM: return "FM Drum";
        case CLAP: return "Clap";
        default: return "?";
        }
    }

    ChannelSettings &get_channel_settings(Channel chan) {
        return settings[chan];
    }

    // --- code below is solely used by the playback code ---

    // sets the current note's velocity
    void set_velocity(Channel chan, byte vel) {
        status[chan].velocity = vel > 127 ? 127 : vel;
    }

    // sample value accessor
    int16_t &operator[](Channel chan) {
        return status[chan].sample;
    }

    // mixes all samples according to settings and values
    void mix(int16_t *left, int16_t *right) {
        int32_t lt = 0, rt = 0;

        for (unsigned chan = 0; chan < CHANNEL_MAX; ++chan) {
            // first we mix up the final sample volume
            int32_t mixed =
                ( (  status[chan].velocity
                   * (settings[chan].volume >> 7)) * status[chan].sample
                ) >> 16;
            // now we pan it left/right
            int32_t pan = settings[chan].panning;
            lt += mixed * pan >> 16;
            rt += mixed * (65535 - pan) >> 16;
        }

        *left  = peaks::CLIP(lt);
        *right = peaks::CLIP(rt);
    }


protected:

    ChannelSettings settings[CHANNEL_MAX];
    ChannelStatus   status[CHANNEL_MAX];
};
