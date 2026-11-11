#include "PluginEditor.h"

using namespace ml4;
namespace ml4ui
{
juce::Font font (float h, bool bold)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

// ======================= LookAndFeel =======================
LnF::LnF()
{
    setColour (juce::PopupMenu::backgroundColourId, col::disp);
    setColour (juce::PopupMenu::textColourId, col::cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::cream);
    setColour (juce::PopupMenu::highlightedTextColourId, col::ink);
}

void LnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                            float start, float end, juce::Slider& s)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (1.0f);
    const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    auto pt = [&] (float rad, float a) { return juce::Point<float> (c.x + rad * std::sin (a), c.y - rad * std::cos (a)); };

    // scale ticks
    g.setColour (col::creamDim);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * (float) i / 10.0f;
        g.drawLine (juce::Line<float> (pt (i % 5 == 0 ? r * 0.80f : r * 0.85f, a), pt (r * 0.95f, a)), 1.3f);
    }

    // value arc
    const bool bip = (bool) s.getProperties().getWithDefault ("bipolar", false);
    const float from = bip ? 0.5f : 0.0f;
    const float lo = std::min (from, pos), hi = std::max (from, pos);
    if (hi - lo > 0.003f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, r * 0.72f, r * 0.72f, 0.0f, start + (end - start) * lo, start + (end - start) * hi, true);
        g.setColour (col::amber.withAlpha (0.85f));
        g.strokePath (arc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // shadow, metal ring, cap
    const float rr = r * 0.63f, rc = r * 0.51f;
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (c.x - rr, c.y - rr + 1.6f, rr * 2, rr * 2);
    juce::ColourGradient ring (juce::Colour (0xff9a958d), c.x, c.y - rr, juce::Colour (0xff121110), c.x, c.y + rr, false);
    ring.addColour (0.45, juce::Colour (0xff4a4743));
    g.setGradientFill (ring);
    g.fillEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2);
    juce::ColourGradient cap (juce::Colour (0xff55514c), c.x - rc * 0.25f, c.y - rc * 0.35f, juce::Colour (0xff0f0e0d), c.x + rc, c.y + rc, true);
    cap.addColour (0.55, juce::Colour (0xff22201e));
    g.setGradientFill (cap);
    g.fillEllipse (c.x - rc, c.y - rc, rc * 2, rc * 2);

    const float a = start + (end - start) * pos;
    g.setColour (col::cream);
    g.drawLine (juce::Line<float> (pt (rc * 0.35f, a), pt (rr * 0.92f, a)), 2.4f);
}

// ======================= Knob =======================
Knob::Knob (ML4Processor& p, const ParamDef& d, Content& content, bool horiz) : def (d), horizontal (horiz)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (170);
    slider.getProperties().set ("bipolar", d.bipolar);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, d.id, slider);
    slider.setDoubleClickReturnValue (true, d.def);
    auto* proc = &p;
    slider.onValueChange = [this, proc, &content]
    {
        if (slider.isMouseButtonDown() || slider.hasKeyboardFocus (false))
            content.setStatus (UA (def.ua) + "  " + formatValue (def.fmt, (float) slider.getValue(), true, proc->modeForText.load()));
    };
}

void Knob::resized()
{
    auto b = getLocalBounds();
    if (horizontal) slider.setBounds (b.removeFromRight (b.getHeight()));
    else slider.setBounds (b.removeFromTop (b.getWidth() - 6).withSizeKeepingCentre (b.getWidth() - 6, b.getWidth() - 6));
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (col::cream);
    g.setFont (font (11.5f));
    const auto text = UA (def.ua).toUpperCase();
    if (horizontal) g.drawText (text, getLocalBounds().withTrimmedRight (getHeight() + 4), juce::Justification::centredRight);
    else g.drawFittedText (text, getLocalBounds().removeFromBottom (16), juce::Justification::centred, 1, 0.8f);
}

// ======================= Seg (segmented switch) =======================
Seg::Seg (ML4Processor& p, const ParamDef& d, Content& c)
    : def (d), content (c),
      att (*p.apvts.getParameter (d.id), [this] (float v) { index = juce::roundToInt (v); repaint(); })
{
    att.sendInitialUpdate();
}

juce::Rectangle<float> Seg::boxArea() const { return getLocalBounds().toFloat().withTrimmedTop (15.0f); }

void Seg::paint (juce::Graphics& g)
{
    g.setColour (col::creamDim);
    g.setFont (font (11.0f));
    g.drawText (UA (def.ua).toUpperCase(), getLocalBounds().removeFromTop (14), juce::Justification::centredLeft);

    auto box = boxArea();
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (box, 3.0f);
    box.reduce (2.0f, 2.0f);
    const int n = (int) def.chUa.size();
    const float w = (box.getWidth() - 2.0f * (float) (n - 1)) / (float) n;
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (box.getX() + (float) i * (w + 2.0f), box.getY(), w, box.getHeight());
        const bool on = i == index;
        if (on) { g.setColour (col::cream); g.fillRoundedRectangle (r, 2.0f); }
        g.setColour (on ? col::ink : col::creamDim);
        g.setFont (font (12.0f, on));
        g.drawFittedText (UA (def.chUa[(size_t) i]), r.toNearestInt(), juce::Justification::centred, 1, 0.7f);
    }
}

void Seg::mouseDown (const juce::MouseEvent& e)
{
    const auto box = boxArea();
    if (! box.contains (e.position)) return;
    const int n = (int) def.chUa.size();
    const int i = juce::jlimit (0, n - 1, (int) ((e.position.x - box.getX()) / box.getWidth() * (float) n));
    att.setValueAsCompleteGesture ((float) i);
    content.setStatus (UA (def.ua) + "  " + UA (def.chUa[(size_t) i]));
}

// ======================= Display =======================
Display::Display (ML4Processor& p, Content& c) : proc (p), content (c) { snap.resize (1024); }

void Display::stepProgram (int d)
{
    const int n = proc.getNumPrograms();
    proc.setCurrentProgram ((proc.getCurrentProgram() + d + n) % n);
    content.setStatus (UA ("Патч"));
}

void Display::mouseDown (const juce::MouseEvent& e)
{
    const float x = e.position.x;
    if (x < 38.0f) { stepProgram (-1); return; }
    const float scopeX = (float) getWidth() - 210.0f;
    if (x >= scopeX - 38.0f && x < scopeX) { stepProgram (1); return; }
    if (x >= scopeX) return;

    juce::PopupMenu m;
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        m.addItem (i + 1, proc.programNameUa (i), true, i == proc.getCurrentProgram() && ! proc.isPatchEdited());
    juce::Component::SafePointer<Display> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                     [safe] (int r) { if (safe != nullptr && r > 0) { safe->proc.setCurrentProgram (r - 1); safe->repaint(); } });
}

void Display::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float rad = b.getHeight() * 0.5f;
    juce::ColourGradient bg (col::dispHi, 0, 0, col::disp, 0, b.getHeight(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (b, rad);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (b.reduced (0.5f), rad, 1.0f);

    // arrows
    const float scopeX = b.getWidth() - 210.0f;
    auto tri = [&g] (float cx, float cy, bool left)
    {
        juce::Path p;
        if (left) p.addTriangle (cx + 4, cy - 6, cx + 4, cy + 6, cx - 5, cy);
        else      p.addTriangle (cx - 4, cy - 6, cx - 4, cy + 6, cx + 5, cy);
        g.setColour (col::cream);
        g.fillPath (p);
    };
    tri (20.0f, b.getCentreY(), true);
    tri (scopeX - 19.0f, b.getCentreY(), false);

    // text
    auto t = juce::Rectangle<float> (38.0f, 3.0f, scopeX - 76.0f, b.getHeight() - 6.0f);
    static const char* modes[] = { "Полі", "Унісон", "Акорд", "Арп" };
    g.setFont (font (11.0f));
    g.setColour (col::creamDim);
    g.drawText (content.currentStatus() + UA ("   ·   режим: ") + UA (modes[juce::jlimit (0, 3, proc.modeForText.load())]),
                t.removeFromTop (14.0f), juce::Justification::centred);
    g.setColour (col::cream);
    g.setFont (font (21.0f));
    auto name = proc.programNameUa (proc.getCurrentProgram());
    if (proc.isPatchEdited()) name += " *";
    g.drawFittedText (name, t.toNearestInt(), juce::Justification::centred, 1, 0.8f);

    // scope
    auto sc = juce::Rectangle<float> (scopeX, 5.0f, 200.0f, b.getHeight() - 10.0f);
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillRoundedRectangle (sc, sc.getHeight() * 0.5f);
    const int w = proc.scopeWrite.load();
    const int N = ML4Processor::scopeSize;
    int start = (w - 2048 + N) % N;
    for (int i = 1; i < 1024; ++i)
    {
        const float a = proc.scope[(size_t) ((start + i - 1) % N)], bb = proc.scope[(size_t) ((start + i) % N)];
        if (a < 0.0f && bb >= 0.0f) { start = (start + i) % N; break; }
    }
    for (int i = 0; i < 1024; ++i) snap[(size_t) i] = proc.scope[(size_t) ((start + i) % N)];
    juce::Path p;
    const auto in = sc.reduced (12.0f, 4.0f);
    for (int i = 0; i < 1024; ++i)
    {
        const float x = in.getX() + in.getWidth() * (float) i / 1023.0f;
        const float y = in.getCentreY() - juce::jlimit (-1.0f, 1.0f, snap[(size_t) i]) * in.getHeight() * 0.5f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (col::amber);
    g.strokePath (p, juce::PathStrokeType (1.4f));
}

// ======================= Content =======================
Content::Content (ML4Processor& p)
    : display (p, *this),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    modules = {
        { "VCO 1",               true,  { "w1", "o1", "sh1" } },
        { "VCO 2",               true,  { "w2", "o2", "ring", "sh2", "p2", "fn2", "xm" } },
        { "Мікшер",              true,  { "l1", "l2", "ln", "nc" } },
        { "Фільтр",              true,  { "kt", "poles", "cut", "res", "drv" } },
        { "Огинаюча гучності",   true,  { "aA", "aD", "aS", "aR" } },
        { "Огинаюча модуляції",  true,  { "eT", "eA", "eD", "eI" } },
        { "LFO",                 false, { "lw", "lt", "lr", "li" } },
        { "Голоси",              false, { "vm", "latch", "arate", "vd", "port" } },
        { "Модуляція",           false, { "mt", "md", "mr", "mm" } },
        { "Затримка",            false, { "dt", "df", "dm" } },
        { "Реверберація",        false, { "rt", "rm" } },
    };
    for (auto& m : modules)
        for (auto* id : m.ids)
        {
            const auto* d = findParam (id);
            jassert (d != nullptr);
            juce::Component* c = nullptr;
            if (d->choice) c = segs.add (new Seg (p, *d, *this));
            else           c = knobs.add (new Knob (p, *d, *this));
            addAndMakeVisible (c);
            byId[id] = c;
        }
    output = std::make_unique<Knob> (p, *findParam ("vol"), *this, true);
    addAndMakeVisible (*output);
    addAndMakeVisible (display);

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, col::key);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff1e1c1a));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, col::creamDim);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, col::amber);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, col::amber.withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, juce::Colour (0xff6a5f4c));
    keyboard.setAvailableRange (36, 84);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    setSize (W, H);
}

juce::String Content::currentStatus() const
{
    if (status.isNotEmpty() && juce::Time::getMillisecondCounter() - statusTime < 1800) return status;
    return UA ("Патч");
}

void Content::resized()
{
    const int x0 = wood, iw = W - 2 * wood;
    display.setBounds (x0 + 178, 10, iw - 178 - 128, 44);
    output->setBounds (x0 + iw - 118, 6, 108, 52);

    moduleRects.clear();
    const int darkN = 6, redN = 5;
    for (int i = 0; i < darkN; ++i)
        moduleRects.push_back ({ x0 + i * iw / darkN, 64, (i + 1) * iw / darkN - i * iw / darkN, 300 });
    for (int i = 0; i < redN; ++i)
        moduleRects.push_back ({ x0 + i * iw / redN, 364, (i + 1) * iw / redN - i * iw / redN, 240 });

    for (size_t m = 0; m < modules.size(); ++m)
    {
        auto r = moduleRects[m].reduced (10, 0);
        int y = r.getY() + (modules[m].dark ? 12 : 30);
        std::vector<juce::Component*> ks;
        for (auto* id : modules[m].ids)
        {
            auto* c = byId[id];
            if (dynamic_cast<Seg*> (c) != nullptr) { c->setBounds (r.getX(), y, r.getWidth(), 36); y += 42; }
            else ks.push_back (c);
        }
        y += 4;
        const int cw = 56, ch = 68;
        const int cols = juce::jmax (1, juce::jmin ((int) ks.size(), r.getWidth() / cw));
        const int gx = r.getX() + (r.getWidth() - cols * cw) / 2;
        for (size_t i = 0; i < ks.size(); ++i)
            ks[i]->setBounds (gx + (int) (i % (size_t) cols) * cw, y + (int) (i / (size_t) cols) * ch, cw, ch);
    }

    keyboard.setBounds (x0 + 10, 614, iw - 20, 116);
    keyboard.setKeyWidth ((float) (iw - 20) / 29.0f);
    keyboard.setLowestVisibleKey (36);
}

void Content::paint (juce::Graphics& g)
{
    const int x0 = wood, iw = W - 2 * wood;
    g.fillAll (col::charLo);

    // wood cheeks
    juce::Random rng (4);
    for (int side = 0; side < 2; ++side)
    {
        const auto r = juce::Rectangle<float> (side == 0 ? 0.0f : (float) (W - wood), 0.0f, (float) wood, (float) H);
        juce::ColourGradient wg (col::woodLo, r.getX(), 0, col::woodLo, r.getRight(), 0, false);
        wg.addColour (0.4, col::woodHi);
        g.setGradientFill (wg);
        g.fillRect (r);
        for (int i = 0; i < 70; ++i)
        {
            const float yy = rng.nextFloat() * (float) H;
            g.setColour ((rng.nextBool() ? col::woodLo : col::woodHi).withAlpha (0.25f + rng.nextFloat() * 0.3f));
            g.drawLine (r.getX(), yy, r.getRight(), yy + rng.nextFloat() * 8.0f - 4.0f, 1.0f + rng.nextFloat());
        }
        juce::ColourGradient sh (juce::Colours::black.withAlpha (0.45f), side == 0 ? r.getRight() : r.getX(), 0,
                                 juce::Colours::transparentBlack, side == 0 ? r.getRight() - 6 : r.getX() + 6, 0, false);
        g.setGradientFill (sh);
        g.fillRect (r);
    }

    // header
    juce::ColourGradient hg (col::hdrHi, 0, 0, col::hdr, 0, 64, false);
    g.setGradientFill (hg);
    g.fillRect (x0, 0, iw, 64);
    g.setColour (col::redLo);
    g.fillRect (x0, 62, iw, 2);
    auto logo = juce::Rectangle<float> ((float) x0 + 12.0f, 12.0f, 152.0f, 40.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (logo.translated (0, 2), 4.0f);
    g.setColour (col::cream);
    g.fillRoundedRectangle (logo, 4.0f);
    g.setColour (col::creamDim);
    g.fillRect (logo.removeFromBottom (2.0f).reduced (3.0f, 0));
    g.setColour (col::ink);
    g.setFont (font (28.0f, true));
    g.drawText ("ML-4", juce::Rectangle<float> ((float) x0 + 22.0f, 12.0f, 80.0f, 38.0f), juce::Justification::centredLeft);
    g.setFont (font (10.5f));
    g.setColour (juce::Colour (0xff6a5f4c));
    g.drawText (UA ("ПОЛІСИНТ"), juce::Rectangle<float> ((float) x0 + 96.0f, 12.0f, 64.0f, 38.0f), juce::Justification::centredLeft);

    // modules
    for (size_t m = 0; m < modules.size(); ++m)
    {
        const auto r = moduleRects[m].toFloat();
        const bool dark = modules[m].dark;
        juce::ColourGradient mg (dark ? col::charcoal : col::red, 0, r.getY(), dark ? col::charBot : col::redDeep, 0, r.getBottom(), false);
        g.setGradientFill (mg);
        g.fillRect (r);
        g.setColour (dark ? col::charLine : col::redLo);
        g.fillRect (r.getRight() - 1.0f, r.getY(), 1.0f, r.getHeight());
        if (! dark) { g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRect (r.getX(), r.getY(), r.getWidth(), 1.0f); }

        const auto title = UA (modules[m].title).toUpperCase();
        if (dark)
        {
            g.setColour (col::charLine);
            g.fillRect (r.getX() + 10.0f, r.getBottom() - 26.0f, r.getWidth() - 20.0f, 1.0f);
            juce::Path t;
            const float ty = r.getBottom() - 13.0f;
            t.addTriangle (r.getX() + 12.0f, ty - 3.0f, r.getX() + 20.0f, ty - 3.0f, r.getX() + 16.0f, ty + 3.0f);
            g.setColour (col::creamDim);
            g.fillPath (t);
            g.setFont (font (11.5f, true));
            g.drawText (title, juce::Rectangle<float> (r.getX() + 25.0f, r.getBottom() - 24.0f, r.getWidth() - 30.0f, 22.0f), juce::Justification::centredLeft);
        }
        else
        {
            g.setColour (col::cream);
            g.setFont (font (12.5f, true));
            g.drawText (title, juce::Rectangle<float> (r.getX(), r.getY() + 6.0f, r.getWidth(), 18.0f), juce::Justification::centred);
        }
    }
    g.setColour (col::charLo);
    g.fillRect (x0, 361, iw, 3);

    // keybed
    juce::ColourGradient kg (col::charLo, 0, 604, juce::Colour (0xff151413), 0, (float) H, false);
    g.setGradientFill (kg);
    g.fillRect (x0, 604, iw, H - 604);
    g.setColour (juce::Colours::black);
    g.fillRect (x0, 604, iw, 2);
}
} // namespace ml4ui

// ======================= Editor =======================
ML4Editor::ML4Editor (ML4Processor& p) : AudioProcessorEditor (p), content (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    setResizable (true, true);
    setResizeLimits (770, 518, 1650, 1110);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ml4ui::Content::W / ml4ui::Content::H);
    setSize (ml4ui::Content::W, ml4ui::Content::H);
    startTimerHz (30);
}

ML4Editor::~ML4Editor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ML4Editor::resized()
{
    const float s = (float) getWidth() / (float) ml4ui::Content::W;
    content.setTransform (juce::AffineTransform::scale (s));
}
