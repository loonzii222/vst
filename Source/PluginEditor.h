#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <atomic>
#include <array>
#include <memory>

class NoirnuitLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
};

class NoirnuitReverseAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                  private juce::Thread,
                                                  private juce::Timer,
                                                  public juce::FileDragAndDropTarget
{
public:
    explicit NoirnuitReverseAudioProcessorEditor(NoirnuitReverseAudioProcessor&);
    ~NoirnuitReverseAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    class DraggableButton final : public juce::TextButton
    {
    public:
        DraggableButton(NoirnuitReverseAudioProcessorEditor& owner);
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
    private:
        NoirnuitReverseAudioProcessorEditor& editor;
        bool dragged = false;
    };
    void run() override;
    void timerCallback() override;
    void startRender();
    void loadAudioFile(const juce::File&);
    void openImportChooser();
    void openExportChooser();
    void openSavePresetChooser();
    void openLoadPresetChooser();
    void updatePreset(int idx);
    void setParam(const juce::String& id, float raw);
    void externalDrag();
    void formatButton(juce::TextButton&, const juce::String&, juce::Colour);
    void formatSlider(juce::Slider&, const juce::String& id,
                      std::unique_ptr<NoirnuitReverseAudioProcessor::APVTS::SliderAttachment>& att);
    void setStatus(const juce::String&);
    void paintWaveform(juce::Graphics&, juce::Rectangle<float> bounds);

    NoirnuitReverseAudioProcessor& processor;
    NoirnuitLookAndFeel skin;
    juce::Image backdrop;
    float animationPhase = 0.0f;
    juce::ComboBox factoryPresets, lengthChoice;
    juce::TextButton captureButton, liveButton, importButton;
    juce::TextButton vocalButton, phoneButton, chorusButton, driveButton, tremoloButton;
    juce::TextButton renderButton, listenButton, exportButton, savePresetButton, loadPresetButton;
    DraggableButton dragButton;
    juce::Slider decaySlider, toneSlider, widthSlider, sibilanceSlider;
    using SliderAtt = NoirnuitReverseAudioProcessor::APVTS::SliderAttachment;
    using ButtonAtt = NoirnuitReverseAudioProcessor::APVTS::ButtonAttachment;
    std::unique_ptr<SliderAtt> decayAttach, toneAttach, widthAttach, sibilanceAttach;
    std::unique_ptr<NoirnuitReverseAudioProcessor::APVTS::ComboBoxAttachment> lengthAttach;
    std::unique_ptr<ButtonAtt> vocalAttach, phoneAttach, chorusAttach, driveAttach, tremoloAttach;
    std::unique_ptr<juce::FileChooser> fileChooser;
    noirnuit::StereoClip renderSource;
    noirnuit::Settings renderSettings;
    juce::File temporaryWav;
    std::atomic<int> jobResult {0}; // 0=idle/running, 1=ok, 2=empty, 3=error
    bool isRendering = false;
    juce::String status = "Lecture en continu : la piste est prete";
    float displayedBpm = 120.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoirnuitReverseAudioProcessorEditor)
};
