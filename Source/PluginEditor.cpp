#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace
{
const auto bg      = juce::Colour::fromRGB(11, 11, 17);
const auto panel   = juce::Colour::fromRGB(22, 22, 31);
const auto panel2  = juce::Colour::fromRGB(31, 28, 43);
const auto violet  = juce::Colour::fromRGB(159, 116, 255);
const auto lilac   = juce::Colour::fromRGB(201, 185, 255);
const auto gold    = juce::Colour::fromRGB(242, 174, 96);
const auto muted   = juce::Colour::fromRGB(142, 140, 155);
const auto white   = juce::Colour::fromRGB(242, 239, 252);
}

NoirnuitReverseAudioProcessorEditor::DraggableButton::DraggableButton(NoirnuitReverseAudioProcessorEditor& o)
    : juce::TextButton("GLISSER WAV"), editor(o) {}
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
    setSize(990, 720);
    setOpaque(true);
    formatButton(captureButton, "CAPTURER", violet);
    formatButton(liveButton, "ENTREE LIVE", panel2);
    formatButton(importButton, "IMPORTER AUDIO", panel2);
    formatButton(renderButton, "CREER LE REVERSE", violet);
    formatButton(listenButton, "ECOUTER", panel2);
    formatButton(exportButton, "EXPORTER WAV", panel2);
    formatButton(savePresetButton, "+ PRESET", panel2);
    formatButton(loadPresetButton, "IMPORT PRESET", panel2);
    formatButton(dragButton, "GLISSER WAV", gold);
    formatButton(vocalButton, "MODE VOIX", panel2);
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

    lengthChoice.addItemList({"1 MESURE", "2 MESURES", "4 MESURES", "8 MESURES"}, 1);
    addAndMakeVisible(lengthChoice);
    lengthChoice.setColour(juce::ComboBox::backgroundColourId, panel2);
    lengthChoice.setColour(juce::ComboBox::textColourId, white);
    lengthChoice.setColour(juce::ComboBox::outlineColourId, violet.withAlpha(0.45f));
    lengthAttach = std::make_unique<NoirnuitReverseAudioProcessor::APVTS::ComboBoxAttachment>(processor.parameters, "bars", lengthChoice);

    factoryPresets.addItemList({"Vocal / Clean", "Dream / Wide", "Dark / Telephone", "Glitch / Rhythm"}, 1);
    addAndMakeVisible(factoryPresets);
    factoryPresets.setSelectedId(1, juce::dontSendNotification);
    factoryPresets.setColour(juce::ComboBox::backgroundColourId, panel2);
    factoryPresets.setColour(juce::ComboBox::textColourId, white);
    factoryPresets.setColour(juce::ComboBox::outlineColourId, panel2);
    factoryPresets.onChange = [this] { updatePreset(factoryPresets.getSelectedId()); };

    captureButton.onClick = [this]
    {
        auto source = processor.snapshotLiveAudio(12.0);
        if (!source.valid()) { setStatus("Aucun signal : lance la lecture de la piste"); return; }
        processor.setCapturedSource(std::move(source));
        setStatus("Capture figee : genere le reverse");
    };
    liveButton.onClick = [this]
    {
        processor.useLiveInput();
        setStatus("Entree live : lecture de la piste en continu");
    };
    importButton.onClick = [this] { openImportChooser(); };
    renderButton.onClick = [this] { startRender(); };
    listenButton.onClick = [this]
    {
        if (processor.isPreviewing()) { processor.stopPreview(); setStatus("Pre-ecoute arretee"); }
        else if (processor.getRendered()) { processor.startPreview(); setStatus("Pre-ecoute : lance la lecture du DAW"); }
        else setStatus("Genere un reverse avant d'ecouter");
    };
    exportButton.onClick = [this] { openExportChooser(); };
    dragButton.onClick = [this] { setStatus("Maintiens et glisse GLISSER WAV vers la playlist"); };
    savePresetButton.onClick = [this] { openSavePresetChooser(); };
    loadPresetButton.onClick = [this] { openLoadPresetChooser(); };
    displayedBpm = static_cast<float>(processor.getBpm());
    vocalButton.setButtonText(processor.parameters.getRawParameterValue("vocal")->load() > 0.5f ? "MODE VOIX" : "MODE INSTRUMENT");
    startTimerHz(12);
}

NoirnuitReverseAudioProcessorEditor::~NoirnuitReverseAudioProcessorEditor()
{
    stopTimer();
    signalThreadShouldExit();
    stopThread(15000);
}

void NoirnuitReverseAudioProcessorEditor::formatButton(juce::TextButton& b, const juce::String& text, juce::Colour base)
{
    b.setButtonText(text);
    b.setColour(juce::TextButton::buttonColourId, base);
    b.setColour(juce::TextButton::buttonOnColourId, violet);
    b.setColour(juce::TextButton::textColourOffId, base == violet ? bg : white);
    b.setColour(juce::TextButton::textColourOnId, bg);
    addAndMakeVisible(b);
}

void NoirnuitReverseAudioProcessorEditor::formatSlider(juce::Slider& s, const juce::String& id, std::unique_ptr<SliderAtt>& att)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 24);
    s.setColour(juce::Slider::rotarySliderFillColourId, violet);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, panel2);
    s.setColour(juce::Slider::thumbColourId, white);
    s.setColour(juce::Slider::textBoxTextColourId, white);
    s.setColour(juce::Slider::textBoxBackgroundColourId, panel);
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(s);
    att = std::make_unique<SliderAtt>(processor.parameters, id, s);
}

void NoirnuitReverseAudioProcessorEditor::resized()
{
    factoryPresets.setBounds(610, 26, 170, 32);
    savePresetButton.setBounds(792, 26, 79, 32);
    loadPresetButton.setBounds(879, 26, 85, 32);
    vocalButton.setBounds(830, 102, 123, 31);
    captureButton.setBounds(38, 357, 138, 39);
    liveButton.setBounds(184, 357, 142, 39);
    importButton.setBounds(334, 357, 169, 39);
    lengthChoice.setBounds(660, 364, 266, 39);
    decaySlider.setBounds(54, 473, 210, 111);
    toneSlider.setBounds(280, 473, 210, 111);
    widthSlider.setBounds(506, 473, 210, 111);
    sibilanceSlider.setBounds(732, 473, 210, 111);
    phoneButton.setBounds(40, 607, 151, 36);
    chorusButton.setBounds(199, 607, 151, 36);
    driveButton.setBounds(358, 607, 151, 36);
    tremoloButton.setBounds(517, 607, 151, 36);
    renderButton.setBounds(38, 659, 230, 40);
    listenButton.setBounds(279, 659, 119, 40);
    dragButton.setBounds(407, 659, 155, 40);
    exportButton.setBounds(571, 659, 160, 40);
}

void NoirnuitReverseAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    g.setColour(panel);
    g.fillRoundedRectangle(22.0f, 82.0f, 946.0f, 253.0f, 19.0f);
    g.setColour(panel2);
    g.drawRoundedRectangle(22.5f, 82.5f, 945.0f, 252.0f, 19.0f, 1.0f);
    g.setColour(violet);
    g.setFont(juce::Font(juce::FontOptions(26.0f, juce::Font::bold)));
    g.drawText("NOIRNUIT", 36, 18, 172, 39, juce::Justification::centredLeft);
    g.setColour(white);
    g.drawText("REVERSE", 205, 18, 170, 39, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("VOCAL SWELLS / AUDIO TO REVERSE REVERB", 37, 58, 470, 20, juce::Justification::centredLeft);
    g.setColour(lilac);
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.drawText("REVERSE FIELD", 48, 106, 270, 28, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("La texture monte jusqu'au debut de la phrase", 49, 132, 490, 22, juce::Justification::centredLeft);
    paintWaveform(g, {50.0f, 169.0f, 890.0f, 131.0f});
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("ENTREE : " + juce::String(processor.hasSource() ? "CAPTURE / FICHIER" : "LIVE") , 52, 298, 230, 24, juce::Justification::centredLeft);
    g.drawText("TEMPO DU DAW : " + juce::String(displayedBpm, 1) + " BPM", 628, 299, 310, 24, juce::Justification::centredRight);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText("SOURCE", 41, 337, 150, 21, juce::Justification::centredLeft);
    g.drawText("DUREE DE LA MONTEE (4/4)", 660, 339, 280, 24, juce::Justification::centredLeft);
    g.setColour(panel);
    g.fillRoundedRectangle(24.0f, 439.0f, 942.0f, 154.0f, 16.0f);
    g.setColour(lilac);
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    const char* names[] = {"DECAY", "TONE", "WIDTH", "DE-ESS"};
    for (int i = 0; i < 4; ++i)
        g.drawText(names[i], 54 + i * 226, 448, 210, 22, juce::Justification::centred);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("TEXTURES", 40, 587, 140, 19, juce::Justification::centredLeft);
    g.drawText("V1.0 | © NOIRNUIT AUDIO", 756, 608, 201, 30, juce::Justification::centredRight);
    g.setColour(juce::Colour::fromRGB(77, 74, 95));
    g.drawHorizontalLine(649, 26.0f, 964.0f);
    g.setColour(isRendering ? gold : muted);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawFittedText(status, 747, 655, 215, 48, juce::Justification::centredLeft, 2);
}

void NoirnuitReverseAudioProcessorEditor::paintWaveform(juce::Graphics& g, juce::Rectangle<float> rect)
{
    const float centreY = rect.getCentreY();
    g.setColour(violet.withAlpha(0.13f));
    g.fillRoundedRectangle(rect, 11.0f);
    auto rendered = processor.getRendered();
    const int count = rendered ? rendered->getNumSamples() : 0;
    const int columns = 210;
    for (int k = 0; k < columns; ++k)
    {
        float amp = 0.0f;
        if (rendered && count > 0)
        {
            const int begin = static_cast<int>(static_cast<int64_t>(k) * count / columns);
            const int end = static_cast<int>(static_cast<int64_t>(k + 1) * count / columns);
            for (int n = begin; n < end; n += std::max(1, (end - begin) / 8))
                amp = std::max(amp, 0.5f * (std::abs(rendered->getSample(0, n)) + std::abs(rendered->getSample(1, n))));
            amp = std::min(1.0f, amp * 1.8f);
        }
        else
        {
            const float progress = static_cast<float>(k) / columns;
            amp = 0.05f + std::pow(progress, 1.7f) * (0.42f + 0.11f * std::sin(static_cast<float>(k) * 0.43f));
        }
        const float height = std::max(2.0f, (rect.getHeight() - 16.0f) * amp);
        const float x = rect.getX() + (k + 0.5f) * rect.getWidth() / columns;
        g.setColour(juce::Colour::fromFloatRGBA(0.60f + 0.2f * k / columns, 0.35f + 0.2f * k / columns, 1.0f, 0.35f + 0.65f * k / columns));
        g.drawLine(x, centreY - height * 0.5f, x, centreY + height * 0.5f, 2.2f);
    }
    g.setColour(lilac.withAlpha(0.45f));
    g.drawLine(rect.getRight() - 4.0f, rect.getY() + 8.0f, rect.getRight() - 4.0f, rect.getBottom() - 8.0f, 1.3f);
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
    const auto modeText = processor.parameters.getRawParameterValue("vocal")->load() > 0.5f ? "MODE VOIX" : "MODE INSTRUMENT";
    if (vocalButton.getButtonText() != modeText) vocalButton.setButtonText(modeText);
    if (std::abs(bpm - displayedBpm) > 0.01f) { displayedBpm = bpm; repaint(); }
    if (isRendering && !isThreadRunning())
    {
        isRendering = false;
        renderButton.setEnabled(true);
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
