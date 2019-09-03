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
