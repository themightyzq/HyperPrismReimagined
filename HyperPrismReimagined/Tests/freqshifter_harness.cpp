// ============================================================================
// Frequency Shifter DSP measurement harness  (no JUCE dependency)
//
// Transcribes FrequencyShifterProcessor's DSP math exactly so we can measure,
// in dB, the three artifacts found during root-cause investigation and verify
// that the proposed fixes remove them:
//
//   A. dry/wet delay misalignment  -> comb filtering at mix < 100%
//   B. shared DSP state / single oscillator phase across stereo channels
//   C. residual opposite sideband (imperfect SSB cancellation)
//
// Build:  c++ -std=c++17 -O2 freqshifter_harness.cpp -o /tmp/fsh && /tmp/fsh
// ============================================================================
#include <vector>
#include <array>
#include <cmath>
#include <cstdio>
#include <algorithm>

static constexpr double kPi   = 3.14159265358979323846;
static constexpr double kTwoPi = 2.0 * kPi;

// ---------------------------------------------------------------------------
// Hilbert coefficient generation -- byte-for-byte the same formula as
// FrequencyShifterProcessor::HilbertTransform::createHilbertCoefficients()
// (generalised to arbitrary order + window so we can test improvements).
// ---------------------------------------------------------------------------
enum class Window { Hann, Blackman };

static std::vector<float> makeHilbert(int order, Window win)
{
    std::vector<float> h(order);
    for (int i = 0; i < order; ++i)
    {
        int n = i - order / 2;
        if (n == 0 || (n % 2 == 0)) h[i] = 0.0f;
        else                        h[i] = 2.0f / (float)(kPi * n);

        double w;
        if (win == Window::Hann)
            w = 0.5 * (1.0 - std::cos(kTwoPi * i / (order - 1)));
        else // Blackman
            w = 0.42 - 0.5 * std::cos(kTwoPi * i / (order - 1))
                     + 0.08 * std::cos(2.0 * kTwoPi * i / (order - 1));
        h[i] *= (float)w;
    }
    return h;
}

// Direct-form FIR, same convention as juce::dsp::FIR::Filter:
// out = sum_k coeff[k] * x[n-k], coeff[0] weights the newest sample.
struct FIR
{
    std::vector<float> coeff, z;
    void init(const std::vector<float>& c) { coeff = c; z.assign(c.size(), 0.0f); }
    void reset() { std::fill(z.begin(), z.end(), 0.0f); }
    float process(float x)
    {
        for (int i = (int)z.size() - 1; i > 0; --i) z[i] = z[i - 1];
        z[0] = x;
        float acc = 0.0f;
        for (size_t i = 0; i < coeff.size(); ++i) acc += coeff[i] * z[i];
        return acc;
    }
};

// Integer delay line (juce::dsp::DelayLine with linear interp at integer delay
// is an exact integer delay -> plain ring buffer).
struct Delay
{
    std::vector<float> buf; int w = 0, d = 0;
    void init(int delaySamples, int maxLen)
    { buf.assign((size_t)maxLen + 1, 0.0f); d = delaySamples; w = 0; }
    void reset() { std::fill(buf.begin(), buf.end(), 0.0f); w = 0; }
    float process(float x)
    {
        buf[(size_t)w] = x;
        int r = w - d; if (r < 0) r += (int)buf.size();
        w = (w + 1) % (int)buf.size();
        return buf[(size_t)r];
    }
};

// ---------------------------------------------------------------------------
struct Params { float shiftHz; float mix; double fs; };

// === BUGGY model: exact transcription of current processFrequencyShifting ===
//  - one shared FIR + one shared delay line for BOTH channels
//  - oscillator phase runs continuously through ch0 then ch1
//  - dry path (input) is NOT delay-compensated
static void processBuggy(std::vector<float>** io, int numCh, int N,
                         int order, Window win, Params p, int blockSize)
{
    FIR fir; fir.init(makeHilbert(order, win));
    Delay del; del.init(order / 2, order);
    double phase = 0.0;
    const double inc = kTwoPi * p.shiftHz / p.fs;
    const float mix = p.mix;

    for (int start = 0; start < N; start += blockSize)
    {
        int n = std::min(blockSize, N - start);
        for (int ch = 0; ch < numCh; ++ch)
        {
            double ph = phase;            // each channel restarts from block phase...
            for (int s = 0; s < n; ++s)
            {
                float in = (*io[ch])[start + s];
                float imag = fir.process(in);     // SHARED fir state across channels
                float real = del.process(in);     // SHARED delay across channels
                float c = (float)std::cos(ph), sn = (float)std::sin(ph);
                ph += inc; if (ph >= kTwoPi) ph -= kTwoPi;
                float shifted = real * c - imag * sn;
                (*io[ch])[start + s] = in * (1.0f - mix) + shifted * mix;
            }
            phase = ph;                   // ...but phase leaks from ch0 into ch1
        }
    }
}

// === FIXED model ===
//  - per-channel FIR + delay state
//  - single oscillator advanced once per sample, shared by all channels
//  - dry path delay-compensated to match the wet path group delay
static void processFixed(std::vector<float>** io, int numCh, int N,
                         int order, Window win, Params p, int blockSize)
{
    std::array<FIR, 2>   fir;
    std::array<Delay, 2> wetAlign;  // present already as delayLine in real code
    std::array<Delay, 2> dryComp;   // NEW: compensate dry path
    auto coeff = makeHilbert(order, win);
    for (int ch = 0; ch < 2; ++ch)
    { fir[ch].init(coeff); wetAlign[ch].init(order / 2, order); dryComp[ch].init(order / 2, order); }

    double phase = 0.0;
    const double inc = kTwoPi * p.shiftHz / p.fs;
    const float mix = p.mix;

    for (int start = 0; start < N; start += blockSize)
    {
        int n = std::min(blockSize, N - start);
        for (int s = 0; s < n; ++s)
        {
            float c = (float)std::cos(phase), sn = (float)std::sin(phase);
            phase += inc; if (phase >= kTwoPi) phase -= kTwoPi;   // once per sample
            for (int ch = 0; ch < numCh; ++ch)
            {
                float in = (*io[ch])[start + s];
                float imag = fir[ch].process(in);
                float real = wetAlign[ch].process(in);
                float dry  = dryComp[ch].process(in);             // aligned dry
                float shifted = real * c - imag * sn;
                (*io[ch])[start + s] = dry * (1.0f - mix) + shifted * mix;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Goertzel single-bin magnitude (skip warmup samples to ignore FIR fill).
static double goertzel(const float* x, int N, int skip, double freq, double fs)
{
    double w = kTwoPi * freq / fs;
    double coeff = 2.0 * std::cos(w);
    double s0 = 0, s1 = 0, s2 = 0;
    for (int n = skip; n < N; ++n) { s0 = x[n] + coeff * s1 - s2; s2 = s1; s1 = s0; }
    double re = s1 - s2 * std::cos(w), im = s2 * std::sin(w);
    return std::sqrt(re * re + im * im) / (N - skip);
}

static double dB(double x) { return 20.0 * std::log10(std::max(x, 1e-12)); }

// ---------------------------------------------------------------------------
int main()
{
    const double fs = 48000.0;
    const int    N  = (int)fs;     // 1 s -> 1 Hz bins (integer freqs land on bins)
    const int    skip = 2048;      // discard FIR/delay warmup
    const int    block = 512;      // realistic host block size

    auto sine = [&](double f) {
        std::vector<float> v(N);
        for (int n = 0; n < N; ++n) v[n] = (float)std::sin(kTwoPi * f * n / fs);
        return v;
    };

    printf("=================================================================\n");
    printf(" Frequency Shifter artifact measurements  (fs=%.0f, block=%d)\n", fs, block);
    printf("=================================================================\n\n");

    // ---- Metric C: opposite-sideband rejection ----------------------------
    // Input 1000 Hz, shift +200 Hz, 100%% wet. Wanted tone = 1200 Hz,
    // image (artifact) = 800 Hz. Lower = better rejection.
    auto sidebandTest = [&](const char* name,
        void(*fn)(std::vector<float>**,int,int,int,Window,Params,int),
        int order, Window win)
    {
        std::vector<float> ch = sine(1000.0);
        std::vector<float>* io[1] = { &ch };
        fn(io, 1, N, order, win, { 200.0f, 1.0f, fs }, block);
        double up = goertzel(ch.data(), N, skip, 1200.0, fs);
        double lo = goertzel(ch.data(), N, skip,  800.0, fs);
        printf("  %-26s wanted=%.1f dB  image=%.1f dB  rejection=%.1f dB\n",
               name, dB(up), dB(lo), dB(lo) - dB(up));
    };
    printf("[C] Opposite-sideband rejection (more negative rejection = better):\n");
    sidebandTest("current 256/Hann",   processBuggy, 256, Window::Hann);
    sidebandTest("fixed   256/Hann",   processFixed, 256, Window::Hann);
    sidebandTest("fixed   257/Hann",   processFixed, 257, Window::Hann);
    sidebandTest("fixed   257/Blackman",processFixed,257, Window::Blackman);
    sidebandTest("fixed   513/Blackman",processFixed,513, Window::Blackman);
    printf("\n");

    // ---- Metric A: comb filtering from dry/wet misalignment ---------------
    // shift=0, mix=50%%. Sweep freqs, report deepest notch. 0 dB = flat (good).
    auto combTest = [&](const char* name,
        void(*fn)(std::vector<float>**,int,int,int,Window,Params,int),
        int order)
    {
        double worst = 0.0; double worstF = 0;
        for (double f = 100.0; f <= 8000.0; f += 100.0)
        {
            std::vector<float> ch = sine(f);
            std::vector<float>* io[1] = { &ch };
            fn(io, 1, N, order, Window::Hann, { 0.0f, 0.5f, fs }, block);
            double g = dB(goertzel(ch.data(), N, skip, f, fs)
                       / (1.0/std::sqrt(2.0)));   // input sine RMS-mag reference
            if (g < worst) { worst = g; worstF = f; }
        }
        printf("  %-26s deepest notch = %.1f dB @ %.0f Hz\n", name, worst, worstF);
    };
    printf("[A] Dry/wet comb (shift=0, mix=50%%; 0 dB = flat, good):\n");
    combTest("current (undelayed dry)", processBuggy, 256);
    combTest("fixed   (aligned dry)",   processFixed, 256);
    printf("\n");

    // ---- Metric B: stereo channel divergence ------------------------------
    // Feed L=1000Hz, R=1500Hz together; compare each channel to processing it
    // in isolation. Nonzero = cross-channel contamination.
    auto stereoTest = [&](const char* name,
        void(*fn)(std::vector<float>**,int,int,int,Window,Params,int))
    {
        std::vector<float> L = sine(1000.0), R = sine(1500.0);
        std::vector<float>* st[2] = { &L, &R };
        fn(st, 2, N, 256, Window::Hann, { 137.0f, 0.5f, fs }, block);

        std::vector<float> Lonly = sine(1000.0), dummy(N, 0.0f);
        std::vector<float>* s1[2] = { &Lonly, &dummy };
        fn(s1, 2, N, 256, Window::Hann, { 137.0f, 0.5f, fs }, block);

        double err = 0, ref = 0;
        for (int n = skip; n < N; ++n)
        { double d = L[n] - Lonly[n]; err += d*d; ref += Lonly[n]*Lonly[n]; }
        printf("  %-26s L-channel divergence = %.1f dB\n",
               name, dB(std::sqrt(err / std::max(ref, 1e-12))));
    };
    printf("[B] Stereo cross-channel contamination (lower = better; -inf = none):\n");
    stereoTest("current (shared state)", processBuggy);
    stereoTest("fixed   (per-channel)",  processFixed);
    printf("\n");

    return 0;
}
