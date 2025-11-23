#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <type_traits>
#include "core/platform.h"

namespace sf {

inline constexpr size_t align_up(size_t x, size_t a) {
	assert((a & (a - 1)) == 0 && "Alignment must be a power of two");
	const size_t mask = a - 1;
	return (x + mask) & ~mask;
}

namespace mem {

class LinearArena {
  public:
	LinearArena();
	LinearArena(void *mem, size_t bytes);

	void reset_to(void *mem, size_t bytes);

	void *allocate(size_t bytes, size_t align);

	// bump arenas don't free individual blocks
	inline void deallocate(void *, size_t, size_t) noexcept {}
	inline void reset() noexcept { cur_ = begin_; } // does not call destructors
	inline size_t capacity() const noexcept { return static_cast<size_t>(end_ - begin_); }
	inline size_t used() const noexcept { return static_cast<size_t>(cur_ - begin_); }
	inline size_t peak() const noexcept { return used_peak_; }
	inline size_t alloc_calls() const noexcept { return alloc_calls_; }

  private:
	uint8_t *begin_;
	uint8_t *cur_;
	uint8_t *end_;
	size_t used_peak_;
	size_t alloc_calls_;
};

// ---------- Fixed-block pool backed by a LinearArena ----------
// For node-heavy containers (list/map/unordered_map), supports deallocate.
// Each container instance gets its own pool; freed nodes return to the pool.
class FixedBlockPool {
  public:
	FixedBlockPool() = default;
	FixedBlockPool(LinearArena *arena, std::size_t block_size, std::size_t blocks_per_chunk = 256);

	void *allocate();
	void deallocate(void *p) noexcept;

	inline size_t block_size() const noexcept { return blk_size_; }
	inline size_t chunks() const noexcept { return chunks_allocated_; }

  private:
	void add_chunk();

  private:
	struct Node {
		Node *next;
	};

	LinearArena *arena_{nullptr};
	size_t blk_size_{0};
	size_t per_chunk_{0};
	Node *free_{nullptr};
	size_t chunks_allocated_{0};
};

// ---------- STL allocator: Linear / bump ----------
template <class T> class LinearTaggedAllocator {
  public:
	using value_type = T;
	using propagate_on_container_copy_assignment = std::true_type;
	using propagate_on_container_move_assignment = std::true_type;
	using propagate_on_container_swap = std::true_type;

	LinearTaggedAllocator() noexcept : arena_(nullptr) {}
	explicit LinearTaggedAllocator(LinearArena *a) noexcept : arena_(a) {}

	template <class U> LinearTaggedAllocator(const LinearTaggedAllocator<U> &other) noexcept : arena_(other.arena_) {}

	T *allocate(std::size_t n) {
		assert(arena_ && "LinearTaggedAllocator requires a valid LinearArena");
		if (arena_ == nullptr) [[unlikely]] {
			throw std::bad_alloc();
		}
		if (n > (std::numeric_limits<std::size_t>::max() / sizeof(T))) {
			throw std::bad_alloc();
		}
		void *p = arena_->allocate(n * sizeof(T), alignof(T));
		if (p == nullptr) [[unlikely]] {
			throw std::bad_alloc();
		}
		return static_cast<T *>(p);
	}

	void deallocate(T *, std::size_t) noexcept {
		// no-op; reclaimed when arena is reset/destroyed
	}

	template <class U> struct rebind {
		using other = LinearTaggedAllocator<U>;
	};
	template <class U> friend class LinearTaggedAllocator;

	LinearArena *arena() const noexcept { return arena_; }

	bool operator==(const LinearTaggedAllocator &rhs) const noexcept { return arena_ == rhs.arena_; }
	bool operator!=(const LinearTaggedAllocator &rhs) const noexcept { return !(*this == rhs); }

  private:
	LinearArena *arena_;
};

// ---------- STL allocator: Fixed-block pool (supports deallocate) ----------
template <class T> class PoolTaggedAllocator {
  public:
	using value_type = T;
	using propagate_on_container_copy_assignment = std::true_type;
	using propagate_on_container_move_assignment = std::true_type;
	using propagate_on_container_swap = std::true_type;

	PoolTaggedAllocator() noexcept : arena_(nullptr), pool_(nullptr) {}
	explicit PoolTaggedAllocator(LinearArena *a) : arena_(a), pool_(nullptr) {}

	template <class U> PoolTaggedAllocator(const PoolTaggedAllocator<U> &other) noexcept : arena_(other.arena_), pool_(nullptr) {}

	T *allocate(std::size_t n) {
		if (n == 1) {
			ensure_pool_for(sizeof(T), alignof(T));
			void *p = pool_->allocate();
			return static_cast<T *>(p);
		} else {
			// fallback for multi-object allocations
			void *p = arena_->allocate(n * sizeof(T), alignof(T));
			if (!p)
				throw std::bad_alloc();
			return static_cast<T *>(p);
		}
	}

	void deallocate(T *p, std::size_t n) noexcept {
		if (p == nullptr || n == 0) {
			return;
		}
		if (n == 1 && pool_) {
			pool_->deallocate(p);
		} else if (arena_) {
			arena_->deallocate(p, n * sizeof(T), alignof(T));
		} else {
			assert(false && "PoolTaggedAllocator::deallocate: arena_ is null");
		}
	}

	template <class U> struct rebind {
		using other = PoolTaggedAllocator<U>;
	};
	template <class U> friend class PoolTaggedAllocator;

	// expose for debugging
	FixedBlockPool *pool() const noexcept { return pool_; }
	LinearArena *arena() const noexcept { return arena_; }

	bool operator==(const PoolTaggedAllocator &rhs) const noexcept { return arena_ == rhs.arena_ && pool_ == rhs.pool_; }
	bool operator!=(const PoolTaggedAllocator &rhs) const noexcept { return !(*this == rhs); }

  private:
	void ensure_pool_for(std::size_t sz, std::size_t /*align*/) {
		if (pool_)
			return;
		// allocate the FixedBlockPool object itself from the arena (keeps it inside budget)
		void *mem = arena_->allocate(sizeof(FixedBlockPool), alignof(FixedBlockPool));
		if (mem == nullptr) [[unlikely]] {
			throw std::bad_alloc();
		}
		pool_ = new (mem) FixedBlockPool(arena_, sz);
	}

	LinearArena *arena_;
	FixedBlockPool *pool_;
};

} // namespace mem
} // namespace sf
