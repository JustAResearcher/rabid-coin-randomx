#ifndef WIN32
#include <cstddef>
#include <sys/mman.h>
#include <stdexcept>

extern "C" void freePagedMemory_c_impl(void* ptr, size_t bytes);

void* allocExecutableMemory(size_t bytes, bool hugePages) {
    void* p = mmap(nullptr, bytes,
                   PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) throw std::runtime_error("Failed to allocate executable memory");
    return p;
}

// C++ mangled version (used by jit_compiler_x86 and allocator via virtual_memory.h)
void freePagedMemory(void* ptr, size_t bytes) {
    freePagedMemory_c_impl(ptr, bytes);
}

#endif // WIN32
