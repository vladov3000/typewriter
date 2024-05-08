typedef struct {
  U8* memory;
  U64 size;
  U64 used;
  U64 committed;
} Arena;

static void arena_initialize(Arena* arena, U64 size) {
  U8* memory = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(memory != MAP_FAILED);

  arena->memory	   = memory;
  arena->size	   = size;
  arena->used	   = 0;
  arena->committed = 0;
}

static U64 align(U64 offset, U64 alignment) {
  return (offset + alignment - 1) & (alignment - 1);
}

static U8* arena_allocate_bytes(Arena* arena, U64 bytes_size, U64 bytes_alignment) {
  U8* memory    = arena->memory;
  U64 size      = arena->size;
  U64 used      = arena->used;
  U64 committed = arena->committed;

  U64 aligned_size = align(size, bytes_alignment);
  U64 new_size     = aligned_size + bytes_size;
  U8* bytes        = &memory[aligned_size];
  assert(new_size <= size);

  if (new_size > committed) {
    U64 difference = align(new_size - committed, 4096);
    I32 result     = mprotect(&memory[committed], difference, PROT_READ | PROT_WRITE);
    assert(result == 0);
  }
  
  return bytes;
}

#define arena_allocate(arena, type) \
  ((type*) arena_allocate_bytes((arena), sizeof(type), _Alignof(type)))