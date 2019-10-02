#pragma once

#include <Arduino.h>

#include "lut.h"

namespace peaks {

// Note: Have to change this if there ever comes a percussion with more
// parameters
const uint16_t PARAM_MAX = 6;

// Code in this namespace is largerly based on Peaks source code.
// https://github.com/pichenettes/eurorack

const uint16_t kAdcThresholdUnlocked = 1 << (16 - 10); // 10 bits
const uint16_t kAdcThresholdLocked = 1 << (16 - 8);    // 8 bits

enum ControlBitMask {
    CONTROL_GATE = 1,
    CONTROL_GATE_RISING = 2,
    CONTROL_GATE_FALLING = 4,

    CONTROL_GATE_AUXILIARY = 16,
    CONTROL_GATE_RISING_AUXILIARY = 32,
    CONTROL_GATE_FALLING_AUXILIARY = 64
};

enum ControlMode { CONTROL_MODE_FULL, CONTROL_MODE_HALF };

const uint16_t kHighestNote = 128 * 128;
const uint16_t kPitchTableStart = 116 * 128;
const uint16_t kOctave = 128 * 12;

static inline int32_t CLIP(int32_t sample) {
    if (sample < -32768)
        return -32768;
    if (sample > +32767)
        return +32767;
    return sample;
}

inline int16_t Interpolate824(const int16_t *table, uint32_t phase) {
    int32_t a = table[phase >> 24];
    int32_t b = table[(phase >> 24) + 1];
    return a + ((b - a) * static_cast<int32_t>((phase >> 8) & 0xffff) >> 16);
}

inline uint16_t Interpolate824(const uint16_t *table, uint32_t phase) {
    uint32_t a = table[phase >> 24];
    uint32_t b = table[(phase >> 24) + 1];
    return a + ((b - a) * static_cast<uint32_t>((phase >> 8) & 0xffff) >> 16);
}

inline int16_t Interpolate824(const uint8_t *table, uint32_t phase) {
    int32_t a = table[phase >> 24];
    int32_t b = table[(phase >> 24) + 1];
    return (a << 8) + ((b - a) * static_cast<int32_t>(phase & 0xffffff) >> 16) -
           32768;
}

inline int16_t Interpolate1022(const int16_t* table, uint32_t phase) {
  int32_t a = table[phase >> 22];
  int32_t b = table[(phase >> 22) + 1];
  return a + ((b - a) * static_cast<int32_t>((phase >> 6) & 0xffff) >> 16);
}

inline int16_t Mix(int16_t a, int16_t b, uint16_t balance) {
  return (a * (65535 - balance) + b * balance) >> 16;
}

inline uint16_t Mix(uint16_t a, uint16_t b, uint16_t balance) {
  return (a * (65535 - balance) + b * balance) >> 16;
}

inline uint32_t ComputePhaseIncrement(int16_t midi_pitch) {
    if (midi_pitch >= kHighestNote) {
        midi_pitch = kHighestNote - 1;
    }

    int32_t ref_pitch = midi_pitch;
    ref_pitch -= kPitchTableStart;

    size_t num_shifts = 0;
    while (ref_pitch < 0) {
        ref_pitch += kOctave;
        ++num_shifts;
    }

    uint32_t a = lut_oscillator_increments[ref_pitch >> 4];
    uint32_t b = lut_oscillator_increments[(ref_pitch >> 4) + 1];
    uint32_t phase_increment = a + \
                               (static_cast<int32_t>(b - a) * (ref_pitch & 0xf) >> 4);
    phase_increment >>= num_shifts;
    return phase_increment;
}

class Excitation {
public:
    Excitation() {}
    ~Excitation() {}

    void Init() {
        delay_ = 0;
        decay_ = 4093;
        counter_ = 0;
        state_ = 0;
    }

    void set_delay(uint16_t delay) { delay_ = delay; }

    void set_decay(uint16_t decay) { decay_ = decay; }

    void Trigger(int32_t level) {
        level_   = level;
        counter_ = delay_ + 1;
        // this breaks the continuity of things, but without it bass drum
        // repetitions break
        state_   = 0;
    }

    // done - as observed by the counter
    bool done() { return counter_ == 0; }

    // finished - as in delay passed and excitation went to zero
    bool finished() {
        return state_ == 0 && counter_ == 0;
    }

    inline int32_t Process() {
        state_ = (state_ * decay_ >> 12);
        if (counter_ > 0) {
            --counter_;
            if (counter_ == 0) {
                state_ += level_ < 0 ? -level_ : level_;
            }
        }
        return level_ < 0 ? -state_ : state_;
    }

private:
    uint32_t delay_;
    uint32_t decay_;
    int32_t counter_;
    int32_t state_;
    int32_t level_;
};

// repeated excitation. N counts, then longer decay
class Repeater {
public:
    Repeater() {}
    ~Repeater() {}

    void Init() {
        decay_ = 3340;
        decay_term_ = 4095;

        ex_.Init();
        ex_.set_delay(0);
        ex_.set_decay(decay_);
    }

    void Trigger(int32_t level) {
        rep_counter_ = 0;
        level_       = level;
        ex_.set_decay(decay_);
        ex_.Trigger(level_);
    }

    inline int32_t Process() {
        // TODO: transition to Decay-Only repeat (via exc.finished())
        int32_t exc = ex_.Process();
        if (ex_.finished()) {
            ++rep_counter_;
            if (rep_counter_ == repeats_) {
                ex_.set_decay(decay_term_);
                ex_.Trigger(level_);
            } else if (rep_counter_ < repeats_) {
                ex_.Trigger(level_);
            }
        }

        return exc;
    }

    void set_repeats(uint32_t repeats) { repeats_ = repeats; }
    void set_decay(uint32_t decay) { decay_ = decay; }
    void set_decay_term(uint32_t decay) { decay_term_ = decay; }

private:
    Excitation ex_;

    uint32_t level_;  // level for re-trigger
    int32_t rep_counter_; // counts down the initial pulses (repeats_ to zero)
    uint32_t decay_;
    uint32_t decay_term_;
    uint32_t repeats_;
};

enum SvfMode { SVF_MODE_LP, SVF_MODE_BP, SVF_MODE_HP };

class Svf {
public:
    Svf() {}
    ~Svf() {}

    void Init() {
        lp_ = 0;
        bp_ = 0;
        frequency_ = 33 << 7;
        resonance_ = 16384;
        dirty_ = true;
        punch_ = 0;
        mode_ = SVF_MODE_BP;
    }

    void set_frequency(int16_t frequency) {
        dirty_ = dirty_ || (frequency_ != frequency);
        frequency_ = frequency;
    }

    void set_resonance(int16_t resonance) {
        resonance_ = resonance;
        dirty_ = true;
    }

    void set_punch(uint16_t punch) {
        punch_ = (static_cast<uint32_t>(punch) * punch) >> 24;
    }

    void set_mode(SvfMode mode) { mode_ = mode; }

    int32_t Process(int32_t in) {
        if (dirty_) {
            f_ = Interpolate824(lut_svf_cutoff, frequency_ << 17);
            damp_ = Interpolate824(lut_svf_damp, resonance_ << 17);
            dirty_ = false;
        }
        int32_t f = f_;
        int32_t damp = damp_;
        if (punch_) {
            int32_t punch_signal = lp_ > 4096 ? lp_ : 2048;
            f += ((punch_signal >> 4) * punch_) >> 9;
            damp += ((punch_signal - 2048) >> 3);
        }
        int32_t notch = in - (bp_ * damp >> 15);
        lp_ += f * bp_ >> 15;
        lp_ = CLIP(lp_);
        int32_t hp = notch - lp_;
        bp_ += f * hp >> 15;
        bp_ = CLIP(bp_);

        return mode_ == SVF_MODE_BP ? bp_ : (mode_ == SVF_MODE_HP ? hp : lp_);
    }

private:
    bool dirty_;

    int16_t frequency_;
    int16_t resonance_;

    int32_t punch_;
    int32_t f_;
    int32_t damp_;

    int32_t lp_;
    int32_t bp_;

    SvfMode mode_;
};

class Random {
public:
    static inline uint32_t state() { return rng_state_; }

    static inline void Seed(uint16_t seed) { rng_state_ = seed; }

    static inline uint32_t GetWord() {
        rng_state_ = rng_state_ * 1664525L + 1013904223L;
        return state();
    }

    static inline int16_t GetSample() {
        return static_cast<int16_t>(GetWord() >> 16);
    }

    static inline float GetFloat() {
        return static_cast<float>(GetWord()) / 4294967296.0f;
    }

private:
    static uint32_t rng_state_;
};

// ancestor to configurable objects - exposes param names, default values and current values
class Configurable {
public:
    virtual unsigned param_count() const = 0;

    // name getter for specified index
    virtual const char *param_name(unsigned idx) const = 0;

    // fills given array with current parameter values
    virtual void params_fetch_current(uint16_t *tgt) const = 0;

    // fills given array with default parameter values
    virtual void params_fetch_default(uint16_t *tgt) const = 0;

    // sets new parameter values
    virtual void params_set(uint16_t *params) = 0;
};

class BassDrum : public Configurable {
public:
    BassDrum() {}
    ~BassDrum() {}

    constexpr static int16_t  DEFAULT_FREQUENCY = 0;
    constexpr static uint16_t DEFAULT_DECAY     = 32768;
    constexpr static uint16_t DEFAULT_TONE      = 32768;
    constexpr static uint16_t DEFAULT_PUNCH     = 65535;

    void Init() {
        pulse_up_.Init();
        pulse_down_.Init();
        attack_fm_.Init();
        resonator_.Init();

        pulse_up_.set_delay(0);
        pulse_up_.set_decay(3340);

        pulse_down_.set_delay(1.0e-3 * 48000);
        pulse_down_.set_decay(3072);

        attack_fm_.set_delay(4.0e-3 * 48000);
        attack_fm_.set_decay(4093);

        resonator_.set_punch(32768);
        resonator_.set_mode(SVF_MODE_BP);

        set_frequency(DEFAULT_FREQUENCY);
        set_decay(DEFAULT_DECAY);
        set_tone(DEFAULT_TONE);
        set_punch(DEFAULT_PUNCH);

        lp_state_ = 0;
    }

    int16_t ProcessSingleSample(uint8_t control) {
        if (control & CONTROL_GATE_RISING) {
            pulse_up_.Trigger(12 * 32768 * 0.7);
            pulse_down_.Trigger(-19662 * 0.7);
            attack_fm_.Trigger(18000);
        }
        int32_t excitation = 0;
        excitation += pulse_up_.Process();
        excitation += !pulse_down_.done() ? 16384 : 0;
        excitation += pulse_down_.Process();
        attack_fm_.Process();
        resonator_.set_frequency(frequency_ +
                                 (attack_fm_.done() ? 0 : 17 << 7));

        int32_t resonator_output =
            (excitation >> 4) + resonator_.Process(excitation);
        lp_state_ += (resonator_output - lp_state_) * lp_coefficient_ >> 15;
        int32_t output = lp_state_;
        output = CLIP(output);
        return output;
    }

    /// configurable interface:
    unsigned param_count() const override { return 4; }
    const char *param_name(unsigned arg) const override {
        switch (arg) {
        case 0: return "Frequency";
        case 1: return "Punch";
        case 2: return "Tone";
        case 3: return "Decay";
        default: return "?";
        }
    }

    void params_fetch_current(uint16_t *tgt) const override {
        tgt[0] = freq_param + 32768;
        tgt[1] = punch_param;
        tgt[2] = tone_param;
        tgt[3] = decay_param;
    }

    void params_fetch_default(uint16_t *tgt) const override {
        tgt[0] = DEFAULT_FREQUENCY;
        tgt[1] = DEFAULT_PUNCH;
        tgt[2] = DEFAULT_TONE;
        tgt[3] = DEFAULT_DECAY;
    }

    void params_set(uint16_t *params) override {
        set_frequency(params[0] - 32768);
        set_punch(params[1]);
        set_tone(params[2]);
        set_decay(params[3]);
    }

    void set_frequency(int16_t frequency) {
        freq_param = frequency;
        frequency_ = (31 << 7) + (static_cast<int32_t>(frequency) * 896 >> 15);
    }

    void set_decay(uint16_t decay) {
        decay_param = decay;
        uint32_t scaled;
        uint32_t squared;
        scaled = 65535 - decay;
        squared = scaled * scaled >> 16;
        scaled = squared * scaled >> 18;
        resonator_.set_resonance(32768 - 128 - scaled);
    }

    void set_tone(uint16_t tone) {
        tone_param = tone;
        uint32_t coefficient = tone;
        coefficient = coefficient * coefficient >> 16;
        lp_coefficient_ = 512 + (coefficient >> 2) * 3;
    }

    void set_punch(uint16_t punch) {
        punch_param = punch;
        resonator_.set_punch(punch * punch >> 16);
    }

private:
    Excitation pulse_up_;
    Excitation pulse_down_;
    Excitation attack_fm_;
    Svf resonator_;

    int16_t freq_param;
    uint16_t punch_param;
    uint16_t tone_param;
    uint16_t decay_param;

    int32_t frequency_;
    int32_t lp_coefficient_;
    int32_t lp_state_;
};

class SnareDrum : public Configurable {
  public:
    SnareDrum() {}
    ~SnareDrum() {}

    constexpr static const uint16_t DEFAULT_TONE      = 0;
    constexpr static const uint16_t DEFAULT_SNAPPY    = 32768;
    constexpr static const uint16_t DEFAULT_DECAY     = 32768;
    constexpr static const uint16_t DEFAULT_FREQUENCY = 0;

    void Init() {
        excitation_1_up_.Init();
        excitation_1_up_.set_delay(0);
        excitation_1_up_.set_decay(1536);

        excitation_1_down_.Init();
        excitation_1_down_.set_delay(1e-3 * 48000);
        excitation_1_down_.set_decay(3072);

        excitation_2_.Init();
        excitation_2_.set_delay(1e-3 * 48000);
        excitation_2_.set_decay(1200);

        excitation_noise_.Init();
        excitation_noise_.set_delay(0);

        body_1_.Init();
        body_2_.Init();

        noise_.Init();
        noise_.set_resonance(2000);
        noise_.set_mode(SVF_MODE_BP);

        set_tone(DEFAULT_TONE);
        set_snappy(DEFAULT_SNAPPY);
        set_decay(DEFAULT_DECAY);
        set_frequency(DEFAULT_FREQUENCY);
    }

    int16_t ProcessSingleSample(uint8_t control) {
        if (control & CONTROL_GATE_RISING) {
            excitation_1_up_.Trigger(15 * 32768);
            excitation_1_down_.Trigger(-1 * 32768);
            excitation_2_.Trigger(13107);
            excitation_noise_.Trigger(snappy_);
        }

        int32_t excitation_1 = 0;
        excitation_1 += excitation_1_up_.Process();
        excitation_1 += excitation_1_down_.Process();
        excitation_1 += !excitation_1_down_.done() ? 2621 : 0;

        int32_t body_1 = body_1_.Process(excitation_1) + (excitation_1 >> 4);

        int32_t excitation_2 = 0;
        excitation_2 += excitation_2_.Process();
        excitation_2 += !excitation_2_.done() ? 13107 : 0;

        int32_t body_2 = body_2_.Process(excitation_2) + (excitation_2 >> 4);
        int32_t noise_sample = Random::GetSample();
        int32_t noise = noise_.Process(noise_sample);
        int32_t noise_envelope = excitation_noise_.Process();
        int32_t sd = 0;
        sd += body_1 * gain_1_ >> 15;
        sd += body_2 * gain_2_ >> 15;
        sd += noise_envelope * noise >> 15;
        sd = CLIP(sd);
        return sd;
    }

    unsigned param_count() const override { return 4; }
    const char *param_name(unsigned arg) const override {
        switch (arg) {
        case 0: return "Frequency";
        case 1: return "Decay";
        case 2: return "Tone";
        case 3: return "Snappy";
        default: return "?";
        }
    }

    void params_fetch_current(uint16_t *tgt) const override {
        tgt[0] = freq_param + 32768;
        tgt[1] = decay_param;
        tgt[2] = tone_param;
        tgt[3] = snappy_param;
    }

    void params_fetch_default(uint16_t *tgt) const override {
        tgt[0] = DEFAULT_FREQUENCY;
        tgt[1] = DEFAULT_DECAY;
        tgt[2] = DEFAULT_TONE;
        tgt[3] = DEFAULT_SNAPPY;
    }

    void params_set(uint16_t *params) override {
        set_frequency(params[0] - 32768);
        set_decay(params[1]);
        set_tone(params[2]);
        set_snappy(params[3]);
    }

    void set_tone(uint16_t tone) {
        tone_param = tone;
        gain_1_ = 22000 - (tone >> 2);
        gain_2_ = 22000 + (tone >> 2);
    }

    void set_snappy(uint16_t snappy) {
        snappy_param = snappy;
        snappy >>= 1;
        if (snappy >= 28672) {
            snappy = 28672;
        }
        snappy_ = 512 + snappy;
    }

    void set_decay(uint16_t decay) {
        decay_param = decay;
        body_1_.set_resonance(29000 + (decay >> 5));
        body_2_.set_resonance(26500 + (decay >> 5));
        excitation_noise_.set_decay(4092 + (decay >> 14));
    }

    void set_frequency(int16_t frequency) {
        freq_param = frequency;
        int16_t base_note = 52 << 7;
        int32_t transposition = frequency;
        base_note += transposition * 896 >> 15;
        body_1_.set_frequency(base_note);
        body_2_.set_frequency(base_note + (12 << 7));
        noise_.set_frequency(base_note + (48 << 7));
    }

private:
    Excitation excitation_1_up_;
    Excitation excitation_1_down_;
    Excitation excitation_2_;
    Excitation excitation_noise_;
    Svf body_1_;
    Svf body_2_;
    Svf noise_;

    int32_t gain_1_;
    int32_t gain_2_;

    uint16_t snappy_;

    int16_t freq_param;
    uint16_t tone_param;
    uint16_t snappy_param;
    uint16_t decay_param;
};

class HighHat : public Configurable {
public:
    HighHat() {}
    ~HighHat() {}

    constexpr static uint16_t DEFAULT_FREQUENCY     = 105 << 9;  // 8kHz, regularized to 64k
    constexpr static uint16_t DEFAULT_TONE          = 47104;     // ~13kHz, regularized as needed in set_tone
    constexpr static uint16_t DEFAULT_CLOSED_DECAY  = 32768;
    constexpr static uint16_t DEFAULT_OPEN_DECAY    = 65535;

    void Init() {
        noise_.Init();
        noise_.set_resonance(24000);
        noise_.set_mode(SVF_MODE_BP);
        // sets both param and frequency
        set_frequency(DEFAULT_FREQUENCY);

        vca_coloration_.Init();
        vca_coloration_.set_resonance(0);
        vca_coloration_.set_mode(SVF_MODE_HP);
        set_tone(DEFAULT_TONE);

        vca_envelope_.Init();
        vca_envelope_.set_delay(0);
        set_decay(DEFAULT_CLOSED_DECAY);
    }

    int16_t ProcessSingleSample(uint8_t control) {
        if (control & CONTROL_GATE_RISING) {
            vca_envelope_.Trigger(32768 * 15);
        }

        phase_[0] += 48318382;
        phase_[1] += 71582788;
        phase_[2] += 37044092;
        phase_[3] += 54313440;
        phase_[4] += 66214079;
        phase_[5] += 93952409;

        int16_t noise = 0;
        noise += phase_[0] >> 31;
        noise += phase_[1] >> 31;
        noise += phase_[2] >> 31;
        noise += phase_[3] >> 31;
        noise += phase_[4] >> 31;
        noise += phase_[5] >> 31;
        noise <<= 12;

        // Run the SVF at the double of the original sample rate for stability.
        int32_t filtered_noise = 0;
        filtered_noise += noise_.Process(noise);
        filtered_noise += noise_.Process(noise);

        // The 808-style VCA amplifies only the positive section of the signal.
        if (filtered_noise < 0) {
            filtered_noise = 0;
        } else if (filtered_noise > 32767) {
            filtered_noise = 32767;
        }

        int32_t envelope = vca_envelope_.Process() >> 4;
        int32_t vca_noise = envelope * filtered_noise >> 14;
        vca_noise = CLIP(vca_noise);
        int32_t hh = 0;
        hh += vca_coloration_.Process(vca_noise);
        hh += vca_coloration_.Process(vca_noise);
        hh <<= 1;
        hh = CLIP(hh);
        return hh;
    }

    unsigned param_count() const override { return 4; }
    const char *param_name(unsigned arg) const override {
        switch (arg) {
        case 0: return "Frequency";
        case 1: return "Tone";
        case 2: return "Cl. Decay";
        case 3: return "Op. Decay";
        default: return "?";
        }
    }

    void params_fetch_current(uint16_t *tgt) const override {
        tgt[0] = freq_param;
        tgt[1] = tone_param;
        tgt[2] = closed_decay_param;
        tgt[3] = open_decay_param;
    }

    void params_fetch_default(uint16_t *tgt) const override {
        tgt[0] = DEFAULT_FREQUENCY;
        tgt[1] = DEFAULT_TONE;
        tgt[2] = DEFAULT_CLOSED_DECAY;
        tgt[3] = DEFAULT_OPEN_DECAY;
    }

    void params_set(uint16_t *params) override {
        set_frequency(params[0]);
        set_tone(params[1]);

        closed_decay_param = params[2];
        open_decay_param   = params[3];
        set_open(open);
    }

    void set_frequency(uint16_t frequency) {
        freq_param = frequency;
        noise_.set_frequency(frequency >> 2);
    }

    void set_decay(uint16_t decay) {
        vca_envelope_.set_decay(4092 + (decay >> 14));
    }

    void set_tone(uint16_t tone) {
        tone_param = tone;
        vca_coloration_.set_frequency(8192 + (tone >> 3));
    }

    void set_open(bool open) {
        if (open)
            set_decay(open_decay_param);
        else
            set_decay(closed_decay_param);
    }

  private:
    Svf noise_;
    Svf vca_coloration_;
    Excitation vca_envelope_;

    uint32_t phase_[6];

    bool open = false;
    uint16_t freq_param, tone_param;
    uint16_t closed_decay_param = DEFAULT_CLOSED_DECAY,
             open_decay_param = DEFAULT_OPEN_DECAY;
};

class FmDrum : public Configurable {
public:
    constexpr static int16_t  DEFAULT_FREQUENCY = 31744;
    constexpr static uint16_t DEFAULT_FM        = 19456;
    constexpr static uint16_t DEFAULT_DECAY     = 31744;
    constexpr static uint16_t DEFAULT_NOISE     = 51199;

    FmDrum() { }
    ~FmDrum() { }

    void Init() {
        step_  = 0;
        phase_ = 0;
        fm_envelope_phase_ = 0xffffffff;
        am_envelope_phase_ = 0xffffffff;
        previous_sample_ = 0;

        set_frequency(DEFAULT_FREQUENCY);
        set_fm_amount(DEFAULT_FM);
        set_decay(DEFAULT_DECAY);
        set_noise(DEFAULT_NOISE);
    }

    int16_t ProcessSingleSample(uint8_t control) {
        if (control & CONTROL_GATE_RISING) {
            fm_envelope_phase_ = 0;
            am_envelope_phase_ = 0;
            aux_envelope_phase_ = 0;
            phase_ = 0x3fff * fm_amount_ >> 16;
            step_ = 0;
        }

        fm_envelope_phase_ += fm_envelope_increment_;
        if (fm_envelope_phase_ < fm_envelope_increment_) {
            fm_envelope_phase_ = 0xffffffff;
        }

        aux_envelope_phase_ += 4473924;

        if (aux_envelope_phase_ < 4473924) {
            aux_envelope_phase_ = 0xffffffff;
        }

        if ((step_ & 3) == 0) {
            uint32_t aux_envelope = 65535 - Interpolate824(
                    lut_env_expo, aux_envelope_phase_);
            uint32_t fm_envelope = 65535 - Interpolate824(
                    lut_env_expo, fm_envelope_phase_);
            phase_increment_ = ComputePhaseIncrement(
                    frequency_ + \
                    (fm_envelope * fm_amount_ >> 16) + \
                    (aux_envelope * aux_envelope_strength_ >> 15) + \
                    (previous_sample_ >> 6));
        }

        phase_ += phase_increment_;

        int16_t mix = Interpolate1022(wav_sine, phase_);
        if (noise_) {
            mix = Mix(mix, Random::GetSample(), noise_);
        }

        am_envelope_phase_ += am_envelope_increment_;
        if (am_envelope_phase_ < am_envelope_increment_) {
            am_envelope_phase_ = 0xffffffff;
        }

        uint32_t am_envelope =
            65535 - Interpolate824(lut_env_expo, am_envelope_phase_);

        mix = (((int32_t)mix) * am_envelope) >> 16;

        if (overdrive_) {
            uint32_t phi = (static_cast<int32_t>(mix) << 16) + (1L << 31);
            int16_t overdriven = Interpolate1022(wav_overdrive, phi);
            mix = Mix(mix, overdriven, overdrive_);
        }

        step_++;
        previous_sample_ = mix;
        return mix;
    }

    void Morph(uint16_t x, uint16_t y) {
        const uint16_t (*map)[4] = sd_range_ ? sd_map : bd_map;
        uint16_t parameters[4];
        for (uint8_t i = 0; i < 4; ++i) {
            uint16_t x_integral = (x >> 14) << 1;
            uint16_t x_fractional = x << 2;
            uint16_t a = map[x_integral][i];
            uint16_t b = map[x_integral + 2][i];
            uint16_t c = map[x_integral + 1][i];
            uint16_t d = map[x_integral + 3][i];

            uint16_t e = a + ((b - a) * x_fractional >> 16);
            uint16_t f = c + ((d - c) * x_fractional >> 16);
            parameters[i] = e + ((f - e) * y >> 16);
        }
    }

    unsigned param_count() const override { return 4; }

    const char *param_name(unsigned arg) const override {
        switch (arg) {
        case 0: return "Frequency";
        case 1: return "FM Amount";
        case 2: return "Decay";
        case 3: return "Noise";
        default: return "?";
        }
    }

    void params_fetch_current(uint16_t *tgt) const override {
        tgt[0] = freq_param;
        tgt[1] = fm_param;
        tgt[2] = decay_param;
        tgt[3] = noise_param;
    }

    void params_fetch_default(uint16_t *tgt) const override {
        tgt[0] = DEFAULT_FREQUENCY;
        tgt[1] = DEFAULT_FM;
        tgt[2] = DEFAULT_DECAY;
        tgt[3] = DEFAULT_NOISE;
    }

    void params_set(uint16_t *params) override {
        set_frequency(params[0]);
        set_fm_amount(params[1]);
        set_decay(params[2]);
        set_noise(params[3]);
    }

    inline void set_sd_range(bool sd_range) {
        sd_range_ = sd_range;
    }

    inline void set_frequency(uint16_t frequency) {
        freq_param = frequency;
        if (frequency <= 16384) {
            aux_envelope_strength_ = 1024;
        } else if (frequency <= 32768) {
            aux_envelope_strength_ = 2048 - (frequency >> 4);
        } else {
            aux_envelope_strength_ = 0;
        }
        frequency_ = (24 << 7) + ((72 << 7) * frequency >> 16);
    }

    inline void set_fm_amount(uint16_t fm_amount) {
        fm_param = fm_amount;
        fm_amount_ = ((fm_amount >> 2) >> 2 * 3);
    }

    inline void set_decay(uint16_t decay) {
        decay_param = decay;
        am_decay_ = 16384 + (decay >> 1);
        fm_decay_ = 8192 + (decay >> 2);
        am_envelope_increment_ = ComputeEnvelopeIncrement(am_decay_);
        fm_envelope_increment_ = ComputeEnvelopeIncrement(fm_decay_);
    }

    inline void set_noise(uint16_t noise) {
        noise_param = noise;
        uint32_t n = noise;
        noise_ = noise >= 32768 ? ((n - 32768) * (n - 32768) >> 15) : 0;
        noise_ = (noise_ >> 2) * 5;
        overdrive_ = noise <= 32767 ? ((32767 - n) * (32767 - n) >> 14) : 0;
    }

private:
    bool sd_range_;

    uint32_t ComputeEnvelopeIncrement(uint16_t decay) {
        // Interpolate the two neighboring values of the env_increments table
        uint32_t a = lut_env_increments[decay >> 8];
        uint32_t b = lut_env_increments[(decay >> 8) + 1];
        return a - ((a - b) * (decay & 0xff) >> 8);
    }

    uint16_t aux_envelope_strength_;
    uint16_t frequency_;
    uint16_t fm_amount_;
    uint16_t am_decay_;
    uint16_t fm_decay_;

    uint32_t am_envelope_increment_;
    uint32_t fm_envelope_increment_;

    uint16_t noise_;
    uint16_t overdrive_;
    int16_t previous_sample_;

    uint32_t phase_;
    uint32_t step_;
    uint32_t fm_envelope_phase_;
    uint32_t am_envelope_phase_;
    uint32_t aux_envelope_phase_;
    uint32_t phase_increment_;

    uint16_t freq_param, fm_param, decay_param, noise_param;
};

class Clap : public Configurable {
public:
    Clap() {};
    ~Clap() {};

    constexpr static uint16_t DEFAULT_FREQUENCY     = 42976;
    constexpr static uint16_t DEFAULT_RESONANCE     = 65535;
    constexpr static uint16_t DEFAULT_FAST_DECAY    = 8960;
    constexpr static uint16_t DEFAULT_LONG_DECAY    = 49151;

    void Init() {
        vca_envelope_.Init();
        vca_envelope_.set_repeats(3);

        set_fast_decay(DEFAULT_FAST_DECAY);
        set_long_decay(DEFAULT_LONG_DECAY);

        vca_filter_.Init();
        vca_filter_.set_resonance(1000);
        vca_filter_.set_mode(SVF_MODE_BP);
        set_frequency(DEFAULT_FREQUENCY);
        set_resonance(DEFAULT_RESONANCE);
    }

    int16_t ProcessSingleSample(uint8_t control) {
        if (control & CONTROL_GATE_RISING) {
            // TODO: Set this properly!
            vca_envelope_.Trigger(32768 * 13);
        }

        int16_t noise = Random::GetSample();

        int32_t filtered_noise = 0;
        filtered_noise += vca_filter_.Process(noise);
        filtered_noise += vca_filter_.Process(noise);

        int32_t envelope = vca_envelope_.Process() >> 4;
        int32_t vca_noise = envelope * filtered_noise >> 14;

        vca_noise = CLIP(vca_noise);
        return vca_noise;
    }

    unsigned param_count() const override { return 4; }
    const char *param_name(unsigned arg) const override {
        switch (arg) {
        case 0: return "Frequency";
        case 1: return "Resonance";
        case 2: return "Fast Decay";
        case 3: return "Long Decay";
        default: return "?";
        }
    }

    void params_fetch_current(uint16_t *tgt) const override {
        tgt[0] = freq_param;
        tgt[1] = resonance_param;
        tgt[2] = fast_decay_param;
        tgt[3] = long_decay_param;
    }

    void params_fetch_default(uint16_t *tgt) const override {
        tgt[0] = DEFAULT_FREQUENCY;
        tgt[1] = DEFAULT_RESONANCE;
        tgt[2] = DEFAULT_FAST_DECAY;
        tgt[3] = DEFAULT_LONG_DECAY;
    }

    void params_set(uint16_t *params) override {
        set_frequency(params[0]);
        set_resonance(params[1]);
        set_fast_decay(params[2]);
        set_long_decay(params[3]);
    }

    void set_frequency(uint16_t frequency) {
        freq_param = frequency;
        vca_filter_.set_frequency(frequency >> 2);
    }

    void set_resonance(uint16_t frequency) {
        resonance_param = frequency;
        vca_filter_.set_resonance(frequency >> 2);
    }

    void set_fast_decay(uint16_t decay) {
        fast_decay_param = decay;
        vca_envelope_.set_decay(3968 + (decay >> 9));
    }

    void set_long_decay(uint16_t decay) {
        long_decay_param = decay;
        vca_envelope_.set_decay_term(4092 + (decay >> 14));
    }

private:
    Repeater vca_envelope_;
    Svf vca_filter_;

    uint16_t freq_param, resonance_param;
    uint16_t fast_decay_param, long_decay_param;

};

/// 909-style kick synth
class KickDrum : public Configurable {
public:
    KickDrum() {};
    ~KickDrum() {};

    constexpr static uint16_t DEFAULT_FREQUENCY     = 16000;
    constexpr static uint16_t DEFAULT_TONE          = 36000;
    constexpr static uint16_t DEFAULT_ATTACK        = 40000;
    constexpr static uint16_t DEFAULT_DECAY         = 45535;
    constexpr static uint16_t DEFAULT_OVERDRIVE     = 42384;
    constexpr static uint16_t DEFAULT_TONE_DECAY    = 32767;

    void Init() {
        tone_envelope_.Init();
        peak_envelope_.Init();
        ps_envelope_.Init();

        // hardcoded
        peak_envelope_.set_decay(4000);

        peak_filter_.Init();
        peak_filter_.set_resonance(36384);
        peak_filter_.set_mode(SVF_MODE_LP);

        set_frequency(DEFAULT_FREQUENCY);
        set_attack(DEFAULT_ATTACK);
        set_tone(DEFAULT_TONE);
        set_decay(DEFAULT_DECAY);
        set_overdrive(DEFAULT_OVERDRIVE);
        set_tone_decay(DEFAULT_TONE_DECAY);
    }

    int16_t ProcessSingleSample(uint8_t control) {
        if (control & CONTROL_GATE_RISING) {
            tone_envelope_.Trigger(32768 * 2);
            peak_envelope_.Trigger(32768 * 6);
            ps_envelope_.Trigger(32768);
            phase_ = 0;
            state_ = 0;
            phase_increment_ = 0;
            tone_excitation_ = 0;
        }

        // ---- Noise --------------------------------------
        // we just use excitation directly, noise just added inconsistency here
        int32_t envelope = peak_envelope_.Process() >> 4;

        int32_t filtered_noise = 0;
        filtered_noise += peak_filter_.Process(envelope);
        filtered_noise += peak_filter_.Process(envelope);

        int32_t vca_noise = filtered_noise;
        vca_noise = vca_noise * attack_param >> 16;

        // ---- Tone ---------------------------------------
        if ((state_ & 0x03) == 0) {
            // this makes the tone excitation 4x longer
            tone_excitation_ = tone_envelope_.Process() >> 4;
            // ramp up to limit clicking
            tone_excitation_ = tone_excitation_ * lut_env_expo[state_ < 255 ? state_ : 255] >> 16;
            pitch_sweep_      = ps_envelope_.Process();
            phase_increment_ = ComputePhaseIncrement(
                    frequency_
                    + (frequency_ * (65535 - tone_decay_) >> 16)
                    + (tone_decay_ * pitch_sweep_ >> 16));
        }

        state_++;

        int16_t tone = Interpolate1022(wav_sine, phase_);

        phase_ += phase_increment_;

        int32_t vca_tone = tone_excitation_ * tone >> 15;

        // ---- Mix ----------------------------------------
        // TODO: Distort the tonal portion when applicable
        // TODO: Delay for the tone_envelope trigger
        int32_t mix = vca_noise + vca_tone;

        if (overdrive_) {
            uint32_t phi = (static_cast<int32_t>(mix) << 16) + (1L << 31);
            int16_t overdriven = Interpolate1022(wav_overdrive, phi);
            mix = Mix(mix, overdriven, overdrive_);
        }

        mix = CLIP(mix);
        return mix;
    }

    unsigned param_count() const override { return 6; }
    const char *param_name(unsigned arg) const override {
        switch (arg) {
        case 0: return "Frequency";
        case 1: return "Tone";
        case 2: return "Attack";
        case 3: return "Decay";
        case 4: return "Overdrive";
        case 5: return "Tone Decay";
        default: return "?";
        }
    }

    void params_fetch_current(uint16_t *tgt) const override {
        tgt[0] = freq_param;
        tgt[1] = tone_param;
        tgt[2] = attack_param;
        tgt[3] = decay_param;
        tgt[4] = overdrive_param;
        tgt[5] = tone_decay_param;
    }

    void params_fetch_default(uint16_t *tgt) const override {
        tgt[0] = DEFAULT_FREQUENCY;
        tgt[1] = DEFAULT_TONE;
        tgt[2] = DEFAULT_ATTACK;
        tgt[3] = DEFAULT_DECAY;
        tgt[3] = DEFAULT_OVERDRIVE;
        tgt[3] = DEFAULT_TONE_DECAY;
    }

    void params_set(uint16_t *params) override {
        set_frequency(params[0]);
        set_tone(params[1]);
        set_attack(params[2]);
        set_decay(params[3]);
        set_overdrive(params[4]);
        set_tone_decay(params[5]);
    }

    void set_frequency(uint16_t freq) {
        freq_param = freq;
        frequency_ = (24 << 6) + ((72 << 5) * freq >> 16);
    }

    void set_tone(uint16_t tone) {
        tone_param = tone;
        peak_filter_.set_frequency(tone >> 2);
    }

    void set_attack(uint16_t attack) {
        attack_param = attack;
    }

    void set_decay(uint16_t decay) {
        decay_param = decay;
        tone_envelope_.set_decay(4080 + (decay >> 12));
        ps_envelope_.set_decay(4080 + (decay >> 12));
    }

    void set_overdrive(uint16_t overdrive) {
        overdrive_param = overdrive;
        overdrive_ = overdrive_param;
    }

    void set_tone_decay(uint16_t decay) {
        tone_decay_param = decay;
        tone_decay_ = decay >> 3;
    }

private:
    Excitation tone_envelope_;  // envelope of the tonal part
    Excitation peak_envelope_; // envelope of the peak part (initial peak click)
    Excitation ps_envelope_;    // pitch sweep (applies to the tonal part)
    Svf peak_filter_;  // this filters the peak sound with low pass

    // cached
    uint32_t frequency_;
    uint16_t overdrive_, tone_decay_;
    int32_t pitch_sweep_;

    // these are state variables, not parameters
    uint32_t phase_, state_, phase_increment_;
    int32_t tone_excitation_;

    uint16_t freq_param, tone_param;
    uint16_t attack_param, decay_param, overdrive_param, tone_decay_param;
};

} // end namespace peaks
