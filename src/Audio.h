#pragma once
#include "Engine.h"
// Persistent phase/history makes the sample stream independent of host block size.
class Audio {
public:
    void prepare(double rate);
    void reset();
    void render(Engine&,float* left,float* right,unsigned count);
    unsigned latency() const;
private:
    double rate=44100,phase=1;
    unsigned head=0;
    std::array<float,32> histL{},histR{};
};
