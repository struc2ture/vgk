#pragma once

#include <cstddef>

#include "types.hpp"

struct Arena
{
    u8 *base;
    size_t capacity;
    size_t offset;
};

Arena make_arena(size_t capacity);
void destroy_arena(Arena *arena);
void *arena_alloc_raw(Arena *arena, size_t size, size_t align);
void *arena_alloc_zero_raw(Arena *arena, size_t size, size_t align);
void arena_reset(Arena* arena);

#define arena_alloc(arena, Type) \
    (Type *)arena_alloc_raw((arena), sizeof(Type), alignof(Type))

#define arena_alloc_array(arena, Type, Count) \
    (Typ e*)arena_alloc_raw((arena), (Count) * sizeof(Type), alignof(Type))

#define arena_alloc_zero(arena, Type) \
    (Type *)arena_alloc_zero_raw((arena), sizeof(Type), alignof(Type))

#define arena_alloc_array_zero(arena, Type, Count) \
    (Type *)arena_alloc_zero_raw((arena), (Count) * sizeof(Type), alignof(Type))
