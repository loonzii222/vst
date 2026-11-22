#pragma once
#include <vector>
#include <cstddef>

namespace noirnuit
{
struct StereoClip
{
    int sampleRate = 44100;
    std::vector<float> left, right;
    std::size_t length() const noexcept { return left.size(); }
    bool valid() const noexcept { return !left.empty() && left.size() == right.size() && sampleRate > 0; }
};

struct Settings
{
    double durationSeconds = 8.0;
    float decay = 0.72f;      // 0..1
    float tone = 0.65f;       // 0..1; dark to bright
    float width = 1.15f;     // 0..1.8
    float sibilance = 0.75f;  // 0..1
    bool vocalMode = true;
    bool phone = false;
    bool chorus = false;
    bool drive = false;
    bool tremolo = false;
    double bpm = 120.0;
};

// Pure C++ DSP, so the exact same audio engine can be used in the VST3 and CLI.
// Generates a wet reverse-reverb file whose end lines up with the next phrase.
StereoClip createReverseReverb(const StereoClip& input, const Settings& settings);
}
