#pragma once

#include <cstddef>
#include <mutex>
#include <new>
#include <utility>
#include <vector>

template <typename T>
class MemoryPool {
private:
    struct Block {
        alignas(T) unsigned char data[sizeof(T)];
        Block* next;
    };

    Block*              free_list_{nullptr};
    std::vector<Block*> slabs_;
    size_t              slab_size_;
    mutable std::mutex  mutex_;

    // Called only while mutex_ is held.
    void grow() {
        Block* slab = new Block[slab_size_];
        slabs_.push_back(slab);
        for (size_t i = 0; i < slab_size_; ++i) {
            slab[i].next = free_list_;
            free_list_   = &slab[i];
        }
    }

public:
    explicit MemoryPool(size_t slab_size = 1000)
        : slab_size_(slab_size) {
        std::lock_guard<std::mutex> lock(mutex_);
        grow();
    }

    ~MemoryPool() {
        for (Block* slab : slabs_) {
            delete[] slab;
        }
    }

    MemoryPool(const MemoryPool&)            = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    template <typename... Args>
    T* allocate(Args&&... args) {
        Block* block;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!free_list_) {
                grow();
            }
            block      = free_list_;
            free_list_ = block->next;
        }
        // Placement-new outside the lock: no other thread can touch this block
        // until it is returned to the free list via deallocate().
        return ::new (block->data) T(std::forward<Args>(args)...);
    }

    void deallocate(T* ptr) {
        ptr->~T();
        // Block::data is the first member of Block (offset 0, standard-layout),
        // so the address of the T object equals the address of its enclosing Block.
        Block* block = reinterpret_cast<Block*>(ptr);
        std::lock_guard<std::mutex> lock(mutex_);
        block->next = free_list_;
        free_list_  = block;
    }

    size_t freeCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = 0;
        for (Block* b = free_list_; b; b = b->next) {
            ++count;
        }
        return count;
    }

    size_t totalCapacity() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return slabs_.size() * slab_size_;
    }
};

template <typename T>
struct PoolDeleter {
private:
    MemoryPool<T>* pool_;

public:
    explicit PoolDeleter(MemoryPool<T>* p) : pool_(p) {}
    void operator()(T* ptr) {
        if (pool_) pool_->deallocate(ptr);
    }
};
