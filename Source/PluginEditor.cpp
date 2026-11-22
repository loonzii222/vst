#include "PluginEditor.h"
#include "BinaryData.h"
#include <algorithm>
#include <cmath>

namespace
{
const auto bg       = juce::Colour::fromRGB(8, 9, 14);
const auto panel    = juce::Colour::fromRGB(17, 17, 25);
const auto panel2   = juce::Colour::fromRGB(31, 29, 41);
const auto violet   = juce::Colour::fromRGB(166, 121, 238);
const auto lilac    = juce::Colour::fromRGB(220, 205, 253);
const auto muted    = juce::Colour::fromRGB(143, 139, 158);
const auto white    = juce::Colour::fromRGB(241, 237, 250);
const auto charcoal = juce::Colour::fromRGB(10, 10, 16);

void drawPanel(juce::Graphics& g, juce::Rectangle<float> r, float radius = 15.0f)
{
    g.setGradientFill(juce::ColourGradient(panel.withAlpha(0.96f), r.getX(), r.getY(),
                                          charcoal.withAlpha(0.97f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, radius);
    g.setColour(juce::Colours::white.withAlpha(0.105f));
    g.drawRoundedRectangle(r.reduced(0.5f), radius, 0.9f);
    g.setColour(violet.withAlpha(0.055f));
    g.drawRoundedRectangle(r.reduced(2.0f), radius - 1.0f, 1.0f);
}

void drawCaps(juce::Graphics& g, const juce::String& s, juce::Rectangle<int> r,
              float fontSize = 11.0f, juce::Colour c = muted,
              juce::Justification justification = juce::Justification::centredLeft)
{
    g.setColour(c);
    g.setFont(juce::Font(juce::FontOptions(fontSize, juce::Font::bold)));
    g.drawFittedText(s, r, justification, 1);
}
} // namespace

void NoirnuitLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPosProportional, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider&)
{
    const float size = std::min(static_cast<float>(width), static_cast<float>(height)) - 5.0f;
    const float radius = size * 0.40f;
    const float cx = x + width * 0.5f;
    const float cy = y + height * 0.49f;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // The ring has separate inactive and illuminated segments.
    juce::Path backgroundArc, illuminatedArc;
    backgroundArc.addCentredArc(cx, cy, radius + 7.0f, radius + 7.0f, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
    illuminatedArc.addCentredArc(cx, cy, radius + 7.0f, radius + 7.0f, 0.0f,
                                 rotaryStartAngle, angle, true);
    g.setColour(juce::Colour::fromRGB(71, 67, 84));
    g.strokePath(backgroundArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    g.setColour(violet.withAlpha(0.10f));
    g.strokePath(illuminatedArc, juce::PathStrokeType(12.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    g.setColour(lilac);
    g.strokePath(illuminatedArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

    const auto ring = juce::Rectangle<float>(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    g.setColour(juce::Colours::black.withAlpha(0.80f));
    g.fillEllipse(ring.translated(0.0f, 3.0f));
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(58, 57, 69), cx - radius * .8f, cy - radius,
                                          juce::Colour::fromRGB(13, 13, 20), cx + radius, cy + radius, false));
    g.fillEllipse(ring);
    g.setColour(juce::Colours::white.withAlpha(.17f));
    g.drawEllipse(ring.reduced(.7f), 1.1f);
    g.setColour(juce::Colours::black.withAlpha(.48f));
    g.drawEllipse(ring.reduced(5.0f), 1.5f);

    // Fine radial reflections make the knob look machined instead of flat.
    for (int i = 0; i < 34; ++i)
    {
        const float a = static_cast<float>(i) * juce::MathConstants<float>::twoPi / 34.0f;
        const float r1 = radius * .40f;
        const float r2 = radius * .91f;
        g.setColour(juce::Colours::white.withAlpha(i % 3 == 0 ? .018f : .007f));
        g.drawLine(cx + std::sin(a) * r1, cy - std::cos(a) * r1,
                   cx + std::sin(a) * r2, cy - std::cos(a) * r2, .6f);
    }
    const float pointerR1 = radius * .30f, pointerR2 = radius * .75f;
    g.setColour(juce::Colours::white.withAlpha(.10f));
    g.drawLine(cx + std::sin(angle) * pointerR1, cy - std::cos(angle) * pointerR1,
               cx + std::sin(angle) * pointerR2, cy - std::cos(angle) * pointerR2, 5.0f);
    g.setColour(white);
    g.drawLine(cx + std::sin(angle) * pointerR1, cy - std::cos(angle) * pointerR1,
               cx + std::sin(angle) * pointerR2, cy - std::cos(angle) * pointerR2, 1.8f);
}

void NoirnuitLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                               const juce::Colour& backgroundColour,
                                               bool isMouseOverButton, bool isButtonDown)
{
    const auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    const bool isActive = b.getToggleState();
    const bool prominent = backgroundColour == violet;
    auto fill1 = prominent ? juce::Colour::fromRGB(91, 60, 130) : panel2;
    auto fill2 = prominent ? juce::Colour::fromRGB(38, 28, 56) : panel;
    if (isActive) { fill1 = juce::Colour::fromRGB(98, 63, 143); fill2 = juce::Colour::fromRGB(44, 30, 68); }
    if (isMouseOverButton) { fill1 = fill1.brighter(.12f); fill2 = fill2.brighter(.12f); }
    if (isButtonDown) { fill1 = fill1.darker(.2f); fill2 = fill2.darker(.2f); }
    g.setGradientFill(juce::ColourGradient(fill1, r.getX(), r.getY(), fill2, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 9.0f);
    const auto edge = prominent || isActive ? violet.withAlpha(.72f) : juce::Colours::white.withAlpha(.17f);
    g.setColour(edge);
    g.drawRoundedRectangle(r.reduced(.5f), 9.0f, 1.0f);
    if (prominent || isActive)
    {
        g.setColour(violet.withAlpha(isMouseOverButton ? .23f : .11f));
        g.drawRoundedRectangle(r.expanded(.3f), 9.0f, 3.0f);
    }
}

void NoirnuitLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b,
                                         bool, bool)
{
    const auto label = b.getButtonText();
    const float fontSize = b.getHeight() >= 42 ? 12.5f : 10.8f;
    g.setFont(juce::Font(juce::FontOptions(fontSize, juce::Font::bold)));
    g.setColour(b.isEnabled() ? (b.getToggleState() ? white : lilac) : muted);
    g.drawFittedText(label, b.getLocalBounds().reduced(8, 3), juce::Justification::centred, 1);
}

void NoirnuitLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                       bool, int, int, int, int, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float>(1, 1, static_cast<float>(width - 2), static_cast<float>(height - 2));
    g.setGradientFill(juce::ColourGradient(panel2, 0, 0, panel, 0, static_cast<float>(height), false));
    g.fillRoundedRectangle(r, 8.0f);
    g.setColour(violet.withAlpha(.36f));
    g.drawRoundedRectangle(r, 8.0f, 1.0f);
    juce::Path arrow;
    const float x = width - 18.0f, y = height * .5f;
    arrow.startNewSubPath(x - 4, y - 2); arrow.lineTo(x, y + 2); arrow.lineTo(x + 4, y - 2);
    g.setColour(lilac);
    g.strokePath(arrow, juce::PathStrokeType(1.5f));
}
void NoirnuitLookAndFeel::positionComboBoxText(juce::ComboBox& b, juce::Label& label)
{
    label.setBounds(13, 1, b.getWidth() - 42, b.getHeight() - 2);
    label.setFont(juce::Font(juce::FontOptions(12.0f)));
}

NoirnuitReverseAudioProcessorEditor::DraggableButton::DraggableButton(NoirnuitReverseAudioProcessorEditor& o)
    : juce::TextButton("DRAG WAV"), editor(o) {}
void NoirnuitReverseAudioProcessorEditor::DraggableButton::mouseDown(const juce::MouseEvent& e)
{
    dragged = false;
    juce::TextButton::mouseDown(e);
}
void NoirnuitReverseAudioProcessorEditor::DraggableButton::mouseDrag(const juce::MouseEvent& e)
{
    if (!dragged && e.getDistanceFromDragStart() > 7)
    {
        dragged = true;
        editor.externalDrag();
    }
    else juce::TextButton::mouseDrag(e);
}

NoirnuitReverseAudioProcessorEditor::NoirnuitReverseAudioProcessorEditor(NoirnuitReverseAudioProcessor& p)
    : AudioProcessorEditor(&p), juce::Thread("NOIRNUIT render"), processor(p), dragButton(*this)
{
    setLookAndFeel(&skin);
    setSize(1180, 790);
    setOpaque(true);
    backdrop = juce::ImageCache::getFromMemory(BinaryData::noirnuit_surface_png,
                                               BinaryData::noirnuit_surface_pngSize);
    formatButton(captureButton, "●  CAPTURE", violet);
    formatButton(liveButton, "LIVE INPUT", panel2);
    formatButton(importButton, "IMPORT AUDIO", panel2);
    formatButton(renderButton, "✦  CREATE REVERSE", violet);
    formatButton(listenButton, "▶  PREVIEW", panel2);
    formatButton(exportButton, "↓  EXPORT WAV", violet);
    formatButton(savePresetButton, "SAVE", panel2);
    formatButton(loadPresetButton, "LOAD", panel2);
    formatButton(dragButton, "↗  DRAG WAV", panel2);
    formatButton(vocalButton, "VOICE MODE", panel2);
    formatButton(phoneButton, "PHONE", panel2);
    formatButton(chorusButton, "CHORUS", panel2);
    formatButton(driveButton, "DRIVE", panel2);
    formatButton(tremoloButton, "TREMOLO", panel2);
    for (auto* button : {&vocalButton, &phoneButton, &chorusButton, &driveButton, &tremoloButton})
        button->setClickingTogglesState(true);

    vocalAttach = std::make_unique<ButtonAtt>(processor.parameters, "vocal", vocalButton);
    phoneAttach = std::make_unique<ButtonAtt>(processor.parameters, "phone", phoneButton);
    chorusAttach = std::make_unique<ButtonAtt>(processor.parameters, "chorus", chorusButton);
    driveAttach = std::make_unique<ButtonAtt>(processor.parameters, "drive", driveButton);
    tremoloAttach = std::make_unique<ButtonAtt>(processor.parameters, "tremolo", tremoloButton);
    formatSlider(decaySlider, "decay", decayAttach);
    formatSlider(toneSlider, "tone", toneAttach);
    formatSlider(widthSlider, "width", widthAttach);
    formatSlider(sibilanceSlider, "sibilance", sibilanceAttach);

    lengthChoice.addItemList({"1 BAR", "2 BARS", "4 BARS", "8 BARS"}, 1);
    addAndMakeVisible(lengthChoice);
    lengthAttach = std::make_unique<NoirnuitReverseAudioProcessor::APVTS::ComboBoxAttachment>(processor.parameters, "bars", lengthChoice);
    factoryPresets.addItemList({"Vocal / Clean", "Dream / Wide", "Dark / Telephone", "Glitch / Rhythm"}, 1);
    addAndMakeVisible(factoryPresets);
    factoryPresets.setSelectedId(1, juce::dontSendNotification);
    factoryPresets.onChange = [this] { updatePreset(factoryPresets.getSelectedId()); };

    captureButton.onClick = [this]
    {
        auto source = processor.snapshotLiveAudio(12.0);
        if (!source.valid()) { setStatus("No audio: play your track first"); return; }
        processor.setCapturedSource(std::move(source));
        setStatus("Captured. Press CREATE REVERSE to render.");
    };
    liveButton.onClick = [this] { processor.useLiveInput(); setStatus("Live input enabled."); };
    importButton.onClick = [this] { openImportChooser(); };
    renderButton.onClick = [this] { startRender(); };
    listenButton.onClick = [this]
    {
        if (processor.isPreviewing()) { processor.stopPreview(); setStatus("Preview stopped."); }
        else if (processor.getRendered()) { processor.startPreview(); setStatus("Play the FL Studio transport to hear the preview."); }
        else setStatus("Render a reverse before previewing.");
    };
    exportButton.onClick = [this] { openExportChooser(); };
    dragButton.onClick = [this] { setStatus("Hold and drag DRAG WAV into the FL Studio playlist."); };
    savePresetButton.onClick = [this] { openSavePresetChooser(); };
    loadPresetButton.onClick = [this] { openLoadPresetChooser(); };
    displayedBpm = static_cast<float>(processor.getBpm());
    vocalButton.setButtonText(processor.parameters.getRawParameterValue("vocal")->load() > .5f ? "VOICE MODE" : "INSTRUMENT");
    startTimerHz(12);
}

NoirnuitReverseAudioProcessorEditor::~NoirnuitReverseAudioProcessorEditor()
{
    stopTimer();
    signalThreadShouldExit();
    stopThread(15000);
    setLookAndFeel(nullptr);
}

void NoirnuitReverseAudioProcessorEditor::formatButton(juce::TextButton& b, const juce::String& text, juce::Colour base)
{
    b.setButtonText(text);
    b.setColour(juce::TextButton::buttonColourId, base);
    b.setColour(juce::TextButton::buttonOnColourId, violet);
    b.setColour(juce::TextButton::textColourOffId, white);
    b.setColour(juce::TextButton::textColourOnId, white);
    b.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(b);
}

void NoirnuitReverseAudioProcessorEditor::formatSlider(juce::Slider& s, const juce::String& id,
                                                       std::unique_ptr<SliderAtt>& att)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 75, 23);
    s.setColour(juce::Slider::textBoxTextColourId, white);
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    s.setDoubleClickReturnValue(true, id == "width" ? 1.15 : (id == "tone" ? .65 : (id == "decay" ? .76 : .72)));
    addAndMakeVisible(s);
    att = std::make_unique<SliderAtt>(processor.parameters, id, s);
}

void NoirnuitReverseAudioProcessorEditor::resized()
{
    factoryPresets.setBounds(875, 27, 158, 34);
    savePresetButton.setBounds(1041, 27, 52, 34);
    loadPresetButton.setBounds(1100, 27, 53, 34);
    captureButton.setBounds(46, 193, 201, 45);
    liveButton.setBounds(45, 263, 98, 33);
    importButton.setBounds(150, 263, 98, 33);
    lengthChoice.setBounds(47, 331, 200, 36);
    decaySlider.setBounds(92, 421, 156, 106);
    toneSlider.setBounds(369, 421, 156, 106);
    widthSlider.setBounds(646, 421, 156, 106);
    sibilanceSlider.setBounds(923, 421, 156, 106);
    for (int i = 0; i < 5; ++i)
    {
        auto* b = std::array<juce::TextButton*, 5>{ &vocalButton, &phoneButton, &chorusButton, &driveButton, &tremoloButton }[static_cast<std::size_t>(i)];
        b->setBounds(42 + i * 226, 562, 184, 35);
    }
    renderButton.setBounds(45, 704, 228, 51);
    listenButton.setBounds(282, 704, 131, 51);
    dragButton.setBounds(422, 704, 159, 51);
    exportButton.setBounds(944, 704, 205, 51);
}

void NoirnuitReverseAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    if (backdrop.isValid()) g.drawImageAt(backdrop, 0, 0);

    // Minimal cinematic header: editorial typography + precise utility controls.
    g.setColour(violet.withAlpha(.9f));
    g.setFont(juce::Font(juce::FontOptions("Georgia", 26.0f, juce::Font::plain)));
    g.drawText("✦", 29, 23, 33, 44, juce::Justification::centred);
    drawCaps(g, "N O I R N U I T   A U D I O", {69, 38, 260, 22}, 11.0f, lilac);
    g.setFont(juce::Font(juce::FontOptions("Georgia", 39.0f, juce::Font::plain)));
    g.setColour(white);
    g.drawText("NOIRNUIT", 408, 12, 358, 51, juce::Justification::centred);
    g.setColour(lilac);
    g.setFont(juce::Font(juce::FontOptions(19.0f)));
    g.drawText("R  E  V  E  R  S  E", 405, 57, 365, 29, juce::Justification::centred);
    drawCaps(g, "R E V E R S E   R E V E R B   D E S I G N E R", {387, 93, 404, 18}, 9.0f, muted,
             juce::Justification::centred);
    g.setColour(juce::Colours::white.withAlpha(.10f));
    g.drawLine(27.0f, 121.0f, 1153.0f, 121.0f, 1.0f);

    drawPanel(g, {28.0f, 140.0f, 235.0f, 237.0f});
    drawPanel(g, {278.0f, 140.0f, 874.0f, 237.0f});
    drawCaps(g, "S O U R C E   /   C A P T U R E", {45, 157, 203, 20}, 10.0f, muted);
    drawCaps(g, processor.hasSource() ? "●  AUDIO CAPTURED" : "●  MONITORING INPUT",
             {49, 244, 195, 19}, 10.0f, processor.hasSource() ? lilac : muted);
    drawCaps(g, "R I S E   L E N G T H", {47, 307, 170, 18}, 10.0f, muted);

    drawCaps(g, "R E V E R S E   F I E L D", {303, 158, 350, 20}, 11.0f, lilac);
    drawCaps(g, "SYNC  /  " + juce::String(displayedBpm, 1) + " BPM", {922, 157, 207, 22}, 10.0f, muted,
             juce::Justification::centredRight);
    const auto time = processor.getRendered() ? "RENDER READY" : "READY TO CAPTURE";
    drawCaps(g, time, {303, 347, 290, 22}, 10.0f, processor.getRendered() ? lilac : muted);
    paintWaveform(g, {302.0f, 193.0f, 825.0f, 148.0f});

    drawPanel(g, {28.0f, 393.0f, 1124.0f, 151.0f});
    const juce::String mainLabels[] = {"DECAY", "TONE", "WIDTH", "DE-ESS"};
    const juce::String mainDescriptions[] = {"REVERB TAIL", "DARK ↔ BRIGHT", "STEREO IMAGE", "SIBILANCE CONTROL"};
    for (int i = 0; i < 4; ++i)
    {
        const int cx = 169 + i * 277;
        drawCaps(g, mainLabels[i], {cx - 99, 400, 198, 20}, 12.0f, white, juce::Justification::centred);
        drawCaps(g, mainDescriptions[i], {cx - 105, 526, 210, 15}, 9.0f, muted, juce::Justification::centred);
        if (i != 0)
        {
            g.setColour(juce::Colours::white.withAlpha(.10f));
            g.drawLine(cx - 139.0f, 419.0f, cx - 139.0f, 525.0f, .8f);
        }
    }

    // Five cards directly correspond to five working AudioProcessor parameters.
    const juce::String labels[] = {"VOCAL / INSTRUMENT", "FILTERED TEXTURE", "WIDE MODULATION", "SATURATION", "RHYTHMIC PULSE"};
    for (int i = 0; i < 5; ++i)
    {
        const auto r = juce::Rectangle<float>(28.0f + 226.0f * i, 553.0f, 214.0f, 124.0f);
        drawPanel(g, r, 12.0f);
        const float cx = r.getCentreX();
        juce::Path line;
        const int cols = 65;
        for (int k = 0; k < cols; ++k)
        {
            const float t = static_cast<float>(k) / (cols - 1);
            const float envelope = std::pow(std::max(0.0f, 1.0f - std::abs(t - .5f) * 2.0f), 1.4f);
            const float wave = (i == 3 ? std::tanh(2.5f * std::sin(t * 23.0f)) :
                               i == 4 ? std::sin(t * 20.0f) :
                               std::sin(t * (27.0f + i * 5.0f)) * std::cos(t * 11.0f));
            const float px = r.getX() + 20.0f + t * (r.getWidth() - 40.0f);
            const float py = 627.0f - wave * (5.5f + envelope * 12.0f);
            if (k == 0) line.startNewSubPath(px, py); else line.lineTo(px, py);
        }
        g.setColour(violet.withAlpha(.17f));
        g.strokePath(line, juce::PathStrokeType(5.0f));
        g.setColour(lilac.withAlpha(.65f));
        g.strokePath(line, juce::PathStrokeType(1.0f));
        drawCaps(g, labels[i], {static_cast<int>(cx - 100), 649, 200, 17}, 8.6f, muted,
                 juce::Justification::centred);
    }

    drawPanel(g, {28.0f, 687.0f, 1124.0f, 84.0f}, 13.0f);
    g.setColour(violet.withAlpha(.38f));
    g.drawLine(596.0f, 705.0f, 596.0f, 750.0f, 1.0f);
    drawCaps(g, "S T A T U S", {608, 703, 100, 17}, 9.0f, muted);
    g.setColour(isRendering ? lilac : white.withAlpha(.80f));
    g.setFont(juce::Font(juce::FontOptions(11.2f)));
    g.drawFittedText(status, 608, 723, 317, 31, juce::Justification::centredLeft, 2);
    drawCaps(g, "N O I R N U I T   /   V 2 . 0", {954, 774, 198, 12}, 8.0f, muted,
             juce::Justification::centredRight);
}

void NoirnuitReverseAudioProcessorEditor::paintWaveform(juce::Graphics& g, juce::Rectangle<float> rect)
{
    const float cy = rect.getCentreY();
    g.setColour(juce::Colours::white.withAlpha(.065f));
    g.drawLine(rect.getX(), cy, rect.getRight(), cy, 1.0f);
    g.setColour(violet.withAlpha(.10f));
    g.drawLine(rect.getX(), cy - 27.0f, rect.getRight(), cy - 27.0f, .6f);
    g.drawLine(rect.getX(), cy + 27.0f, rect.getRight(), cy + 27.0f, .6f);
    const auto rendered = processor.getRendered();
    const int count = rendered ? rendered->getNumSamples() : 0;
    constexpr int columns = 278;
    const float phase = animationPhase;
    for (int k = 0; k < columns; ++k)
    {
        const float t = static_cast<float>(k) / (columns - 1);
        float amp = 0.0f;
        if (rendered && count > 0)
        {
            const int begin = static_cast<int>(static_cast<int64_t>(k) * count / columns);
            const int end = static_cast<int>(static_cast<int64_t>(k + 1) * count / columns);
            const int step = std::max(1, (end - begin) / 11);
            for (int n = begin; n < end; n += step)
                amp = std::max(amp, .5f * (std::abs(rendered->getSample(0, n)) +
                                               std::abs(rendered->getSample(1, n))));
            amp = std::min(1.0f, amp * 1.75f);
        }
        else
        {
            const float envelope = std::pow(t, 1.9f);
            const float texture = std::abs(std::sin(k * 1.93f) * std::cos(k * .47f))
                                  + .30f * std::abs(std::sin(k * 3.47f + phase));
            amp = .012f + envelope * (.10f + texture * .75f);
        }
        const float h = std::max(1.4f, (rect.getHeight() - 18.0f) * amp);
        const float x = rect.getX() + t * rect.getWidth();
        const float glow = .12f + .36f * t;
        g.setColour(violet.withAlpha(glow * .18f));
        g.drawLine(x, cy - h * .55f, x, cy + h * .55f, 5.5f);
        g.setColour(lilac.withAlpha(.30f + .68f * t));
        g.drawLine(x, cy - h * .5f, x, cy + h * .5f, 1.5f);
    }
    g.setColour(lilac.withAlpha(.75f));
    g.drawLine(rect.getRight() - 1.0f, rect.getY() + 10.0f,
               rect.getRight() - 1.0f, rect.getBottom() - 10.0f, 1.3f);
}

void NoirnuitReverseAudioProcessorEditor::setStatus(const juce::String& s)
{
    status = s;
    repaint();
}

void NoirnuitReverseAudioProcessorEditor::startRender()
{
    if (isRendering || isThreadRunning()) return;
    renderSource = processor.snapshotSource();
    if (!renderSource.valid()) { setStatus("Pas de signal : lis la piste ou importe un WAV"); return; }
    renderSettings = processor.currentSettings();
    isRendering = true;
    jobResult.store(0);
    renderButton.setEnabled(false);
    renderButton.setButtonText("PROCESSING...");
    setStatus("Calcul de la reverb inversee...");
    startThread();
}

void NoirnuitReverseAudioProcessorEditor::run()
{
    auto result = noirnuit::createReverseReverb(renderSource, renderSettings);
    if (!result.valid()) { jobResult.store(2); return; }
    processor.storeRendered(std::move(result));
    temporaryWav = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("NOIRNUIT_REVERSE_TEMP.wav");
    temporaryWav.deleteFile();
    jobResult.store(processor.writeRenderedWav(temporaryWav) ? 1 : 3,
                    std::memory_order_release);
}

void NoirnuitReverseAudioProcessorEditor::timerCallback()
{
    const auto bpm = static_cast<float>(processor.getBpm());
    const auto modeText = processor.parameters.getRawParameterValue("vocal")->load() > 0.5f ? "VOICE MODE" : "INSTRUMENT";
    if (vocalButton.getButtonText() != modeText) vocalButton.setButtonText(modeText);
    if (std::abs(bpm - displayedBpm) > 0.01f) { displayedBpm = bpm; repaint(); }
    animationPhase += 0.16f;
    if (!processor.getRendered() || processor.isPreviewing()) repaint(302, 193, 825, 148);
    const auto listenText = processor.isPreviewing() ? "■  STOP" : "▶  PREVIEW";
    if (listenButton.getButtonText() != listenText) listenButton.setButtonText(listenText);
    if (isRendering && !isThreadRunning())
    {
        isRendering = false;
        renderButton.setEnabled(true);
        renderButton.setButtonText("✦  CREATE REVERSE");
        const int result = jobResult.load(std::memory_order_acquire);
        if (result == 1) setStatus("Reverse pret : glisse le WAV vers FL Studio");
        else if (result == 2) setStatus("Signal audio non valide");
        else setStatus("Rendu pret, mais export temporaire impossible");
        repaint();
    }
}

void NoirnuitReverseAudioProcessorEditor::externalDrag()
{
    if (isRendering) { setStatus("Le rendu est encore en cours"); return; }
    if (!temporaryWav.existsAsFile()) { setStatus("Genere le reverse avant de le glisser"); return; }
    const juce::StringArray files {temporaryWav.getFullPathName()};
    const bool started = juce::DragAndDropContainer::performExternalDragDropOfFiles(files, false, this);
    if (!started) setStatus("Glisser-deposer non pris en charge ici : utilise EXPORTER WAV");
}

bool NoirnuitReverseAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        const auto ext = juce::File(file).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aiff" || ext == ".aif" || ext == ".mp3") return true;
    }
    return false;
}
void NoirnuitReverseAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (const auto& file : files)
        if (isInterestedInFileDrag({file})) { loadAudioFile(juce::File(file)); break; }
}

void NoirnuitReverseAudioProcessorEditor::loadAudioFile(const juce::File& file)
{
    if (isRendering) return;
    juce::String message;
    processor.importWav(file, message);
    setStatus(message);
}

void NoirnuitReverseAudioProcessorEditor::openImportChooser()
{
    if (isRendering) return;
    fileChooser = std::make_unique<juce::FileChooser>("Importer une voix / instrument", juce::File(), "*.wav;*.aif;*.aiff;*.mp3");
    juce::Component::SafePointer<NoirnuitReverseAudioProcessorEditor> safe(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
       [safe](const juce::FileChooser& chooser)
       { if (safe != nullptr && chooser.getResult().existsAsFile()) safe->loadAudioFile(chooser.getResult()); });
}

void NoirnuitReverseAudioProcessorEditor::openExportChooser()
{
    if (!processor.getRendered()) { setStatus("Genere d'abord un reverse"); return; }
    fileChooser = std::make_unique<juce::FileChooser>("Exporter le riser au format WAV 24 bits", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("NOIRNUIT_REVERSE.wav"), "*.wav");
    juce::Component::SafePointer<NoirnuitReverseAudioProcessorEditor> safe(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
       [safe](const juce::FileChooser& chooser)
       {
           if (safe == nullptr) return;
           auto target = chooser.getResult();
           if (target.getFullPathName().isEmpty()) return;
           target = target.withFileExtension("wav");
           target.deleteFile();
           safe->setStatus(safe->processor.writeRenderedWav(target) ? "Export WAV reussi" : "Export WAV echoue");
       });
}

void NoirnuitReverseAudioProcessorEditor::openSavePresetChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>("Enregistrer preset NOIRNUIT", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Noirnuit_Preset.nnr"), "*.nnr");
    juce::Component::SafePointer<NoirnuitReverseAudioProcessorEditor> safe(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
       [safe](const juce::FileChooser& fc)
       {
           if (safe == nullptr || fc.getResult().getFullPathName().isEmpty()) return;
           if (auto xml = safe->processor.parameters.copyState().createXml())
           {
               const bool ok = fc.getResult().withFileExtension("nnr").replaceWithText(xml->toString());
               safe->setStatus(ok ? "Preset sauvegarde" : "Sauvegarde du preset echouee");
           }
       });
}

void NoirnuitReverseAudioProcessorEditor::openLoadPresetChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>("Charger preset NOIRNUIT", juce::File(), "*.nnr");
    juce::Component::SafePointer<NoirnuitReverseAudioProcessorEditor> safe(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
       [safe](const juce::FileChooser& fc)
       {
           if (safe == nullptr || !fc.getResult().existsAsFile()) return;
           if (auto xml = juce::XmlDocument::parse(fc.getResult()))
           {
               const auto tree = juce::ValueTree::fromXml(*xml);
               if (tree.isValid() && tree.hasType("NoirnuitState"))
               {
                   safe->processor.parameters.replaceState(tree);
                   safe->setStatus("Preset charge");
                   return;
               }
           }
           safe->setStatus("Preset .nnr non valide");
       });
}

void NoirnuitReverseAudioProcessorEditor::setParam(const juce::String& id, float value)
{
    if (auto* p = processor.parameters.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(value));
}
void NoirnuitReverseAudioProcessorEditor::updatePreset(int idx)
{
    if (idx == 1)
    {
        setParam("decay", 0.76f); setParam("tone", 0.72f); setParam("width", 1.15f);
        setParam("sibilance", 0.77f); setParam("phone", 0.f);setParam("chorus", 0.f);
        setParam("drive", 0.f);setParam("tremolo", 0.f);setParam("vocal", 1.f);
    }
    else if (idx == 2)
    {
        setParam("decay", 0.94f);setParam("tone", 0.66f);setParam("width", 1.52f);
        setParam("sibilance", 0.6f);setParam("phone", 0.f);setParam("chorus", 1.f);
        setParam("drive", 0.f);setParam("tremolo", 0.f);
    }
    else if (idx == 3)
    {
        setParam("decay", 0.88f);setParam("tone", 0.28f);setParam("width", 0.75f);
        setParam("sibilance", 0.9f);setParam("phone", 1.f);setParam("chorus", 0.f);
        setParam("drive", 0.f);setParam("tremolo", 0.f);
    }
    else if (idx == 4)
    {
        setParam("decay", 0.54f);setParam("tone", 0.76f);setParam("width", 1.40f);
        setParam("sibilance", 0.45f);setParam("phone", 0.f);setParam("chorus", 0.f);
        setParam("drive", 1.f);setParam("tremolo", 1.f);
    }
    setStatus("Preset modifie : genere pour ecouter");
}
