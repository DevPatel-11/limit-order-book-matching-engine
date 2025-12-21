#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include <atomic>
#include <cstdlib>
#include <memory>

template <typename T>
class ConcurrentQueue {
private:
    struct Node {
        T value;
        std::atomic<Node*> next{nullptr};
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;

public:
    ConcurrentQueue() {
        Node* dummy = new Node();
        head.store(dummy, std::memory_order_release);
        tail.store(dummy, std::memory_order_release);
    }

    ~ConcurrentQueue() {
        Node* curr = head.load();
        while (curr) {
            Node* next = curr->next.load();
            delete curr;
            curr = next;
        }
    }

    void enqueue(const T& val) {
        Node* new_node = new Node();
        new_node->value = val;
        Node* old_tail = tail.load(std::memory_order_acquire);
        while (true) {
            Node* next = old_tail->next.load();
            if (next == nullptr) {
                if (old_tail->next.compare_exchange_weak(next, new_node)) {
                    tail.compare_exchange_weak(old_tail, new_node);
                    return;
                }
            } else {
                tail.compare_exchange_weak(old_tail, next);
                old_tail = tail.load(std::memory_order_acquire);
            }
        }
    }

    bool dequeue(T& result) {
        while (true) {
            Node* old_head = head.load(std::memory_order_acquire);
            Node* next = old_head->next.load();
            if (next == nullptr) return false;
            if (head.compare_exchange_weak(old_head, next)) {
                result = next->value;
                delete old_head;
                return true;
            }
        }
    }

    bool try_dequeue(T& result) {
        Node* old_head = head.load(std::memory_order_acquire);
        Node* next = old_head->next.load();
        if (next == nullptr) return false;
        if (head.compare_exchange_weak(old_head, next)) {
            result = next->value;
            delete old_head;
            return true;
        }
        return false;
    }
};

#endif
