#pragma once

#include <atomic>
#include <stdexcept>
#include <utility>

// ---------------------------------------------------------------------------
// ConcurrentQueue<T>
//
// Lock-free, multi-producer / multi-consumer queue based on the Michael-Scott
// algorithm (M. M. Michael and M. L. Scott, "Simple, Fast, and Practical
// Non-Blocking and Blocking Concurrent Queue Algorithms", PODC 1996).
//
// Guarantees:
//   - Linearisable enqueue and dequeue operations.
//   - Progress: lock-free (every operation completes in a finite number of
//     steps system-wide even if individual threads are delayed).
//   - Memory order: acquire/release pairs that are sufficient for x86 and
//     correctly synchronise on weakly-ordered architectures.
//
// Limitations:
//   - No ABA protection beyond what std::atomic CAS provides on the target
//     platform. For production use on platforms with narrow pointers a
//     tagged-pointer scheme should be added.
//   - T must be copyable or movable.
// ---------------------------------------------------------------------------
template <typename T>
class ConcurrentQueue {
private:
    struct Node {
        T                  value{};
        std::atomic<Node*> next{nullptr};

        Node() = default;
        explicit Node(T v) : value(std::move(v)) {}
    };

    // Each pointer lives on its own 64-byte cache line so that producer and
    // consumer threads do not thrash each other's cache lines (false sharing).
    alignas(64) std::atomic<Node*> head_;
    char pad_[64 - sizeof(std::atomic<Node*>)];   // padding to full cache line
    alignas(64) std::atomic<Node*> tail_;

public:
    ConcurrentQueue() {
        // The queue always contains one sentinel (dummy) node so that head_
        // and tail_ are never null and dequeue never races with enqueue on an
        // empty structure.
        Node* sentinel = new Node();
        head_.store(sentinel, std::memory_order_relaxed);
        tail_.store(sentinel, std::memory_order_relaxed);
    }

    ~ConcurrentQueue() {
        // Drain any remaining nodes (including the sentinel).
        Node* cur = head_.load(std::memory_order_relaxed);
        while (cur != nullptr) {
            Node* next = cur->next.load(std::memory_order_relaxed);
            delete cur;
            cur = next;
        }
    }

    // Non-copyable, non-assignable.
    ConcurrentQueue(const ConcurrentQueue&)            = delete;
    ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

    // -----------------------------------------------------------------------
    // enqueue
    //
    // Appends val to the tail of the queue. Always succeeds.
    // -----------------------------------------------------------------------
    void enqueue(T val) {
        Node* node = new Node(std::move(val));

        while (true) {
            Node* tail = tail_.load(std::memory_order_acquire);
            Node* next = tail->next.load(std::memory_order_acquire);

            // Re-read tail_ — it may have changed since we loaded it.
            if (tail != tail_.load(std::memory_order_acquire)) continue;

            if (next == nullptr) {
                // Tail is pointing at the last node: try to link our new node.
                Node* expected = nullptr;
                if (tail->next.compare_exchange_weak(
                        expected, node,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {
                    // Linked successfully. Advance tail_ (best-effort; if we
                    // fail, the next enqueue/dequeue will fix it).
                    tail_.compare_exchange_weak(
                        tail, node,
                        std::memory_order_release,
                        std::memory_order_relaxed);
                    return;
                }
            } else {
                // Tail is lagging behind; advance it and retry.
                tail_.compare_exchange_weak(
                    tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            }
        }
    }

    // -----------------------------------------------------------------------
    // dequeue
    //
    // Removes the front element and stores it in result.
    // Returns true on success, false if the queue was empty.
    // -----------------------------------------------------------------------
    bool dequeue(T& result) {
        while (true) {
            Node* head = head_.load(std::memory_order_acquire);
            Node* tail = tail_.load(std::memory_order_acquire);
            Node* next = head->next.load(std::memory_order_acquire);

            // Re-read head_ to check for concurrent modification.
            if (head != head_.load(std::memory_order_acquire)) continue;

            if (head == tail) {
                // Queue appears empty or tail is lagging.
                if (next == nullptr) {
                    // Truly empty.
                    return false;
                }
                // Tail is behind; advance it to help other threads.
                tail_.compare_exchange_weak(
                    tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            } else {
                // Read the value before CAS so we don't access freed memory.
                T value = next->value;

                // Try to swing head_ forward past the sentinel.
                if (head_.compare_exchange_weak(
                        head, next,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {
                    result = std::move(value);
                    delete head; // old sentinel is now unreachable
                    return true;
                }
                // CAS failed — another thread dequeued; retry.
            }
        }
    }
};
