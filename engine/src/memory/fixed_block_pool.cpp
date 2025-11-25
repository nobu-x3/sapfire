#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include "memory/allocators.h"

namespace sf::mem {
    FixedBlockPool::FixedBlockPool(LinearArena* arena, size_t block_size, size_t blocks_per_chunk) :
        arena_(arena), blk_size_(std::max(static_cast<size_t>(sf::align_up(block_size, alignof(std::max_align_t))), sizeof(Node))),
        per_chunk_(blocks_per_chunk), free_(nullptr), chunks_allocated_(0) {
        assert(arena_ != nullptr);
        assert(per_chunk_ > 0);
    }
    void* FixedBlockPool::allocate() {
        if (!free_) {
            add_chunk();
            if (!free_)
                return nullptr;
        }
        Node* n = free_;
        free_ = free_->next;
        return n;
    }

    void FixedBlockPool::deallocate(void* p) noexcept {
        if (!p)
            return;
        Node* n = static_cast<Node*>(p);
        n->next = free_;
        free_ = n;
    }

    void FixedBlockPool::add_chunk() {
        assert(per_chunk_ > 0);
        assert(arena_ != nullptr);
        assert(per_chunk_ > 0);
        assert(blk_size_ >= sizeof(Node)); // each block must hold a freelist node
        assert(blk_size_ <= std::numeric_limits<size_t>::max() / per_chunk_); // no overflow
        const size_t bytes = blk_size_ * per_chunk_;
        void* mem = arena_->allocate(bytes, alignof(std::max_align_t));
        if (!mem)
            return;
        // carve free list
        uint8_t* base = static_cast<uint8_t*>(mem);
        // Preserve the old free‐list head
        Node* old = free_;
        // Point free_ at the start of the newly allocated chunk
        free_ = reinterpret_cast<Node*>(base);
        Node* cur = free_;
        for (size_t i = 1; i < per_chunk_; ++i) {
            Node* nxt = reinterpret_cast<Node*>(base + i * blk_size_);
            cur->next = nxt;
            cur = nxt;
        }
        // Link the tail of the new chunk to the previous free‐list
        cur->next = old;
        ++chunks_allocated_;
    }
} // namespace sf::mem
