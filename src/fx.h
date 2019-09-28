#pragma once

#include <Arduino.h>

// pushes on top, retrieves from end
template<unsigned LEN>
class RingBuffer {
public:
    void push(int16_t sample) {
        buffer[pos] = sample;
        pos = next_pos();
    }

    int16_t get() {
        return buffer[next_pos()];
    }

protected:
    unsigned next_pos() const {
        return (pos + 1) % LEN;
    }

    unsigned pos = 0;
    int16_t buffer[LEN];
};

// TODO: redo this to use comb filter to save code
// monophonic echo
class Echo {
public:
    Echo() {}
    ~Echo() {}

    void Process(int16_t &left, int16_t &right) {
        int16_t cur_value = (left+right)/2;
        int16_t past = buffer.get();

        buffer.push(cur_value - (past * decay >> 16));

        left += past;
        right += past;
    }

    void set_delay(uint16_t del) {
        delay = del >> 2;
    }

    void set_decay(uint16_t dec) {
        decay = dec;
    }

protected:
    static constexpr unsigned BUF_LEN = 1 << 14;

    uint16_t delay = BUF_LEN;
    uint16_t decay = 0x4000;

    RingBuffer<16384> buffer;
};

// comb filter
template<uint32_t Len>
struct Comb {
    Comb(uint16_t feedback = 32768) : feedback(feedback) {}

    int16_t process(int16_t input) {
        int16_t res = buffer[pos];
        buffer[pos] = input + (res * feedback >> 16);
        pos = (pos + 1) % Len;
        return res;
    }

    uint16_t feedback = 32768;
    uint32_t pos = 0;
    int16_t buffer[Len];
};

// allpass filter
template<uint32_t Len>
struct Allpass {
    Allpass(uint16_t feedback = 32768) : feedback(feedback) {}

    int16_t process(int16_t input) {
        int16_t bout = buffer[pos];
        buffer[pos] = input + (bout * feedback >> 16);
        int16_t output = bout - (buffer[pos] * feedback >> 16);
        pos = (pos + 1) % Len;
        return output;
    }

    uint16_t feedback = 32768;
    uint32_t pos = 0;
    int16_t buffer[Len];
};

// really quite a compromise reverb, but it works reasonably well
// inspired by jcrev and freeverb (a hybrid of the two)
struct Reverb {
    Reverb()
            : ap1(0.5 * 65535)
            , ap2(0.5 * 65535)
            , ap3(0.5 * 65535)
            , ap4(0.5 * 65535)
            , c1(0.773 * 65535)
            , c2(0.802 * 65535)
            , c3(0.753 * 65536)
            , c4(0.733 * 65535)
    {}

    void Process(int16_t &left, int16_t &right) {
        int16_t mix = (left + right) >> 1;
        int16_t rev = process(mix);
        left += rev;
        right += rev;
    }

    int16_t process(int16_t src) {
        int32_t r1 = c1.process(src);
        int32_t r2 = c2.process(src);
        int32_t r3 = c3.process(src);
        int32_t r4 = c4.process(src);

        int16_t sum = (r1 + r2 + r3 + r4) >> 2;

        int16_t p1;
        p1 = ap1.process(sum);
        p1 = ap2.process(p1);
        p1 = ap3.process(p1);
        p1 = ap4.process(p1);

        return p1;
    }

    // allpass in series
    Allpass<225> ap1;
    Allpass<556> ap2;
    Allpass<441> ap3;
    Allpass<341> ap4;

    // comb filters, parallel
    Comb<1687> c1;
    Comb<1601> c2;
    Comb<2053> c3;
    Comb<2251> c4;
};
