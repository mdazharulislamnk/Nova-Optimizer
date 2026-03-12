#pragma once

#include <atomic>
#include <array>
#include <optional>
#include <cstdint>
#include <type_traits>

namespace nova {
namespace utils {

/**
 * A lock-free, thread-safe ring buffer with single-producer, single-consumer (SPSC) semantics.
 * Designed for low-latency IPC scenarios.
 */
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignas
template <typename T, size_t Capacity>
class LockFreeBuffer {
    static_assert((Capacity != 0) && ((Capacity & (Capacity - 1)) == 0), 
                  "Capacity must be a power of 2 for efficient modulo operations.");
    static_assert(std::is_trivially_copyable_v<T>,
                  "Type must be trivially copyable for raw memory operations if needed.");

public:
    LockFreeBuffer() : head_(0), tail_(0) {}

    // Delete copy and move semantics to prevent unintended data races
    LockFreeBuffer(const LockFreeBuffer&) = delete;
    LockFreeBuffer& operator=(const LockFreeBuffer&) = delete;

    /**
     * Push an item into the buffer (Producer side).
     * Returns true if successful, false if the buffer is full.
     */
    bool push(const T& item) {
        auto current_tail = tail_.load(std::memory_order_relaxed);
        auto next_tail = (current_tail + 1) & mask_;

        // Check if buffer is full
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }

        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    /**
     * Pop an item from the buffer (Consumer side).
     * Returns std::optional<T> which has a value if successful, or std::nullopt if empty.
     */
    std::optional<T> pop() {
        auto current_head = head_.load(std::memory_order_relaxed);
        
        // Check if buffer is empty
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return std::nullopt;
        }

        T item = buffer_[current_head];
        head_.store((current_head + 1) & mask_, std::memory_order_release);
        return item;
    }

    /**
     * Check if the buffer is empty.
     */
    bool is_empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    /**
     * Check if the buffer is full.
     */
    bool is_full() const {
        auto next_tail = (tail_.load(std::memory_order_acquire) + 1) & mask_;
        return next_tail == head_.load(std::memory_order_acquire);
    }

private:
    std::array<T, Capacity> buffer_{};
    static constexpr size_t mask_ = Capacity - 1;

    // Use padding to prevent false sharing between consumer and producer cache lines.
    // Assuming typical 64-byte cache line size (L1_CACHE_BYTES)
    alignas(64) std::atomic<size_t> head_; // Modified by consumer, read by producer
    alignas(64) std::atomic<size_t> tail_; // Modified by producer, read by consumer
};
#pragma warning(pop)

} // namespace utils
} // namespace nova
