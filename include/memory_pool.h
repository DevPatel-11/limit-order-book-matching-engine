#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include "order.h"
#include <vector>
#include <memory>
#include <mutex>

template<typename T>
class MemoryPool {
private:
    struct Block {
        alignas(T) unsigned char data[sizeof(T)];
        Block* next;
    };
    
    Block* free_list;
    std::vector<Block*> allocated_blocks;
    size_t block_size;
    std::mutex pool_mutex;
    
    void allocateBlock() {
        Block* new_block = new Block[block_size];
        allocated_blocks.push_back(new_block);
        
        // Add all blocks to free list
        for (size_t i = 0; i < block_size - 1; ++i) {
            new_block[i].next = &new_block[i + 1];
        }
        new_block[block_size - 1].next = free_list;
        free_list = new_block;
    }
    
public:
    explicit MemoryPool(size_t initial_size = 1000) 
        : free_list(nullptr), block_size(initial_size) {
        allocateBlock();
    }
    
    ~MemoryPool() {
        for (auto block : allocated_blocks) {
            delete[] block;
        }
    }
    
    // Prevent copying
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    
    template<typename... Args>
    T* allocate(Args&&... args) {
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        if (!free_list) {
            allocateBlock();
        }
        
        Block* block = free_list;
        free_list = block->next;
        
        // Construct object in-place
        return new (block->data) T(std::forward<Args>(args)...);
    }
    
    void deallocate(T* ptr) {
        if (!ptr) return;
        
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        // Call destructor
        ptr->~T();
        
        // Add back to free list
        Block* block = reinterpret_cast<Block*>(ptr);
        block->next = free_list;
        free_list = block;
    }
    
    size_t getFreeCount() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(pool_mutex));
        size_t count = 0;
        Block* current = free_list;
        while (current) {
            count++;
            current = current->next;
        }
        return count;
    }
    
    size_t getTotalCapacity() const {
        return allocated_blocks.size() * block_size;
    }
};

// Custom deleter for shared_ptr to work with memory pool
template<typename T>
class PoolDeleter {
private:
    MemoryPool<T>* pool;
    
public:
    explicit PoolDeleter(MemoryPool<T>* p) : pool(p) {}
    
    void operator()(T* ptr) {
        if (pool) {
            pool->deallocate(ptr);
        }
    }
};

#endif
