#pragma once
#include <cstdlib>
#include <new>
namespace audio_allocation_probe {
inline thread_local bool enabled = false;
inline thread_local size_t count = 0;
inline void begin() {
    count = 0;
    enabled = true;
}
inline size_t end() {
    enabled = false;
    return count;
}
} // namespace audio_allocation_probe
void *operator new(std::size_t n) {
    if (audio_allocation_probe::enabled)
        ++audio_allocation_probe::count;
    if (auto *p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept {
    std::free(p);
}
void operator delete(void *p, std::size_t) noexcept {
    std::free(p);
}
void *operator new[](std::size_t n) {
    return ::operator new(n);
}
void operator delete[](void *p) noexcept {
    ::operator delete(p);
}
void operator delete[](void *p, std::size_t) noexcept {
    ::operator delete(p);
}
