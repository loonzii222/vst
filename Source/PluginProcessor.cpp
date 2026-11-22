#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <thread>

NoirnuitReverseAudioProcessor::NoirnuitReverseAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "NoirnuitState", createParameterLayout())
{}

NoirnuitReverseAudioProcessor::APVTS::ParameterLayout NoirnuitReverseAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("bars", 1), "Length", juce::StringArray{"1", "2", "4", "8"}, 2));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("decay", 1), "Decay", 0.0f, 1.0f, 0.76f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("tone", 1), "Tone", 0.0f, 1.0f, 0.65f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("width", 1), "Stereo width", 0.0f, 1.8f, 1.15f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sibilance", 1), "De-ess", 0.0f, 1.0f, 0.72f));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("vocal", 1), "Vocal mode", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("phone", 1), "Phone", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("chorus", 1), "Chorus", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("drive", 1), "Drive", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("tremolo", 1), "Tremolo", false));
    return layout;
}

void NoirnuitReverseAudioProcessor::prepareToPlay(double sr, int)
{
    ringFrozen.store(true);
    while (activeWriter.load() != 0) std::this_thread::yield();
    currentSampleRate.store(static_cast<int>(sr));
    const int maxFrames = static_cast<int>(std::ceil(sr * 45.0));
    ring = std::make_unique<juce::AudioBuffer<float>>(2, maxFrames);
    ring->clear();
    ringHead.store(0);
    ringWritten.store(0);
    previewPosition.store(0);
    previewActive.store(false);
    ringFrozen.store(false);
}

void NoirnuitReverseAudioProcessor::releaseResources()
{
    previewActive.store(false);
    ringFrozen.store(true);
    while (activeWriter.load() != 0) std::this_thread::yield();
    ring.reset();
    ringFrozen.store(false);
}

bool NoirnuitReverseAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto input = l.getMainInputChannelSet();
    const auto output = l.getMainOutputChannelSet();
    return (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())
         && input == output;
}

void NoirnuitReverseAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (auto* playHead = getPlayHead())
        if (auto p = playHead->getPosition())
            if (auto b = p->getBpm())
                if (*b >= 30.0 && *b <= 300.0) hostBpm.store(*b, std::memory_order_relaxed);

    activeWriter.fetch_add(1, std::memory_order_acq_rel);
    if (!ringFrozen.load(std::memory_order_acquire) && ring != nullptr)
    {
        const int capacity = ring->getNumSamples();
        int write = ringHead.load(std::memory_order_relaxed);
        const int frames = buffer.getNumSamples();
        const int inCh = buffer.getNumChannels();
        for (int i = 0; i < frames; ++i)
        {
            ring->setSample(0, write, buffer.getSample(0, i));
            ring->setSample(1, write, buffer.getSample(inCh > 1 ? 1 : 0, i));
            if (++write == capacity) write = 0;
        }
        ringHead.store(write, std::memory_order_relaxed);
        ringWritten.fetch_add(frames, std::memory_order_release);
    }
    activeWriter.fetch_sub(1, std::memory_order_release);

    // The original track is passed through unchanged, except when LISTEN is active.
    if (previewActive.load(std::memory_order_relaxed))
    {
        auto wav = getRendered();
        if (wav)
        {
            const int64_t start = previewPosition.fetch_add(buffer.getNumSamples());
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const int64_t pos = start + i;
                if (pos >= wav->getNumSamples()) { previewActive.store(false); break; }
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    buffer.addSample(ch, i, wav->getSample(std::min(ch, wav->getNumChannels() - 1), static_cast<int>(pos)) * 0.85f);
            }
        }
        else previewActive.store(false);
    }
}

noirnuit::StereoClip NoirnuitReverseAudioProcessor::snapshotLiveAudio(double seconds)
{
    // Hold the ring still for a short copy without locking the real-time thread.
    ringFrozen.store(true, std::memory_order_release);
    while (activeWriter.load(std::memory_order_acquire) != 0) std::this_thread::yield();
    noirnuit::StereoClip out;
    out.sampleRate = currentSampleRate.load();
    if (ring && ringWritten.load() > 0)
    {
        const int capacity = ring->getNumSamples();
        const auto desired = static_cast<int64_t>(std::ceil(seconds * out.sampleRate));
        const int n = static_cast<int>(std::min<int64_t>({desired, capacity, ringWritten.load()}));
        out.left.resize(static_cast<std::size_t>(n));
        out.right.resize(static_cast<std::size_t>(n));
        const int end = ringHead.load();
        const int begin = (end - n + capacity) % capacity;
        for (int i = 0; i < n; ++i)
        {
            const int idx = (begin + i) % capacity;
            out.left[static_cast<std::size_t>(i)] = ring->getSample(0, idx);
            out.right[static_cast<std::size_t>(i)] = ring->getSample(1, idx);
        }
    }
    ringFrozen.store(false, std::memory_order_release);
    return out;
}

noirnuit::StereoClip NoirnuitReverseAudioProcessor::snapshotSource()
{
    {
        const std::lock_guard<std::mutex> lock(sourceLock);
        if (chosenSource.valid()) return chosenSource;
    }
    return snapshotLiveAudio(12.0);
}

void NoirnuitReverseAudioProcessor::setCapturedSource(noirnuit::StereoClip clip)
{
    std::lock_guard<std::mutex> lock(sourceLock);
    chosenSource = std::move(clip);
}

void NoirnuitReverseAudioProcessor::useLiveInput()
{
    std::lock_guard<std::mutex> lock(sourceLock);
    chosenSource = {};
}

bool NoirnuitReverseAudioProcessor::hasSource() const
{
    std::lock_guard<std::mutex> lock(sourceLock);
    return chosenSource.valid();
}

bool NoirnuitReverseAudioProcessor::importWav(const juce::File& file, juce::String& message)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (reader == nullptr) { message = "Format audio non reconnu"; return false; }
    if (reader->sampleRate < 8000.0 || reader->sampleRate > 384000.0)
    { message = "Frequence non prise en charge"; return false; }
    // Restrict to last 15 sec; avoids uncontrolled memory use for long songs.
    const int64_t maxOriginal = static_cast<int64_t>(15.0 * reader->sampleRate);
    const int n = static_cast<int>(std::min(reader->lengthInSamples, maxOriginal));
    if (n <= 0) { message = "Fichier audio vide"; return false; }
    juce::AudioBuffer<float> audio(2, n);
    if (!reader->read(&audio, 0, n, reader->lengthInSamples - n, true, true))
    { message = "Lecture audio impossible"; return false; }
    if (reader->numChannels == 1) audio.copyFrom(1, 0, audio, 0, 0, n);
    noirnuit::StereoClip clip;
    clip.sampleRate = currentSampleRate.load();
    if (clip.sampleRate <= 0) clip.sampleRate = 44100;
    // Linear sample-rate conversion for imported files, to match the DAW preview and export.
    const auto targetN = static_cast<std::size_t>(std::ceil(n * (double)clip.sampleRate / reader->sampleRate));
    clip.left.resize(targetN);
    clip.right.resize(targetN);
    for (std::size_t i = 0; i < targetN; ++i)
    {
        const double pos = std::min(static_cast<double>(n - 1), i * reader->sampleRate / clip.sampleRate);
        const int i0 = static_cast<int>(pos);
        const int i1 = std::min(i0 + 1, n - 1);
        const float a = static_cast<float>(pos - i0);
        clip.left[i] = juce::jmap(a, audio.getSample(0, i0), audio.getSample(0, i1));
        clip.right[i] = juce::jmap(a, audio.getSample(1, i0), audio.getSample(1, i1));
    }
    setCapturedSource(std::move(clip));
    message = "Fichier charge : " + file.getFileName();
    return true;
}

noirnuit::Settings NoirnuitReverseAudioProcessor::currentSettings() const
{
    noirnuit::Settings s;
    const int idx = static_cast<int>(parameters.getRawParameterValue("bars")->load());
    const int length = (idx == 0 ? 1 : idx == 1 ? 2 : idx == 2 ? 4 : 8);
    s.bpm = hostBpm.load();
    s.durationSeconds = std::min(45.0, length * 4.0 * 60.0 / std::max(30.0, s.bpm));
    s.decay = parameters.getRawParameterValue("decay")->load();
    s.tone = parameters.getRawParameterValue("tone")->load();
    s.width = parameters.getRawParameterValue("width")->load();
    s.sibilance = parameters.getRawParameterValue("sibilance")->load();
    s.vocalMode = parameters.getRawParameterValue("vocal")->load() > 0.5f;
    s.phone = parameters.getRawParameterValue("phone")->load() > 0.5f;
    s.chorus = parameters.getRawParameterValue("chorus")->load() > 0.5f;
    s.drive = parameters.getRawParameterValue("drive")->load() > 0.5f;
    s.tremolo = parameters.getRawParameterValue("tremolo")->load() > 0.5f;
    return s;
}

void NoirnuitReverseAudioProcessor::storeRendered(noirnuit::StereoClip clip)
{
    auto result = std::make_shared<juce::AudioBuffer<float>>(2, static_cast<int>(clip.length()));
    result->copyFrom(0, 0, clip.left.data(), static_cast<int>(clip.length()));
    result->copyFrom(1, 0, clip.right.data(), static_cast<int>(clip.length()));
    stopPreview();
    std::shared_ptr<const juce::AudioBuffer<float>> immutable = result;
    std::atomic_store_explicit(&renderedAudio, std::move(immutable), std::memory_order_release);
}

std::shared_ptr<const juce::AudioBuffer<float>> NoirnuitReverseAudioProcessor::getRendered() const
{
    return std::atomic_load_explicit(&renderedAudio, std::memory_order_acquire);
}

bool NoirnuitReverseAudioProcessor::writeRenderedWav(const juce::File& file) const
{
    auto result = getRendered();
    if (!result) return false;
    auto stream = file.createOutputStream();
    if (!stream || stream->failedToOpen()) return false;
    juce::WavAudioFormat wav;
    auto* rawWriter = wav.createWriterFor(stream.get(), static_cast<double>(currentSampleRate.load()),
                                           2, 24, {}, 0);
    if (!rawWriter) return false;
    stream.release(); // AudioFormatWriter now owns the FileOutputStream.
    std::unique_ptr<juce::AudioFormatWriter> writer(rawWriter);
    return writer->writeFromAudioSampleBuffer(*result, 0, result->getNumSamples());
}

void NoirnuitReverseAudioProcessor::startPreview()
{
    if (!getRendered()) return;
    previewPosition.store(0);
    previewActive.store(true);
}
void NoirnuitReverseAudioProcessor::stopPreview() { previewActive.store(false); }

void NoirnuitReverseAudioProcessor::getStateInformation(juce::MemoryBlock& b)
{
    auto tree = parameters.copyState();
    if (auto xml = tree.createXml()) copyXmlToBinary(*xml, b);
}
void NoirnuitReverseAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        const auto tree = juce::ValueTree::fromXml(*xml);
        if (tree.isValid()) parameters.replaceState(tree);
    }
}

juce::AudioProcessorEditor* NoirnuitReverseAudioProcessor::createEditor()
{
    return new NoirnuitReverseAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoirnuitReverseAudioProcessor();
}
