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
    static constexpr uint16_t VOL_MAX = (1 << 9) - 1;

    void set_velocity(Channel chan, byte vel) {
        channels[chan].velocity = vel > 127 ? 127 : vel;
    }

    void set_volume(Channel chan, uint16_t vol) {
        channels[chan].volume = vol > VOL_MAX ? VOL_MAX : vol;
    }

    void set_panning(Channel chan, int16_t pan) {
        channels[chan].panning = pan;
    }

    // sample accessor
    int16_t &operator[](Channel chan) {
        return channels[chan].sample;
    }

    void mix(int16_t *left, int16_t *right) {
        int32_t lt = 0, rt = 0;

        for (unsigned chan = 0; chan < CHANNEL_MAX; ++chan) {
            // first we mix up the final sample volume
            int32_t mixed = ((channels[chan].velocity * channels[chan].volume) *
                             channels[chan].sample) >>
                            16;
            // now we pan it left/right
            int32_t pan = channels[chan].panning + 32768;
            lt += mixed * pan >> 16;
            rt += mixed * (65535 - pan) >> 16;
        }

        *left  = peaks::CLIP(lt);
        *right = peaks::CLIP(rt);
    }

protected:
    struct ChannelSettings {
        byte velocity   = 120;   // 0-127 - ie. 7 bit
        uint16_t volume = VOL_MAX >> 3; // 9 bit volume, we shift to avoid clipping
        int16_t panning = 0; // panning -32767 is left

        // current sample value
        int16_t sample;
    };

    ChannelSettings channels[CHANNEL_MAX];
};
