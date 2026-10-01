// hp_rt_alloc_<Effect>: proves processBlock() does not allocate. Counts every heap allocation
// made on the calling thread while processBlock() runs (operator new everywhere; malloc,
// calloc and realloc too on macOS and Linux, which is what juce::HeapBlock and so
// juce::AudioBuffer use), across 400 blocks with:
//   - every parameter (bool and choice included) set to a new random value every few blocks,
//     outside the counted region, so coefficient rebuilds, band-count changes and bypass
//     toggles all happen inside processBlock();
//   - host block sizes that change from block to block (512, 100, 384, 1, 333, 64), all at
//     or below the prepared size;
//   - one host block of twice the prepared size every 7 blocks, which the processor must
//     split into prepared-size chunks rather than grow a buffer (or overrun one).
// Also fails on any non-finite output sample. Built per plugin by
// add_hyperprism_console_check(rt_alloc_check ...) in CMakeLists.txt. Exit 0 on pass.

#include HP_CHECK_PROCESSOR_HEADER
#include <atomic>
#include <cmath>
#include <iterator>
#include <cstdlib>
#include <iostream>
#include <new>

namespace
{
thread_local bool countingThisThread = false;
std::atomic<int> allocationCount { 0 };

inline void noteAllocation() noexcept
{
    if (countingThisThread)
        allocationCount.fetch_add (1, std::memory_order_relaxed);
}
}

//==============================================================================
// operator new: replaced program-wide (standard-sanctioned). Forwards to the C allocator.
void* operator new (std::size_t size)
{
    noteAllocation();
    if (void* p = std::malloc (size == 0 ? 1 : size))
        return p;
    throw std::bad_alloc();
}

void* operator new[] (std::size_t size)
{
    noteAllocation();
    if (void* p = std::malloc (size == 0 ? 1 : size))
        return p;
    throw std::bad_alloc();
}

void operator delete (void* p) noexcept                  { std::free (p); }
void operator delete[] (void* p) noexcept                { std::free (p); }
void operator delete (void* p, std::size_t) noexcept     { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept   { std::free (p); }

//==============================================================================
// malloc/calloc/realloc: defined in this executable so the JUCE code statically linked into it
// binds to these, forwarding to the platform allocator. free() is left alone: the memory comes
// from the same allocator. Not done on Windows, where the CRT's malloc cannot be replaced
// this way; there only operator new is counted.
#if defined (__APPLE__)
 #include <malloc/malloc.h>
extern "C"
{
void* malloc (size_t size)                { noteAllocation(); return malloc_zone_malloc (malloc_default_zone(), size); }
void* calloc (size_t n, size_t size)      { noteAllocation(); return malloc_zone_calloc (malloc_default_zone(), n, size); }
void* realloc (void* p, size_t size)
{
    noteAllocation();
    if (p == nullptr)
        return malloc_zone_malloc (malloc_default_zone(), size);
    if (auto* zone = malloc_zone_from_ptr (p))
        return malloc_zone_realloc (zone, p, size);
    return malloc_zone_realloc (malloc_default_zone(), p, size);
}
}
#elif defined (__linux__) && defined (__GLIBC__)
extern "C"
{
void* __libc_malloc (size_t);
void* __libc_calloc (size_t, size_t);
void* __libc_realloc (void*, size_t);
void* malloc (size_t size)                { noteAllocation(); return __libc_malloc (size); }
void* calloc (size_t n, size_t size)      { noteAllocation(); return __libc_calloc (n, size); }
void* realloc (void* p, size_t size)      { noteAllocation(); return __libc_realloc (p, size); }
}
#endif

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    constexpr double sampleRate = 48000.0;
    constexpr int preparedBlock = 512;
    constexpr int numChannels = 2;

    HP_CHECK_PROCESSOR_CLASS proc;
    proc.setPlayConfigDetails (numChannels, numChannels, sampleRate, preparedBlock);
    proc.prepareToPlay (sampleRate, preparedBlock);

    juce::AudioBuffer<float> storage (numChannels, 2 * preparedBlock);
    juce::MidiBuffer midi;
    juce::Random rng (7);

    const int sizes[] = { 512, 100, 384, 1, 333, 64 };
    int failures = 0;
    int reported = 0;
    bool nonFinite = false;

    for (int iteration = 0; iteration < 400; ++iteration)
    {
        if (iteration % 3 == 0)
            for (auto* p : proc.getParameters())
                p->setValueNotifyingHost (rng.nextFloat());

        const int numSamples = (iteration % 7 == 6) ? 2 * preparedBlock
                                                     : sizes[iteration % (int) std::size (sizes)];
        for (int ch = 0; ch < numChannels; ++ch)
            for (int i = 0; i < numSamples; ++i)
                storage.setSample (ch, i, 0.5f * (rng.nextFloat() * 2.0f - 1.0f));

        juce::AudioBuffer<float> block (storage.getArrayOfWritePointers(), numChannels, 0, numSamples);

        const int before = allocationCount.load();
        countingThisThread = true;
        proc.processBlock (block, midi);
        countingThisThread = false;
        const int made = allocationCount.load() - before;

        if (made > 0)
        {
            ++failures;
            if (reported++ < 8)
                std::cerr << "FAIL block " << iteration << " (" << numSamples << " samples): "
                          << made << " heap allocation(s) inside processBlock\n";
        }

        for (int ch = 0; ch < numChannels && ! nonFinite; ++ch)
            for (int i = 0; i < numSamples; ++i)
                if (! std::isfinite (block.getSample (ch, i)))
                {
                    nonFinite = true;
                    std::cerr << "FAIL non-finite output at block " << iteration << "\n";
                    break;
                }
    }

    if (nonFinite)
        ++failures;

    std::cout << "blocks that allocated: " << (nonFinite ? failures - 1 : failures) << " of 400\n";
    std::cout << (failures == 0 ? "PASS" : "FAIL") << " real-time allocation check\n";
    return failures == 0 ? 0 : 1;
}
