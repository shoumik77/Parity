#pragma once

#include <array>
#include <atomic>

//==============================================================================
/**
    Fixed-size single-producer / single-consumer ring of plain-data samples
    for time-history displays. The audio thread push()es; the UI thread
    read()s a chronologically ordered snapshot. Writes are lock-free and
    allocation-free. A reader may observe a slot mid-write (torn fields),
    which is acceptable for display data.
*/
template <typename Sample, int Capacity>
class HistoryBuffer
{
public:
    static constexpr int capacity = Capacity;

    void clear() noexcept
    {
        writeIndex.store (0, std::memory_order_relaxed);
        count.store (0, std::memory_order_release);
    }

    /** Audio thread. */
    void push (const Sample& sample) noexcept
    {
        const auto index = writeIndex.load (std::memory_order_relaxed);
        slots[(size_t) index] = sample;
        writeIndex.store ((index + 1) % Capacity, std::memory_order_release);

        if (count.load (std::memory_order_relaxed) < Capacity)
            count.fetch_add (1, std::memory_order_release);
    }

    /** UI thread: fills `out` oldest-first and returns the number of valid samples. */
    int read (std::array<Sample, Capacity>& out) const noexcept
    {
        const auto numValid = count.load (std::memory_order_acquire);
        const auto end = writeIndex.load (std::memory_order_acquire);
        auto start = (end - numValid + Capacity) % Capacity;

        for (int i = 0; i < numValid; ++i)
        {
            out[(size_t) i] = slots[(size_t) start];
            start = (start + 1) % Capacity;
        }

        return numValid;
    }

    /** UI thread: most recent sample, or a default-constructed one if empty. */
    Sample latest() const noexcept
    {
        if (count.load (std::memory_order_acquire) == 0)
            return {};

        const auto end = writeIndex.load (std::memory_order_acquire);
        return slots[(size_t) ((end - 1 + Capacity) % Capacity)];
    }

private:
    std::array<Sample, Capacity> slots {};
    std::atomic<int> writeIndex { 0 };
    std::atomic<int> count { 0 };
};
