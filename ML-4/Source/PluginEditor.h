#pragma once
#include "PluginProcessor.h"

namespace ml4ui
{
namespace col
{
    const juce::Colour woodMid  { 0xff7a4424 }, woodHi { 0xff9a5a31 }, woodLo { 0xff4a2512 };
    const juce::Colour hdr      { 0xff7c1f24 }, hdrHi  { 0xff952a30 };
    const juce::Colour red      { 0xff8a2429 }, redLo  { 0xff5e161a }, redDeep { 0xff7e1f24 }, redLine { 0xffa6393f };
    const juce::Colour charcoal { 0xff2c2b29 }, charLo { 0xff1f1e1c }, charLine { 0xff403e3b }, charBot { 0xff272624 };
    const juce::Colour cream    { 0xffe6dcc6 }, creamDim { 0xffb7ac94 }, key { 0xffefe7d5 };
    const juce::Colour ink      { 0xff1b1815 };
    const juce::Colour disp     { 0xff262c31 }, dispHi { 0xff34404a };
    const juce::Colour amber    { 0xfff0a23b };
}

juce::Font font (float h, bool bold = false);

class LnF : public juce::LookAndFeel_V4
{
public:
    LnF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
};

class Content;

class Knob : public juce::Component
{
public:
    Knob (ML4Processor&, const ml4::ParamDef&, Content&, bool horizontal = false);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::Slider slider;
private:
    const ml4::ParamDef& def;
    bool horizontal;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class Seg : public juce::Component
{
public:
    Seg (ML4Processor&, const ml4::ParamDef&, Content&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::Rectangle<float> boxArea() const;
    const ml4::ParamDef& def;
    Content& content;
    int index = 0;
    juce::ParameterAttachment att;
};

class Display : public juce::Component
{
public:
    Display (ML4Processor&, Content&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    void stepProgram (int d);
    ML4Processor& proc;
    Content& content;
    std::vector<float> snap;
};

struct ModuleDef { const char* title; bool dark; std::vector<const char*> ids; };

class Content : public juce::Component
{
public:
    static constexpr int W = 1100, H = 740, wood = 22;
    explicit Content (ML4Processor&);
    void paint (juce::Graphics&) override;
    void resized() override;

    void setStatus (const juce::String& s) { status = s; statusTime = juce::Time::getMillisecondCounter(); }
    juce::String currentStatus() const;
    void tick() { display.repaint(); }

private:
    std::vector<ModuleDef> modules;
    std::vector<juce::Rectangle<int>> moduleRects;
    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<Seg> segs;
    std::map<std::string, juce::Component*> byId;
    Display display;
    std::unique_ptr<Knob> output;
    juce::MidiKeyboardComponent keyboard;
    juce::String status;
    juce::uint32 statusTime = 0;
};
} // namespace ml4ui

class ML4Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ML4Editor (ML4Processor&);
    ~ML4Editor() override;
    void paint (juce::Graphics& g) override { g.fillAll (ml4ui::col::charLo); }
    void resized() override;
private:
    void timerCallback() override { content.tick(); }
    ml4ui::LnF lnf;
    ml4ui::Content content;
};
