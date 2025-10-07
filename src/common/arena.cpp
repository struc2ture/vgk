#include <cstdlib>
#include <cstddef>

#include "arena.hpp"

#include "types.hpp"
#include "util.hpp"

Arena make_arena(size_t capacity)
{
    Arena arena = {};
    arena.base = (u8 *)xmalloc(capacity);
    arena.capacity = capacity;
    return arena;
}

void destroy_arena(Arena *arena)
{
    free(arena->base);
    *arena = (Arena){};
}

static inline size_t align_up(size_t ptr, size_t align)
{
    return (ptr + (align - 1)) & ~(align - 1);
}

void *arena_alloc_raw(Arena *arena, size_t size, size_t align)
{
    size_t current = (size_t)(arena->base + arena->offset);
    size_t aligned = align_up(current, align);
    size_t new_offset = (aligned - (size_t)arena->base) + size;
    bassert(new_offset <= arena->capacity);
    arena->offset = new_offset;
    void *data_ptr = (void *)aligned;
    return data_ptr;
}

void *arena_alloc_zero_raw(Arena *arena, size_t size, size_t align)
{
    size_t current = (size_t)(arena->base + arena->offset);
    size_t aligned = align_up(current, align);
    size_t new_offset = (aligned - (size_t)arena->base) + size;
    bassert(new_offset <= arena->capacity);
    arena->offset = new_offset;
    void *data_ptr = (void *)aligned;
    memset(data_ptr, 0, size);
    return data_ptr;
}

void arena_reset(Arena *arena)
{
    arena->offset = 0;
}
