#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace ml4;

ML4Processor::ML4Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ML4", createLayout())
{
    for (auto& d : params())
    {
        raw.push_back (apvts.getRawParameterValue (d.id));
        apvts.addParameterListener (d.id, this);
    }
    setCurrentProgram (0);
    edited = false;
}

ML4Processor::~ML4Processor()
{
    for (auto& d : params()) apvts.removeParameterListener (d.id, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout ML4Processor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& d : params())
    {
        if (d.choice)
        {
            juce::StringArray names;
            for (auto* c : d.chEn) names.add (c);
            layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { d.id, 1 }, d.en, names, (int) d.def));
        }
        else
        {
            const Fmt f = d.fmt;
            auto* mode = &modeForText;
            auto attrs = juce::AudioParameterFloatAttributes()
                             .withStringFromValueFunction ([f, mode] (float v, int) { return formatValue (f, v, false, mode->load()); });
            layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { d.id, 1 }, d.en,
                                                                      juce::NormalisableRange<float> (0.0f, 1.0f), d.def, attrs));
        }
    }
    return layout;
}

void ML4Processor::parameterChanged (const juce::String& id, float value)
{
    if (id == "vm") modeForText = (int) value;
    if (! loading) edited = true;
}

void ML4Processor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms()) return;
    currentProgram = index;
    loading = true;
    const auto& pr = presets()[(size_t) index];
    for (auto& d : params())
    {
        if (std::string (d.id) == "vol") continue;            // keep the user's output level
        float v = d.def;
        for (auto& kv : pr.values) if (std::string (kv.first) == d.id) v = kv.second;
        if (auto* prm = apvts.getParameter (d.id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    }
    loading = false;
    edited = false;
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

const juce::String ML4Processor::getProgramName (int index)
{
    if (index < 0 || index >= getNumPrograms()) return {};
    return presets()[(size_t) index].en;
}

juce::String ML4Processor::programNameUa (int index) const
{
    if (index < 0 || index >= (int) presets().size()) return {};
    return UA (presets()[(size_t) index].ua);
}

bool ML4Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void ML4Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    monoScratch.assign ((size_t) juce::jmax (512, samplesPerBlock), 0.0f);
    keyboardState.reset();
}

P ML4Processor::readParams() const
{
    P p;
    size_t i = 0;
    auto f = [&] { return raw[i++]->load(); };
    auto c = [&] { return (int) std::lround (raw[i++]->load()); };
    // must follow params() order exactly
    p.w1 = c(); p.o1 = c(); p.sh1 = f(); p.w2 = c(); p.o2 = c(); p.sh2 = f(); p.p2 = f(); p.fn2 = f();
    p.xm = f(); p.ring = c(); p.l1 = f(); p.l2 = f(); p.ln = f(); p.nc = f(); p.drv = f(); p.port = f();
    p.cut = f(); p.res = f(); p.kt = c(); p.poles = c(); p.eA = f(); p.eD = f(); p.eI = f(); p.eT = c();
    p.aA = f(); p.aD = f(); p.aS = f(); p.aR = f(); p.lw = c(); p.lr = f(); p.li = f(); p.lt = c();
    p.vm = c(); p.vd = f(); p.latch = c(); p.arate = c(); p.mt = c(); p.md = f(); p.mr = f(); p.mm = f();
    p.dt = f(); p.df = f(); p.dm = f(); p.rt = f(); p.rm = f(); p.vol = f();
    jassert (i == raw.size());
    return p;
}

void ML4Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    engine.setParams (readParams());

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;

    buffer.clear();
    const bool stereo = buffer.getNumChannels() > 1;
    float* L = buffer.getWritePointer (0);
    if (! stereo && (int) monoScratch.size() < n) monoScratch.resize ((size_t) n);
    float* R = stereo ? buffer.getWritePointer (1) : monoScratch.data();

    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, n, meta.samplePosition);
        if (at > pos) { engine.render (L + pos, R + pos, at - pos, bpm); pos = at; }
        const auto m = meta.getMessage();
        if (m.isNoteOn()) engine.midiNoteOn (m.getNoteNumber());
        else if (m.isNoteOff()) engine.midiNoteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allOff();
    }
    if (pos < n) engine.render (L + pos, R + pos, n - pos, bpm);

    if (! stereo)
        for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + R[i]);

    int w = scopeWrite.load();
    for (int i = 0; i < n; ++i) { scope[(size_t) w] = L[i]; w = (w + 1) % scopeSize; }
    scopeWrite.store (w);
    modeForText = engine.voiceMode();
}

void ML4Processor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("edited", edited.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void ML4Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            loading = true;
            apvts.replaceState (tree);
            loading = false;
            currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
            edited = (bool) tree.getProperty ("edited", false);
        }
}

juce::AudioProcessorEditor* ML4Processor::createEditor() { return new ML4Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ML4Processor(); }
