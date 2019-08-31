#pragma once

namespace peaks {

// Note: Have to change this if there ever comes a percussion with more
// parameters
const uint16_t PARAM_MAX = 4;

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

const uint16_t lut_svf_cutoff[] = {
    35,    37,    39,    41,    44,    46,    49,    52,    55,    58,    62,
    66,    70,    74,    78,    83,    88,    93,    99,    105,   111,   117,
    124,   132,   140,   148,   157,   166,   176,   187,   198,   210,   222,
    235,   249,   264,   280,   297,   314,   333,   353,   374,   396,   420,
    445,   471,   499,   529,   561,   594,   629,   667,   706,   748,   793,
    840,   890,   943,   999,   1059,  1122,  1188,  1259,  1334,  1413,  1497,
    1586,  1681,  1781,  1886,  1999,  2117,  2243,  2377,  2518,  2668,  2826,
    2994,  3172,  3361,  3560,  3772,  3996,  4233,  4485,  4751,  5033,  5332,
    5648,  5983,  6337,  6713,  7111,  7532,  7978,  8449,  8949,  9477,  10037,
    10628, 11254, 11916, 12616, 13356, 14138, 14964, 15837, 16758, 17730, 18756,
    19837, 20975, 22174, 23435, 24761, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078, 25078,
    25078, 25078, 25078, 25078,
};

const uint16_t lut_svf_damp[] = {
    65534, 49166, 46069, 43993, 42386, 41058, 39917, 38910, 38007, 37184, 36427,
    35726, 35070, 34454, 33873, 33322, 32798, 32299, 31820, 31361, 30920, 30496,
    30086, 29690, 29306, 28935, 28574, 28224, 27883, 27551, 27228, 26912, 26605,
    26304, 26010, 25723, 25441, 25166, 24896, 24631, 24371, 24116, 23866, 23620,
    23379, 23141, 22908, 22678, 22452, 22229, 22010, 21794, 21581, 21371, 21164,
    20960, 20759, 20560, 20365, 20171, 19980, 19791, 19605, 19421, 19239, 19059,
    18882, 18706, 18532, 18360, 18190, 18022, 17856, 17691, 17528, 17367, 17207,
    17049, 16892, 16737, 16583, 16431, 16280, 16131, 15982, 15836, 15690, 15546,
    15403, 15261, 15120, 14981, 14843, 14705, 14569, 14434, 14300, 14167, 14036,
    13905, 13775, 13646, 13518, 13391, 13265, 13140, 13015, 12892, 12769, 12648,
    12527, 12407, 12287, 12169, 12051, 11934, 11818, 11703, 11588, 11474, 11361,
    11249, 11137, 11026, 10915, 10805, 10696, 10588, 10480, 10373, 10266, 10160,
    10055, 9950,  9846,  9742,  9639,  9537,  9435,  9333,  9233,  9132,  9033,
    8933,  8835,  8737,  8639,  8542,  8445,  8349,  8253,  8158,  8063,  7969,
    7875,  7782,  7689,  7596,  7504,  7413,  7321,  7231,  7140,  7050,  6961,
    6872,  6783,  6695,  6607,  6519,  6432,  6346,  6259,  6173,  6088,  6003,
    5918,  5833,  5749,  5665,  5582,  5499,  5416,  5334,  5251,  5170,  5088,
    5007,  4926,  4846,  4766,  4686,  4607,  4527,  4449,  4370,  4292,  4214,
    4136,  4059,  3982,  3905,  3828,  3752,  3676,  3601,  3525,  3450,  3375,
    3301,  3226,  3152,  3078,  3005,  2932,  2859,  2786,  2713,  2641,  2569,
    2497,  2426,  2355,  2284,  2213,  2142,  2072,  2002,  1932,  1862,  1793,
    1724,  1655,  1586,  1518,  1449,  1381,  1313,  1246,  1178,  1111,  1044,
    977,   911,   844,   778,   712,   647,   581,   516,   450,   385,   321,
    256,   192,   127,   63,
};

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
        level_ = level;
        counter_ = delay_ + 1;
    }

    bool done() { return counter_ == 0; }

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

    constexpr static uint16_t DEFAULT_FREQUENCY     = 105 << 8;  // 8kHz, regularized to 64k
    constexpr static uint16_t DEFAULT_TONE          = 110 << 8;  // 13kHz, regularized to 64k
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
        // TODO: better scaling? 32kHz max i.e. >> 1
        freq_param = frequency;
        noise_.set_frequency(frequency >> 1);
    }

    void set_decay(uint16_t decay) {
        vca_envelope_.set_decay(4092 + (decay >> 14));
    }

    void set_tone(uint16_t tone) {
        tone_param = tone;
        vca_coloration_.set_frequency(tone >> 1);
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

} // end namespace peaks
