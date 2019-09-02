#pragma once

#include <Arduino.h>

namespace peaks {

extern const uint16_t lut_svf_cutoff[];
extern const uint16_t lut_svf_damp[];
extern const uint16_t lut_env_expo[];
extern const int16_t wav_sine[];
extern const int16_t wav_overdrive[];
extern const uint32_t lut_oscillator_increments[];
extern const uint32_t lut_env_increments[];

// fm drum stuff
extern const uint16_t bd_map[10][4];
extern const uint16_t sd_map[10][4];

} // namespace peaks
