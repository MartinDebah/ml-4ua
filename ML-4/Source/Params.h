#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <vector>

// UI strings are Ukrainian (UTF-8). Everything the host sees — parameter
// names, choice names, value text, program names — is plain ASCII English,
// so Maschine's hardware screens (no Cyrillic) display it correctly.
#define UA(s) juce::String::fromUTF8(s)

namespace ml4
{
// ---- value mappings (identical to the web prototype) ----
namespace map
{
    inline float cutHz (float v)   { return 20.0f * std::exp2 (v * 10.0f); }
    inline float noiseHz (float v) { return 200.0f * std::exp2 (v * 7.0f); }
    inline float ampA (float v)    { return 0.001f + v * v * 4.0f; }
    inline float ampD (float v)    { return 0.005f + v * v * 4.0f; }
    inline float ampR (float v)    { return 0.005f + v * v * 6.0f; }
    inline float egA (float v)     { return 0.001f + v * v * 3.0f; }
    inline float egD (float v)     { return 0.005f + v * v * 4.0f; }
    inline float lfoHz (float v)   { return 0.05f * std::exp2 (v * 9.0f); }
    inline float modHz (float v)   { return 0.05f * std::exp2 (v * 7.0f); }
    inline float glide (float v)   { return v * v * 1.5f; }
    inline float dlySec (float v)  { return 0.03f * std::exp2 (v * 5.1f); }
    inline float revSec (float v)  { return 0.5f + v * 4.5f; }
    inline int   semis (float v)   { return (int) std::lround ((v - 0.5f) * 48.0f); }
    inline float cents (float v)   { return (v - 0.5f) * 100.0f; }
    inline int   idx8 (float v)    { return juce::jlimit (0, 7, (int) std::floor (v * 8.0f)); }
}

enum class Fmt { Pct, PctOff, CutHz, NoiseHz, Semis, Cents, BiPct, AmpA, AmpD, AmpR, EgA, EgD, LfoHz, ModHz, Glide, DlySec, RevSec, VoiceDepth };

struct ParamDef
{
    const char* id;
    const char* en;      // host / Maschine name (ASCII)
    const char* ua;      // GUI label (UTF-8)
    bool choice;
    float def;           // knob default (0..1) or choice index
    Fmt fmt;
    bool bipolar;
    std::vector<const char*> chEn, chUa;
};

// Order = Maschine pages of 8 parameters.
inline const std::vector<ParamDef>& params()
{
    static const std::vector<ParamDef> p = {
        // Page 1 — Oscillators
        { "w1",   "VCO1 Wave",    "Хвиля",    true,  0,    Fmt::Pct, false, { "Saw", "Tri", "Square" }, { "Пилка", "Трикут.", "Квадрат" } },
        { "o1",   "VCO1 Octave",  "Октава",   true,  1,    Fmt::Pct, false, { "16'", "8'", "4'", "2'" }, { "16′", "8′", "4′", "2′" } },
        { "sh1",  "VCO1 Shape",   "Форма",    false, 0.2f, Fmt::Pct, false, {}, {} },
        { "w2",   "VCO2 Wave",    "Хвиля",    true,  0,    Fmt::Pct, false, { "Saw", "Tri", "Square" }, { "Пилка", "Трикут.", "Квадрат" } },
        { "o2",   "VCO2 Octave",  "Октава",   true,  1,    Fmt::Pct, false, { "16'", "8'", "4'", "2'" }, { "16′", "8′", "4′", "2′" } },
        { "sh2",  "VCO2 Shape",   "Форма",    false, 0.2f, Fmt::Pct, false, {}, {} },
        { "p2",   "VCO2 Pitch",   "Висота",   false, 0.5f, Fmt::Semis, true, {}, {} },
        { "fn2",  "VCO2 Fine",    "Точно",    false, 0.56f, Fmt::Cents, true, {}, {} },
        // Page 2 — Osc mod, mixer
        { "xm",   "Cross Mod",    "Крос-мод", false, 0,    Fmt::Pct, false, {}, {} },
        { "ring", "Ring Mod",     "Режим",    true,  0,    Fmt::Pct, false, { "Off", "Ring" }, { "Звичайний", "Кільце" } },
        { "l1",   "VCO1 Level",   "VCO 1",    false, 0.8f, Fmt::Pct, false, {}, {} },
        { "l2",   "VCO2 Level",   "VCO 2",    false, 0.6f, Fmt::Pct, false, {}, {} },
        { "ln",   "Noise Level",  "Шум",      false, 0,    Fmt::Pct, false, {}, {} },
        { "nc",   "Noise Color",  "Колір",    false, 0.6f, Fmt::NoiseHz, false, {}, {} },
        { "drv",  "Drive",        "Драйв",    false, 0.1f, Fmt::Pct, false, {}, {} },
        { "port", "Glide",        "Портаменто", false, 0,  Fmt::Glide, false, {}, {} },
        // Page 3 — Filter + mod EG
        { "cut",  "Cutoff",       "Зріз",     false, 0.55f, Fmt::CutHz, false, {}, {} },
        { "res",  "Resonance",    "Резонанс", false, 0.2f, Fmt::Pct, false, {}, {} },
        { "kt",   "Key Track",    "Слідкування", true, 1,  Fmt::Pct, false, { "0", "50%", "100%" }, { "0", "50%", "100%" } },
        { "poles","Filter Slope", "Крутизна", true,  1,    Fmt::Pct, false, { "12 dB", "24 dB" }, { "2 полюси", "4 полюси" } },
        { "eA",   "Mod EG Attack","Атака",    false, 0,    Fmt::EgA, false, {}, {} },
        { "eD",   "Mod EG Decay", "Спад",     false, 0.35f, Fmt::EgD, false, {}, {} },
        { "eI",   "Mod EG Amount","Глибина",  false, 0.62f, Fmt::BiPct, true, {}, {} },
        { "eT",   "Mod EG Target","Ціль",     true,  0,    Fmt::Pct, false, { "Cutoff", "Pitch", "Pitch 2" }, { "Зріз", "Висота", "Висота 2" } },
        // Page 4 — Amp EG + LFO
        { "aA",   "Amp Attack",   "Атака",    false, 0.05f, Fmt::AmpA, false, {}, {} },
        { "aD",   "Amp Decay",    "Спад",     false, 0.4f, Fmt::AmpD, false, {}, {} },
        { "aS",   "Amp Sustain",  "Рівень",   false, 0.7f, Fmt::Pct, false, {}, {} },
        { "aR",   "Amp Release",  "Згасання", false, 0.35f, Fmt::AmpR, false, {}, {} },
        { "lw",   "LFO Wave",     "Хвиля",    true,  0,    Fmt::Pct, false, { "Tri", "Saw", "Square" }, { "Трикут.", "Пилка", "Квадрат" } },
        { "lr",   "LFO Rate",     "Швидкість",false, 0.35f, Fmt::LfoHz, false, {}, {} },
        { "li",   "LFO Amount",   "Глибина",  false, 0.12f, Fmt::Pct, false, {}, {} },
        { "lt",   "LFO Target",   "Ціль",     true,  0,    Fmt::Pct, false, { "Cutoff", "Pitch", "Volume" }, { "Зріз", "Висота", "Гучність" } },
        // Page 5 — Voices + mod FX
        { "vm",   "Voice Mode",   "Режим",    true,  0,    Fmt::Pct, false, { "Poly", "Unison", "Chord", "Arp" }, { "Полі", "Унісон", "Акорд", "Арп" } },
        { "vd",   "Voice Depth",  "Глибина",  false, 0.3f, Fmt::VoiceDepth, false, {}, {} },
        { "latch","Arp Latch",    "Утримання",true,  0,    Fmt::Pct, false, { "Off", "On" }, { "Вимк", "Утрим." } },
        { "arate","Arp Rate",     "Арп-крок", true,  1,    Fmt::Pct, false, { "1/8", "1/16", "1/32" }, { "1/8", "1/16", "1/32" } },
        { "mt",   "Mod FX Type",  "Тип",      true,  0,    Fmt::Pct, false, { "Chorus", "Flanger" }, { "Хорус", "Фленджер" } },
        { "md",   "Mod FX Depth", "Глибина",  false, 0.5f, Fmt::Pct, false, {}, {} },
        { "mr",   "Mod FX Rate",  "Швидкість",false, 0.3f, Fmt::ModHz, false, {}, {} },
        { "mm",   "Mod FX Mix",   "Мікс",     false, 0.35f, Fmt::PctOff, false, {}, {} },
        // Page 6 — Delay, reverb, output
        { "dt",   "Delay Time",   "Час",      false, 0.45f, Fmt::DlySec, false, {}, {} },
        { "df",   "Delay Feedback","Повтори", false, 0.35f, Fmt::Pct, false, {}, {} },
        { "dm",   "Delay Mix",    "Мікс",     false, 0.2f, Fmt::PctOff, false, {}, {} },
        { "rt",   "Reverb Time",  "Час",      false, 0.5f, Fmt::RevSec, false, {}, {} },
        { "rm",   "Reverb Mix",   "Мікс",     false, 0.25f, Fmt::PctOff, false, {}, {} },
        { "vol",  "Output",       "Вихід",    false, 0.7f, Fmt::Pct, false, {}, {} },
    };
    return p;
}

inline const ParamDef* findParam (const juce::String& id)
{
    for (auto& p : params())
        if (id == p.id) return &p;
    return nullptr;
}

inline const char* chordNameEn (int i) { static const char* n[] = { "Octave", "Fifth", "Major", "Minor", "Maj7", "Min7", "Sus4", "5th+Oct" }; return n[i]; }
inline const char* chordNameUa (int i) { static const char* n[] = { "Октава", "Квінта", "Мажор", "Мінор", "Мажор 7", "Мінор 7", "Сус 4", "Квінта+окт" }; return n[i]; }
inline const char* arpNameEn (int i)   { static const char* n[] = { "Up", "Down", "Up-Down", "Random" }; return n[i]; }
inline const char* arpNameUa (int i)   { static const char* n[] = { "Вгору", "Вниз", "Вгору-вниз", "Випадково" }; return n[i]; }

inline juce::String secText (float s, bool ua)
{
    if (s < 1.0f) return juce::String (juce::roundToInt (s * 1000.0f)) + (ua ? UA (" мс") : juce::String (" ms"));
    return juce::String (s, 2) + (ua ? UA (" с") : juce::String (" s"));
}

inline juce::String hzText (float h, bool ua)
{
    if (h >= 1000.0f) return juce::String (h / 1000.0f, 1) + (ua ? UA (" кГц") : juce::String (" kHz"));
    return juce::String (juce::roundToInt (h)) + (ua ? UA (" Гц") : juce::String (" Hz"));
}

inline juce::String formatValue (Fmt f, float v, bool ua, int voiceMode)
{
    auto pct = [] (float x) { return juce::String (juce::roundToInt (x * 100.0f)) + "%"; };
    auto off = [ua] { return ua ? UA ("вимк") : juce::String ("Off"); };
    switch (f)
    {
        case Fmt::Pct:     return pct (v);
        case Fmt::PctOff:  return v < 0.01f ? off() : pct (v);
        case Fmt::CutHz:   return hzText (map::cutHz (v), ua);
        case Fmt::NoiseHz: return hzText (map::noiseHz (v), ua);
        case Fmt::Semis:   { int s = map::semis (v); return (s > 0 ? "+" : "") + juce::String (s) + (ua ? UA (" пт") : juce::String (" st")); }
        case Fmt::Cents:   { int c = juce::roundToInt (map::cents (v)); return (c > 0 ? "+" : "") + juce::String (c) + (ua ? UA (" ц") : juce::String (" ct")); }
        case Fmt::BiPct:   { int c = juce::roundToInt ((v - 0.5f) * 200.0f); return (c > 0 ? "+" : "") + juce::String (c) + "%"; }
        case Fmt::AmpA:    return secText (map::ampA (v), ua);
        case Fmt::AmpD:    return secText (map::ampD (v), ua);
        case Fmt::AmpR:    return secText (map::ampR (v), ua);
        case Fmt::EgA:     return secText (map::egA (v), ua);
        case Fmt::EgD:     return secText (map::egD (v), ua);
        case Fmt::LfoHz:   return juce::String (map::lfoHz (v), 2) + (ua ? UA (" Гц") : juce::String (" Hz"));
        case Fmt::ModHz:   return juce::String (map::modHz (v), 2) + (ua ? UA (" Гц") : juce::String (" Hz"));
        case Fmt::Glide:   return v < 0.01f ? off() : secText (map::glide (v), ua);
        case Fmt::DlySec:  return secText (map::dlySec (v), ua);
        case Fmt::RevSec:  return juce::String (map::revSec (v), 1) + (ua ? UA (" с") : juce::String (" s"));
        case Fmt::VoiceDepth:
        {
            const int i = map::idx8 (v);
            switch (voiceMode)
            {
                case 0:  return (ua ? UA ("дрейф ") : juce::String ("Drift ")) + pct (v);
                case 1:  return (ua ? UA ("розкид ") : juce::String ("Spread ")) + juce::String (juce::roundToInt (v * 50.0f)) + (ua ? UA (" ц") : juce::String (" ct"));
                case 2:  return ua ? UA (chordNameUa (i)) : juce::String (chordNameEn (i));
                default: return (ua ? UA (arpNameUa (i % 4)) : juce::String (arpNameEn (i % 4)))
                                + (i >= 4 ? (ua ? UA (" · 2 окт") : juce::String (" 2 Oct")) : (ua ? UA (" · 1 окт") : juce::String (" 1 Oct")));
            }
        }
    }
    return pct (v);
}

// ---- presets: English names for the host, Ukrainian for the GUI ----
struct Preset { const char* en; const char* ua; std::vector<std::pair<const char*, float>> values; };

inline const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        { "Warm Cloud", "Тепла хмара", { {"sh1",.3f},{"fn2",.58f},{"l1",.7f},{"l2",.7f},{"cut",.42f},{"res",.15f},{"aA",.45f},{"aD",.5f},{"aS",.8f},{"aR",.55f},{"eI",.58f},{"eD",.5f},{"lr",.25f},{"li",.12f},{"vd",.4f},{"mm",.45f},{"md",.6f},{"dm",.2f},{"rm",.4f},{"rt",.7f} } },
        { "Glass", "Скло", { {"w1",1},{"w2",1},{"o2",2},{"p2",.5f + 7.0f / 48.0f},{"fn2",.5f},{"ring",1},{"l1",.5f},{"l2",.7f},{"cut",.75f},{"res",.1f},{"aA",0},{"aD",.55f},{"aS",0},{"aR",.5f},{"eI",.5f},{"mm",.2f},{"rm",.45f},{"dm",.3f},{"df",.5f} } },
        { "Square Bass", "Квадратний бас", { {"vm",1},{"vd",.25f},{"w1",2},{"sh1",.3f},{"w2",0},{"o1",0},{"o2",0},{"fn2",.5f},{"cut",.3f},{"res",.45f},{"drv",.5f},{"eI",.78f},{"eD",.3f},{"aA",0},{"aD",.3f},{"aS",.6f},{"aR",.15f},{"port",.2f},{"mm",0},{"dm",.05f},{"rm",.05f} } },
        { "Saw Lead", "Пилка-лід", { {"vm",1},{"vd",.5f},{"w1",0},{"w2",0},{"xm",.25f},{"cut",.6f},{"res",.3f},{"eI",.65f},{"eD",.35f},{"port",.3f},{"lt",1},{"li",.12f},{"lr",.55f},{"dm",.35f},{"df",.45f},{"rm",.2f} } },
        { "Dust", "Пил", { {"ln",.6f},{"nc",.4f},{"l1",.4f},{"l2",.3f},{"cut",.45f},{"res",.55f},{"aA",.6f},{"aR",.7f},{"li",.4f},{"lr",.15f},{"rm",.55f},{"rt",.85f},{"mm",.5f},{"mt",1},{"md",.5f} } },
        { "Arcade", "Аркада", { {"vm",3},{"vd",.6f},{"latch",1},{"w1",2},{"sh1",.5f},{"w2",2},{"o2",2},{"l2",.3f},{"cut",.6f},{"res",.3f},{"aA",0},{"aD",.25f},{"aS",.2f},{"aR",.2f},{"eI",.65f},{"eD",.25f},{"dm",.3f},{"df",.45f} } },
        { "Chords", "Акорди", { {"vm",2},{"vd",.7f},{"w1",0},{"w2",2},{"cut",.5f},{"res",.3f},{"aA",0},{"aD",.4f},{"aS",.3f},{"aR",.3f},{"eI",.7f},{"eD",.3f},{"mm",.3f},{"dm",.25f},{"rm",.2f} } },
        { "Acid Poly", "Кислотний полі", { {"w1",0},{"w2",0},{"cut",0.25f},{"res",0.75f},{"eI",0.8f},{"eD",0.25f},{"aA",0},{"aD",0.3f},{"aS",0.4f},{"aR",0.15f},{"drv",0.5f},{"dm",0.25f},{"df",0.4f},{"port",0.15f} } },
        { "Electro Stab", "Електро-стаб", { {"vm",2},{"vd",0.45f},{"w1",2},{"w2",0},{"cut",0.45f},{"res",0.35f},{"eI",0.75f},{"eD",0.2f},{"aA",0},{"aD",0.25f},{"aS",0},{"aR",0.2f},{"drv",0.3f},{"mm",0.25f},{"dm",0.3f},{"df",0.45f},{"rm",0.2f} } },
        { "Detroit Pad", "Детройт-пед", { {"w1",0},{"w2",0},{"fn2",0.62f},{"cut",0.38f},{"res",0.25f},{"aA",0.5f},{"aD",0.6f},{"aS",0.85f},{"aR",0.6f},{"li",0.25f},{"lr",0.2f},{"mm",0.6f},{"md",0.7f},{"rm",0.5f},{"rt",0.75f},{"dm",0.2f} } },
        { "Noise Perc", "Шумова перкусія", { {"ln",0.9f},{"nc",0.75f},{"l1",0.2f},{"l2",0},{"cut",0.6f},{"res",0.5f},{"aA",0},{"aD",0.18f},{"aS",0},{"aR",0.12f},{"eI",0.8f},{"eD",0.15f},{"dm",0.35f},{"df",0.55f},{"rm",0.2f} } },
        { "Bee Growl", "Бджола", { {"w1",1},{"w2",0},{"p2",0.75f},{"xm",0.55f},{"cut",0.5f},{"res",0.3f},{"eI",0.7f},{"eD",0.3f},{"aA",0},{"aD",0.4f},{"aS",0.5f},{"aR",0.2f},{"lt",1},{"li",0.1f},{"lr",0.5f},{"drv",0.4f} } },
        { "IDM Bell", "Дзвін IDM", { {"w1",1},{"w2",2},{"sh2",0.7f},{"o2",2},{"p2",0.6042f},{"fn2",0.5f},{"ring",1},{"l1",0.4f},{"l2",0.8f},{"cut",0.8f},{"res",0.15f},{"aA",0},{"aD",0.6f},{"aS",0},{"aR",0.6f},{"dm",0.4f},{"df",0.6f},{"rm",0.5f},{"rt",0.8f} } },
        { "Sub Unison", "Суб-унісон", { {"vm",1},{"vd",0.15f},{"w1",0},{"w2",1},{"o1",0},{"o2",0},{"l2",0.8f},{"cut",0.28f},{"res",0.2f},{"eI",0.6f},{"eD",0.25f},{"aA",0},{"aD",0.3f},{"aS",0.8f},{"aR",0.15f},{"port",0.1f},{"drv",0.25f},{"mm",0},{"dm",0},{"rm",0} } },
        { "Glitch Arp", "Глітч-арп", { {"vm",3},{"vd",0.92f},{"latch",1},{"w1",2},{"sh1",0.7f},{"w2",2},{"o2",2},{"cut",0.55f},{"res",0.5f},{"aA",0},{"aD",0.15f},{"aS",0},{"aR",0.1f},{"eI",0.75f},{"eD",0.15f},{"li",0.3f},{"lr",0.75f},{"lw",2},{"dm",0.3f},{"df",0.5f} } },
        { "Machine Choir", "Хор машин", { {"w1",1},{"w2",1},{"sh1",0.6f},{"sh2",0.6f},{"fn2",0.6f},{"ln",0.15f},{"nc",0.5f},{"cut",0.5f},{"res",0.45f},{"aA",0.4f},{"aS",0.8f},{"aR",0.5f},{"lt",1},{"li",0.2f},{"lr",0.4f},{"mm",0.7f},{"md",0.8f},{"rm",0.45f} } },
        { "Flanged Lead", "Фленджер-лід", { {"vm",1},{"vd",0.35f},{"w1",0},{"w2",2},{"sh2",0.4f},{"cut",0.55f},{"res",0.35f},{"eI",0.65f},{"eD",0.4f},{"port",0.25f},{"mt",1},{"md",0.7f},{"mr",0.2f},{"mm",0.6f},{"dm",0.3f},{"df",0.4f} } },
    };
    return p;
}
} // namespace ml4
