#include "memory/allocators.h"
#include <cstdint>
#include <new>

namespace sf::mem {

LinearArena::LinearArena() : begin_(nullptr), cur_(nullptr), end_(nullptr), used_peak_(0), alloc_calls_(0) {}

LinearArena::LinearArena(void *mem, size_t bytes) { reset_to(mem, bytes); }

void LinearArena::reset_to(void *mem, size_t bytes) {
	begin_ = static_cast<uint8_t *>(mem);
	cur_ = begin_;
	end_ = begin_ + bytes;
	used_peak_ = 0;
	alloc_calls_ = 0;
}

void *LinearArena::allocate(size_t bytes, size_t align) {
	if (bytes == 0)
		return nullptr;

	size_t cur_addr = reinterpret_cast<size_t>(cur_);
	if (cur_addr > SIZE_MAX - align) {
		throw std::bad_alloc();
	}

	// Align the current pointer up to the requested boundary
	uint8_t *p = reinterpret_cast<uint8_t *>(sf::align_up(cur_addr, align));

	if (p > end_ || p + bytes > end_ || p + bytes < p)
		throw std::bad_alloc();

	cur_ = p + bytes;
	size_t used_now = static_cast<size_t>(cur_ - begin_);
	if (used_now > used_peak_)
		used_peak_ = used_now;
	++alloc_calls_;
	return p;
}

} // namespace sf::mem
