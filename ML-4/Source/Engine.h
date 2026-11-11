#pragma once
#include "Params.h"
#include <array>
#include <algorithm>

namespace ml4
{
struct P
{
    int w1 = 0, o1 = 1, w2 = 0, o2 = 1, ring = 0, kt = 1, poles = 1, eT = 0, lw = 0, lt = 0, vm = 0, latch = 0, arate = 1, mt = 0;
    float sh1 = .2f, sh2 = .2f, p2 = .5f, fn2 = .56f, xm = 0, l1 = .8f, l2 = .6f, ln = 0, nc = .6f, drv = .1f, port = 0;
    float cut = .55f, res = .2f, eA = 0, eD = .35f, eI = .62f, aA = .05f, aD = .4f, aS = .7f, aR = .35f, lr = .35f, li = .12f;
    float vd = .3f, md = .5f, mr = .3f, mm = .35f, dt = .45f, df = .35f, dm = .2f, rt = .5f, rm = .25f, vol = .7f;
};

static constexpr int kOct[] = { -1, 0, 1, 2 };
static constexpr float kPi = 3.14159265358979f;

inline float mtof (float m) { return 440.0f * std::exp2 ((m - 69.0f) / 12.0f); }

// ---------------- oscillator (PolyBLEP) ----------------
struct Osc
{
    double ph = 0.0;

    static float blep (float t, float dt)
    {
        if (dt <= 1.0e-6f) return 0.0f;
        if (t < dt)        { t /= dt; return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }
    static float saw (double p, float dt) { return (float) (2.0 * p - 1.0) - blep ((float) p, dt); }

    float tick (int wave, float shape, float freq, float sr)
    {
        const float dt = juce::jlimit (0.0f, 0.45f, freq / sr);
        float y;
        if (wave == 0)
        {
            y = saw (ph, dt);
            if (shape > 0.001f)
            {
                double p2 = ph * 2.0; p2 -= std::floor (p2);
                y = y * (1.0f - shape * 0.5f) + shape * 0.6f * saw (p2, std::min (0.45f, dt * 2.0f));
            }
        }
        else if (wave == 2)
        {
            const float pw = 0.5f - shape * 0.46f;
            double p2 = ph + pw; p2 -= std::floor (p2);
            y = saw (ph, dt) - saw (p2, dt) + (2.0f * pw - 1.0f);
        }
        else
        {
            float t = 4.0f * std::abs ((float) ph - 0.5f) - 1.0f;
            if (shape > 0.001f)
            {
                t *= 1.0f + shape * 3.0f;
                for (int i = 0; i < 4 && std::abs (t) > 1.0f; ++i)
                    t = t > 1.0f ? 2.0f - t : -2.0f - t;
            }
            y = t;
        }
        ph += dt;
        if (ph >= 1.0) ph -= 1.0;
        return y;
    }
};

// ---------------- filter: 4-pole ZDF ladder / 2-pole TPT SVF ----------------
struct Filter
{
    float s[4] {}, z1 = 0, z2 = 0;
    void reset() { std::fill (std::begin (s), std::end (s), 0.0f); z1 = z2 = 0; }

    float process (float x, float fc, float res, bool fourPole, float sr)
    {
        fc = juce::jlimit (15.0f, sr * 0.45f, fc);
        const float g = std::tan (kPi * fc / sr);
        if (fourPole)
        {
            const float G = g / (1.0f + g), b = 1.0f / (1.0f + g);
            const float k = res * 3.95f;
            const float sigma = G * G * G * b * s[0] + G * G * b * s[1] + G * b * s[2] + b * s[3];
            const float G4 = G * G * G * G;
            const float y4 = (G4 * x + sigma) / (1.0f + k * G4);
            float u = std::tanh (x - k * y4);
            for (int i = 0; i < 4; ++i)
            {
                const float v = (u - s[i]) * G;
                const float y = v + s[i];
                s[i] = y + v;
                u = y;
            }
            return u * (1.0f + k * 0.45f);
        }
        const float q = 0.5f + res * res * 24.0f, R = 1.0f / (2.0f * q);
        const float hp = (x - (2.0f * R + g) * z1 - z2) / (1.0f + 2.0f * R * g + g * g);
        const float bp = g * hp + z1; z1 = g * hp + bp;
        const float lp = g * bp + z2; z2 = g * bp + lp;
        return std::tanh (lp);
    }
};

// ---------------- voice ----------------
struct Voice
{
    Osc o1, o2;
    Filter f;
    juce::ADSR amp;
    float eg = 0; int egStage = 0; float egAttackLeft = 0;
    float curNote = 60, target = 60, cents = 0;
    bool held = false, hasPlayed = false, forceGlide = false;
    double startT = 0, offT = -1;
    int key = -1;
    float noiseLp = 0; uint32_t rng = 0x9E3779B9u;

    void prepare (double sr) { amp.setSampleRate (sr); }

    void setAmp (const P& p)
    {
        juce::ADSR::Parameters a;
        a.attack = map::ampA (p.aA); a.decay = map::ampD (p.aD); a.sustain = p.aS; a.release = map::ampR (p.aR);
        amp.setParameters (a);
    }

    void start (int note, float c, int k, double now, const P& p, float sr)
    {
        target = (float) note; cents = c; key = k;
        if (! (hasPlayed && p.port > 0.01f)) curNote = target;
        forceGlide = false; hasPlayed = true;
        held = true; startT = now; offT = 1.0e300;
        setAmp (p);
        amp.noteOn();
        eg = 0; egStage = 1; egAttackLeft = map::egA (p.eA) * sr;
    }
    void glideTo (int note, float c, int k) { target = (float) note; cents = c; key = k; forceGlide = true; }
    void stop (double now) { if (! held) return; held = false; offT = now; amp.noteOff(); }

    float render (const P& p, float sr, float lfo)
    {
        if (! amp.isActive()) return 0.0f;

        // glide
        if (curNote != target)
        {
            const float tSec = forceGlide ? std::max (0.012f, map::glide (p.port)) : map::glide (p.port);
            if (tSec < 0.0005f) curNote = target;
            else { const float c = 1.0f - std::exp (-1.0f / (tSec / 3.0f * sr)); curNote += (target - curNote) * c; if (std::abs (target - curNote) < 0.0005f) curNote = target; }
        }
        // mod envelope (AD)
        if (egStage == 1) { eg += 1.0f / std::max (1.0f, egAttackLeft); if (eg >= 1.0f) { eg = 1.0f; egStage = 2; } }
        else if (egStage == 2) { eg *= std::exp (-1.0f / (map::egD (p.eD) / 3.0f * sr)); }

        const float amt = (p.eI - 0.5f) * 2.0f;
        const float egCut = p.eT == 0 ? amt * 4800.0f * eg : 0.0f;
        const float egP1  = p.eT == 1 ? amt * 2400.0f * eg : 0.0f;
        const float egP2  = p.eT >= 1 ? amt * 2400.0f * eg : 0.0f;
        const float lfoP  = p.lt == 1 ? p.li * p.li * 250.0f * lfo : 0.0f;
        const float lfoC  = p.lt == 0 ? p.li * 2400.0f * lfo : 0.0f;

        const float n1 = curNote + 12.0f * (float) kOct[p.o1];
        const float n2 = curNote + 12.0f * (float) kOct[p.o2] + (float) map::semis (p.p2) + map::cents (p.fn2) / 100.0f;
        const float f1 = mtof (n1 + (cents + egP1 + lfoP) / 100.0f);
        const float f2 = mtof (n2 + (cents + egP2 + lfoP) / 100.0f);

        const float s2 = o2.tick (p.w2, p.sh2, f2, sr);
        const float fm = p.xm * p.xm * f1 * 4.0f * s2;
        const float s1 = o1.tick (p.w1, p.sh1, std::max (0.0f, f1 + fm), sr);
        const float v2 = p.ring ? s1 * s2 * p.l2 * 1.2f : s2 * p.l2;

        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        const float white = (float) rng / 2147483648.0f - 1.0f;
        const float nk = 1.0f - std::exp (-2.0f * kPi * map::noiseHz (p.nc) / sr);
        noiseLp += (white - noiseLp) * nk;

        float x = (s1 * p.l1 + v2 + noiseLp * p.ln * 0.9f) * 0.45f;
        x = std::tanh (2.5f * juce::jlimit (-1.0f, 1.0f, x * (1.0f + p.drv * p.drv * 10.0f))) / std::tanh (2.5f);

        const float kt = p.kt == 0 ? 0.0f : (p.kt == 1 ? 0.5f : 1.0f);
        const float base = std::min (20000.0f, map::cutHz (p.cut) * std::exp2 ((curNote - 60.0f) / 12.0f * kt));
        const float fc = base * std::exp2 ((egCut + lfoC) / 1200.0f);
        const float y = f.process (x, fc, p.res, p.poles == 1, sr);

        return y * amp.getNextSample() * 0.75f;
    }
};

// ---------------- effects ----------------
struct DelayLine
{
    std::vector<float> b; int w = 0;
    void init (int n) { b.assign ((size_t) std::max (4, n), 0.0f); w = 0; }
    void clear() { std::fill (b.begin(), b.end(), 0.0f); }
    float read (float d) const
    {
        const int n = (int) b.size();
        float rp = (float) w - juce::jlimit (1.0f, (float) n - 2.0f, d);
        while (rp < 0) rp += (float) n;
        const int i0 = (int) rp; const float fr = rp - (float) i0; const int i1 = (i0 + 1) % n;
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr;
    }
    void push (float x) { b[(size_t) w] = x; w = (w + 1) % (int) b.size(); }
};

struct Effects
{
    DelayLine mL, mR, dL, dR;
    double sr = 44100;
    float mPh = 0, dTime = -1, dLp = 0;
    juce::Reverb reverb;

    void prepare (double s)
    {
        sr = s;
        mL.init ((int) (0.05 * sr)); mR.init ((int) (0.05 * sr));
        dL.init ((int) (1.2 * sr));  dR.init ((int) (1.2 * sr));
        reverb.setSampleRate (sr); reverb.reset();
        dTime = -1;
    }
    void reset() { mL.clear(); mR.clear(); dL.clear(); dR.clear(); reverb.reset(); dLp = 0; }

    void process (float* L, float* R, int n, const P& p)
    {
        const bool fl = p.mt == 1;
        const float base = fl ? 0.003f : 0.016f, dep = p.md * (fl ? 0.0026f : 0.006f);
        const float fb = fl ? 0.25f + p.md * 0.55f : 0.0f, mix = p.mm * 0.9f;
        const float mInc = map::modHz (p.mr) / (float) sr;
        const float dTarget = map::dlySec (p.dt) * (float) sr;
        if (dTime < 0) dTime = dTarget;
        const float dfb = p.df * 0.85f, send = p.dm * 0.9f;
        const float lpk = 1.0f - std::exp (-2.0f * kPi * 4500.0f / (float) sr);

        for (int i = 0; i < n; ++i)
        {
            const float in = L[i];
            float l = in, r = in;
            if (mix > 0.0005f || fl)
            {
                mPh += mInc; if (mPh >= 1.0f) mPh -= 1.0f;
                const float s = std::sin (2.0f * kPi * mPh);
                const float wl = mL.read ((base + dep * s) * (float) sr);
                const float wr = mR.read ((base * 1.13f - dep * s) * (float) sr);
                mL.push (in + wl * fb); mR.push (in + wr * fb);
                l += wl * mix; r += wr * mix;
            }
            dTime += (dTarget - dTime) * 0.0005f;
            dLp += ((l + r) * 0.5f * send - dLp) * lpk;
            const float yl = dL.read (dTime), yr = dR.read (dTime);
            dL.push (dLp + yr * dfb); dR.push (yl * dfb);
            L[i] = l + yl; R[i] = r + yr;
        }
        if (p.rm > 0.005f)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize = std::min (0.98f, 0.35f + p.rt * 0.63f);
            rp.damping = 0.45f; rp.wetLevel = p.rm * 0.7f; rp.dryLevel = 1.0f; rp.width = 1.0f;
            reverb.setParameters (rp);
            reverb.processStereo (L, R, n);
        }
    }
};

// ---------------- engine: voice routing, arp, mix ----------------
class Engine
{
public:
    void prepare (double s)
    {
        sr = (float) s;
        for (auto& v : voices) { v.prepare (s); v.amp.reset(); v.f.reset(); }
        fx.prepare (s);
        mono.clear(); arpHeld.clear(); arpKey = -1;
    }

    void setParams (const P& np)
    {
        if (np.vm != p.vm) allOff();
        if (p.latch && ! np.latch) arpHeld.clear();
        p = np;
    }

    void allOff()
    {
        for (auto& v : voices) v.stop (now);
        mono.clear(); arpHeld.clear(); arpKey = -1; arpCountdown = 0;
    }

    void midiNoteOn (int note)
    {
        if (p.vm == 3)
        {
            auto it = std::find (arpHeld.begin(), arpHeld.end(), note);
            if (p.latch) { if (it != arpHeld.end()) arpHeld.erase (it); else arpHeld.push_back (note); }
            else if (it == arpHeld.end()) arpHeld.push_back (note);
            return;
        }
        mono.erase (std::remove (mono.begin(), mono.end(), note), mono.end());
        mono.push_back (note);
        playNote (note, note, true);
    }

    void midiNoteOff (int note)
    {
        if (p.vm == 3)
        {
            if (! p.latch) arpHeld.erase (std::remove (arpHeld.begin(), arpHeld.end(), note), arpHeld.end());
            return;
        }
        releaseNote (note, true);
    }

    void render (float* L, float* R, int n, double bpm)
    {
        const float lfoInc = map::lfoHz (p.lr) / sr;
        const double beats = p.arate == 0 ? 0.5 : (p.arate == 1 ? 0.25 : 0.125);
        const double stepLen = 60.0 / std::max (20.0, bpm) * beats * sr;

        for (int i = 0; i < n; ++i)
        {
            // arp
            if (p.vm == 3 && ! arpHeld.empty())
            {
                if (arpCountdown <= 0)
                {
                    if (arpKey >= 0) releaseNote (arpKey, false);
                    const int note = nextArpNote();
                    arpKey = 100000 + (int) (++arpCounter % 100000);
                    playNote (arpKey, note, false);
                    arpOffIn = stepLen * 0.75;
                    arpCountdown += stepLen;
                }
                arpCountdown -= 1.0;
            }
            else { arpCountdown = 0; arpIdx = 0; }
            if (arpKey >= 0) { arpOffIn -= 1.0; if (arpOffIn <= 0) { releaseNote (arpKey, false); arpKey = -1; } }

            // LFO
            lfoPh += lfoInc; if (lfoPh >= 1.0f) lfoPh -= 1.0f;
            const float lfo = p.lw == 0 ? 1.0f - 4.0f * std::abs (lfoPh - 0.5f) : (p.lw == 1 ? 2.0f * lfoPh - 1.0f : (lfoPh < 0.5f ? 1.0f : -1.0f));

            float s = 0;
            for (auto& v : voices) s += v.render (p, sr, lfo);
            if (p.lt == 2) s *= 1.0f - p.li * 0.5f + p.li * 0.5f * lfo;
            L[i] = s * 0.32f;
            now += 1.0;
        }
        fx.process (L, R, n, p);
        const float g = p.vol * 1.6f;
        for (int i = 0; i < n; ++i) { L[i] = std::tanh (L[i] * g) * 0.9f; R[i] = std::tanh (R[i] * g) * 0.9f; }
    }

    int voiceMode() const { return p.vm; }

private:
    P p;
    std::array<Voice, 4> voices;
    Effects fx;
    float sr = 44100, lfoPh = 0;
    double now = 0;
    std::vector<int> mono, arpHeld;
    int arpKey = -1, arpIdx = 0; long long arpCounter = 0;
    double arpCountdown = 0, arpOffIn = 0;
    juce::Random rnd;

    static const std::vector<int>& chord (int i)
    {
        static const std::vector<std::vector<int>> c = { {0,12}, {0,7}, {0,4,7}, {0,3,7}, {0,4,7,11}, {0,3,7,10}, {0,5,7}, {0,7,12,19} };
        return c[(size_t) i];
    }

    Voice& alloc()
    {
        Voice* best = &voices[0]; double bs = 1.0e301;
        for (auto& v : voices)
        {
            const double s = v.held ? 1.0e200 + v.startT : (v.amp.isActive() ? v.offT : -1.0e9 + v.offT);
            if (s < bs) { bs = s; best = &v; }
        }
        return *best;
    }

    void playNote (int key, int note, bool live)
    {
        const int mode = p.vm == 3 ? 0 : p.vm;
        if (mode == 0)
        {
            alloc().start (note, (rnd.nextFloat() * 2.0f - 1.0f) * p.vd * 14.0f, key, now, p, sr);
            return;
        }
        static const std::vector<int> uni = { 0, 0, 0, 0 };
        const auto& ints = mode == 1 ? uni : chord (map::idx8 (p.vd));
        bool legato = false;
        if (live) for (auto& v : voices) legato = legato || v.held;
        for (size_t i = 0; i < voices.size(); ++i)
        {
            auto& v = voices[i];
            if (i < ints.size())
            {
                const float c = mode == 1 ? ((float) i - 1.5f) / 1.5f * p.vd * 50.0f : (rnd.nextFloat() * 2.0f - 1.0f) * 3.0f;
                if (legato && v.held) v.glideTo (note + ints[i], c, key);
                else v.start (note + ints[i], c, key, now, p, sr);
            }
            else if (v.held) v.stop (now);
        }
    }

    void releaseNote (int key, bool live)
    {
        if (live && (p.vm == 1 || p.vm == 2))
        {
            mono.erase (std::remove (mono.begin(), mono.end(), key), mono.end());
            if (! mono.empty()) { playNote (mono.back(), mono.back(), true); return; }
            for (auto& v : voices) v.stop (now);
            return;
        }
        for (auto& v : voices) if (v.key == key && v.held) v.stop (now);
    }

    int nextArpNote()
    {
        std::vector<int> base (arpHeld);
        std::sort (base.begin(), base.end());
        const int i = map::idx8 (p.vd), pat = i % 4, oct = i >= 4 ? 2 : 1;
        std::vector<int> s;
        for (int o = 0; o < oct; ++o) for (int nn : base) s.push_back (nn + 12 * o);
        if (pat == 1) std::reverse (s.begin(), s.end());
        if (pat == 2 && s.size() > 2)
            for (int k = (int) s.size() - 2; k >= 1; --k) { const int v = s[(size_t) k]; s.push_back (v); }
        if (s.empty()) return 60;
        if (pat == 3) return s[(size_t) rnd.nextInt ((int) s.size())];
        return s[(size_t) (arpIdx++ % (int) s.size())];
    }
};
} // namespace ml4
