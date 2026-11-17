#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

// Plain values cross the UI/audio boundary; no command carries an owning pointer.
// Key/Pad/Fader/Wheel use a=index, b=value; Buttons uses a=bitmask;
// Encoder uses a=index, b=relative steps; Midi uses a=status, b/c=data bytes.
enum class ControlKind { Key, Pad, Buttons, Encoder, Fader, Wheel, Midi, Panic, ReleaseNotes };
struct Control { ControlKind kind; int a=0, b=0, c=0; };
struct Display {
    // Firmware-native RGB565 pixels and packed pad colors, copied for the editor.
    std::array<uint16_t, 240*240> pixels{};
    std::array<uint32_t, 16> pads{};
    uint32_t buttons=0, master=0;
};
// One isolated firmware instance. Callers provide synchronization; this class
// never shares mutable musical state with another Engine.
class Engine {
public:
    Engine();
    ~Engine();
    Engine(const Engine&)=delete;
    Engine& operator=(const Engine&)=delete;
    // Always produces 44.1 kHz stereo audio; Audio adapts it to the host rate.
    void render(float* left, float* right, unsigned frames);
    void midi(unsigned status, unsigned d1, unsigned d2);
    void control(Control command);
    void display(Display& result);
    std::vector<uint8_t> save();
    bool load(const uint8_t* data, size_t size);
    // Destination must already be initialized. Copies state without allocating.
    void copyTo(Engine& destination) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
