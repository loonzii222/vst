#pragma once
#include <JuceHeader.h>
#include "ReverseEngine.h"
#include <atomic>
#include <memory>
#include <mutex>

class NoirnuitReverseAudioProcessor final : public juce::AudioProcessor
{
public:
    NoirnuitReverseAudioProcessor();
    ~NoirnuitReverseAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "NOIRNUIT REVERSE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
    static APVTS::ParameterLayout createParameterLayout();

    noirnuit::StereoClip snapshotLiveAudio(double seconds = 12.0);
    noirnuit::StereoClip snapshotSource();
    bool importWav(const juce::File& file, juce::String& message);
    void useLiveInput();
    void setCapturedSource(noirnuit::StereoClip clip);
    void storeRendered(noirnuit::StereoClip clip);
    std::shared_ptr<const juce::AudioBuffer<float>> getRendered() const;
    bool writeRenderedWav(const juce::File& file) const;
    noirnuit::Settings currentSettings() const;
    void startPreview();
    void stopPreview();
    bool isPreviewing() const { return previewActive.load(); }
    double getBpm() const { return hostBpm.load(); }
    int getSourceSampleRate() const { return currentSampleRate.load(); }
    bool hasSource() const;

private:
    std::unique_ptr<juce::AudioBuffer<float>> ring;
    std::atomic<int> ringHead {0};
    std::atomic<int64_t> ringWritten {0};
    std::atomic<bool> ringFrozen {false};
    std::atomic<int> activeWriter {0};
    std::atomic<int> currentSampleRate {44100};
    std::atomic<double> hostBpm {120.0};
    mutable std::mutex sourceLock;
    noirnuit::StereoClip chosenSource;
    mutable std::shared_ptr<const juce::AudioBuffer<float>> renderedAudio;
    std::atomic<bool> previewActive {false};
    std::atomic<int64_t> previewPosition {0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoirnuitReverseAudioProcessor)
};
