#include "ReverseEngine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numeric>

namespace noirnuit
{
namespace
{
constexpr double pi = 3.14159265358979323846;
float clamp(float x, float lo, float hi) { return std::max(lo, std::min(hi, x)); }

class OnePoleLP
{
public:
    explicit OnePoleLP(double freq, double sampleRate)
    {
        a = static_cast<float>(std::exp(-2.0 * pi * freq / sampleRate));
    }
    float process(float input) { z = (1.0f - a) * input + a * z; return z; }
private:
    float a = 0.0f, z = 0.0f;
};

class FractionalDelay
{
public:
    explicit FractionalDelay(int maxSamples) : data(static_cast<std::size_t>(std::max(8, maxSamples)), 0.0f) {}
    float process(float input, double delay)
    {
        const double n = static_cast<double>(data.size());
        double read = static_cast<double>(write) - delay;
        while (read < 0.0) read += n;
        const auto i0 = static_cast<std::size_t>(read) % data.size();
        const auto i1 = (i0 + 1) % data.size();
        const float fraction = static_cast<float>(read - std::floor(read));
        const float out = data[i0] + fraction * (data[i1] - data[i0]);
        data[write] = input;
        write = (write + 1) % data.size();
        return out;
    }
private:
    std::vector<float> data;
    std::size_t write = 0;
};

struct DelayLine
{
    explicit DelayLine(std::size_t n) : samples(std::max<std::size_t>(4, n), 0.0f) {}
    float read() const noexcept { return samples[position]; }
    void write(float x) noexcept
    {
        samples[position] = x;
        if (++position >= samples.size()) position = 0;
    }
    std::vector<float> samples;
    std::size_t position = 0;
    float damping = 0.0f;
};

// 8x8 normalized Walsh-Hadamard feedback network: no third-party DSP code.
void hadamard8(std::array<float, 8>& a)
{
    for (int step = 1; step < 8; step *= 2)
        for (int i = 0; i < 8; i += step * 2)
            for (int j = 0; j < step; ++j)
            {
                const float x = a[static_cast<std::size_t>(i + j)];
                const float y = a[static_cast<std::size_t>(i + j + step)];
                a[static_cast<std::size_t>(i + j)] = x + y;
                a[static_cast<std::size_t>(i + j + step)] = x - y;
            }
    for (auto& v : a) v *= 0.3535533905932738f;
}

struct ChannelFilters
{
    explicit ChannelFilters(double sr) : lowSibilance(2800.0, sr), phoneLow(3500.0, sr), phoneBass(320.0, sr) {}
    OnePoleLP lowSibilance, phoneLow, phoneBass;
    float energyFast = 0.0f, energyLow = 0.0f;
    float preProcess(float x, const Settings& s)
    {
        if (s.phone)
        {
            const float hi = phoneLow.process(x);
            x = hi - phoneBass.process(hi);
        }
        if (s.vocalMode && s.sibilance > 0.0f)
        {
            const float low = lowSibilance.process(x);
            const float high = x - low;
            energyFast = 0.997f * energyFast + 0.003f * high * high;
            energyLow = 0.997f * energyLow + 0.003f * low * low;
            const float ratio = energyFast / (energyFast + energyLow + 1.0e-8f);
            x = low + high * (1.0f - 0.88f * s.sibilance * clamp((ratio - 0.32f) * 2.0f, 0.0f, 1.0f));
        }
        if (s.drive)
            x = 0.60f * std::tanh(2.8f * x);
        return x;
    }
};
}

StereoClip createReverseReverb(const StereoClip& input, const Settings& settings)
{
    StereoClip out;
    if (!input.valid()) return out;
    out.sampleRate = input.sampleRate;
    const double sr = static_cast<double>(input.sampleRate);
    const auto targetFrames = static_cast<std::size_t>(
        std::ceil(std::max(0.25, std::min(45.0, settings.durationSeconds)) * sr));
    out.left.assign(targetFrames, 0.0f);
    out.right.assign(targetFrames, 0.0f);

    // Look back for the latest meaningful audio, not the silence after a phrase.
    // Threshold is relative to the recorded material so quiet takes still work.
    const std::size_t inputFrames = input.length();
    float peakInput = 0.0f;
    for (std::size_t i = 0; i < inputFrames; ++i)
        peakInput = std::max(peakInput, std::max(std::abs(input.left[i]), std::abs(input.right[i])));
    if (peakInput < 1.0e-5f) return out;
    const float activeThreshold = std::max(0.002f, peakInput * 0.035f);
    std::size_t lastActive = inputFrames - 1;
    for (std::size_t i = inputFrames; i > 0; --i)
    {
        const std::size_t k = i - 1;
        if (std::max(std::abs(input.left[k]), std::abs(input.right[k])) > activeThreshold)
        { lastActive = k; break; }
    }
    const std::size_t phraseEnd = std::min(inputFrames, lastActive + static_cast<std::size_t>(sr * 0.045));
    const auto maxPhrase = static_cast<std::size_t>(std::min(2.25, settings.durationSeconds * 0.65) * sr);
    const std::size_t phraseLen = std::min(phraseEnd, std::max<std::size_t>(1, maxPhrase));

    ChannelFilters filtersL(sr), filtersR(sr);
    constexpr std::array<double, 8> baseTimes{{0.0317, 0.0371, 0.0411, 0.0479,
                                                 0.0533, 0.0611, 0.0713, 0.0833}};
    // Changing the line lengths avoids resonant regularities at different rates.
    std::array<DelayLine, 8> lines{{
        DelayLine(static_cast<std::size_t>(baseTimes[0] * sr)),
        DelayLine(static_cast<std::size_t>(baseTimes[1] * sr)),
        DelayLine(static_cast<std::size_t>(baseTimes[2] * sr)),
        DelayLine(static_cast<std::size_t>(baseTimes[3] * sr)),
        DelayLine(static_cast<std::size_t>(baseTimes[4] * sr)),
        DelayLine(static_cast<std::size_t>(baseTimes[5] * sr)),
        DelayLine(static_cast<std::size_t>(baseTimes[6] * sr)),
        DelayLine(static_cast<std::size_t>(baseTimes[7] * sr)) }};
    const double rt60 = std::max(0.3, settings.durationSeconds * (0.35 + 0.65 * clamp(settings.decay, 0.f, 1.f)));
    std::array<float, 8> fb;
    for (std::size_t k = 0; k < 8; ++k)
        fb[k] = static_cast<float>(std::pow(0.001, baseTimes[k] / rt60));
    const float tone = clamp(settings.tone, 0.0f, 1.0f);
    const double filterFreq = 1800.0 * std::pow(11.0, static_cast<double>(tone));
    const float smooth = static_cast<float>(std::exp(-2.0 * pi * filterFreq / sr));

    // Source in reverse; reverb in forward time; output then reversed.
    // Hadamard FDN (8 delays) creates a diffuse tail with a naturally rising reverse envelope.
    for (std::size_t frame = 0; frame < targetFrames; ++frame)
    {
        float dryL = 0.0f, dryR = 0.0f;
        if (frame < phraseLen)
        {
            const std::size_t i = phraseEnd - 1 - frame;
            const float sourceFade = std::min(1.0f, static_cast<float>(frame) / static_cast<float>(0.012 * sr + 1));
            dryL = filtersL.preProcess(input.left[i] * sourceFade, settings);
            dryR = filtersR.preProcess(input.right[i] * sourceFade, settings);
        }
        const float m = 0.5f * (dryL + dryR);
        const float s = 0.5f * (dryL - dryR);
        std::array<float, 8> taps;
        for (std::size_t k = 0; k < 8; ++k)
        {
            const float delayed = lines[k].read();
            lines[k].damping = smooth * lines[k].damping + (1.0f - smooth) * delayed;
            taps[k] = lines[k].damping;
        }
        auto mixed = taps;
        hadamard8(mixed);
        for (std::size_t k = 0; k < 8; ++k)
        {
            const float injected = m * ((k % 2 == 0) ? 0.31f : -0.31f)
                                 + s * ((k % 3 == 0) ? 0.20f : -0.20f);
            lines[k].write(injected + mixed[k] * fb[k]);
        }
        const float reflectedL = (taps[0] - taps[2] + taps[4] - taps[6]) * 0.24f;
        const float reflectedR = (taps[1] - taps[3] + taps[5] - taps[7]) * 0.24f;
        const float env = std::min(1.0f, static_cast<float>(frame) / static_cast<float>(0.008 * sr + 1));
        out.left[targetFrames - 1 - frame] = env * (dryL * 0.18f + reflectedL);
        out.right[targetFrames - 1 - frame] = env * (dryR * 0.18f + reflectedR);
    }

    // Creative effects on the already reversed tail.
    FractionalDelay chorusL(static_cast<int>(0.075 * sr) + 8);
    FractionalDelay chorusR(static_cast<int>(0.075 * sr) + 8);
    const float width = clamp(settings.width, 0.0f, 1.8f);
    const double beatsPerSecond = std::max(30.0, std::min(300.0, settings.bpm)) / 60.0;
    float peak = 0.0f;
    for (std::size_t i = 0; i < targetFrames; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        float l = out.left[i], r = out.right[i];
        if (settings.chorus)
        {
            const double lTime = 0.022 + 0.006 * std::sin(2.0 * pi * 0.43 * t);
            const double rTime = 0.025 + 0.006 * std::sin(2.0 * pi * 0.52 * t + 1.8);
            l = 0.78f * l + 0.22f * chorusL.process(l, lTime * sr);
            r = 0.78f * r + 0.22f * chorusR.process(r, rTime * sr);
        }
        if (settings.tremolo)
        {
            const float gain = 0.72f + 0.28f * static_cast<float>(std::sin(2.0 * pi * beatsPerSecond * 2.0 * t));
            l *= gain; r *= gain;
        }
        const float mid = 0.5f * (l + r);
        const float side = 0.5f * (l - r) * width;
        l = mid + side; r = mid - side;
        // 15 ms fade-in; 12 ms click protection at the destination point.
        const float fadeIn = std::min(1.0f, static_cast<float>(i) / static_cast<float>(0.015 * sr + 1));
        const float fadeOut = std::min(1.0f, static_cast<float>(targetFrames - 1 - i) / static_cast<float>(0.012 * sr + 1));
        const float shaping = std::pow(static_cast<float>(i + 1) / static_cast<float>(targetFrames), 0.28f);
        const float fade = fadeIn * fadeOut * shaping;
        out.left[i] = l * fade;
        out.right[i] = r * fade;
        peak = std::max(peak, std::max(std::abs(out.left[i]), std::abs(out.right[i])));
    }
    if (peak > 0.00001f)
    {
        const float makeup = std::min(5.0f, 0.87f / peak);
        for (std::size_t i = 0; i < targetFrames; ++i)
        {
            out.left[i] = clamp(out.left[i] * makeup, -0.99f, 0.99f);
            out.right[i] = clamp(out.right[i] * makeup, -0.99f, 0.99f);
        }
    }
    return out;
}
}
