typedef struct {
  U8* data;
  I64 capacity;
  I64 cursor;
  I64 back;
} GapBuffer;

static GapBuffer* make_gap_buffer(Arena* arena, I64 capacity) {
  GapBuffer* buffer = arena_allocate_zeroed(arena, GapBuffer);
  buffer->data      = arena_allocate_bytes(arena, capacity, 1);
  buffer->back      = capacity;
  buffer->capacity  = capacity;
  return buffer;
}

static void add_character(GapBuffer* buffer, U8 c) {
  if (buffer->cursor <= buffer->back) {
    buffer->data[buffer->cursor] = c;
    buffer->cursor++;
  }
}

static void delete_character(GapBuffer* buffer) {
  if (buffer->cursor > 0) {
    buffer->cursor--;
  }
}

static void move_left(GapBuffer* buffer) {
  if (buffer->cursor > 0) {
    buffer->back--;
    buffer->cursor--;
    buffer->data[buffer->back] = buffer->data[buffer->cursor];
  }
}

static void move_right(GapBuffer* buffer) {
  if (buffer->back < buffer->capacity) {
    buffer->back++;
    buffer->cursor++;
    buffer->data[buffer->cursor] = buffer->data[buffer->back];
  }
}

static void move_up(GapBuffer* buffer) {
  I64 column = 0;
  while (buffer->cursor > 0 && buffer->data[buffer->cursor] != '\n') {
    column++;
    move_left(buffer);
  }
  move_left(buffer);
  while (buffer->cursor > 0 && buffer->data[buffer->cursor] != '\n') {
    move_left(buffer);
  }
  for (I64 i = 0; i < column - 1; i++) {
    move_right(buffer);
  }
}

static void move_down(GapBuffer* buffer) {
  I64 column = 0;
  while (buffer->cursor > 0 && buffer->data[buffer->cursor] != '\n') {
    column++;
    move_left(buffer);
  }
  move_right(buffer);
  while (buffer->back < buffer->capacity && buffer->data[buffer->cursor] != '\n') {
    move_right(buffer);
  }
  for (I64 i = 0; i < column + 1; i++) {
    move_right(buffer);
  }
}

static void move_to_line_start(GapBuffer* buffer) {
  while (buffer->cursor > 0 && buffer->data[buffer->cursor] != '\n') {
    move_left(buffer);
  }
  if (buffer->data[buffer->cursor] == '\n') {
    move_right(buffer);
  }
}

static void move_to_line_end(GapBuffer* buffer) {
  while (buffer->back < buffer->capacity && buffer->data[buffer->cursor] != '\n') {
    move_right(buffer);
  }
}

static void draw_character(
  Arena*     arena,
  GapBuffer* buffer,
  Renderer*  renderer,
  F32*       offset,
  U8         c
) {
  if (c == '\n') {
    offset[0]  = 0;
    offset[1] += 31.5;
  } else {
    Sprite text = draw_letter(arena, renderer->atlas, c);
    text.position[0] = offset[0];
    text.position[1] = offset[1];
    offset[0] += text.size[0];
    draw_sprite(renderer, text);
  }
}

static void draw_cursor(Renderer* renderer, F32* offset) {
  draw_sprite(renderer, (Sprite) { offset[0], offset[1], 2, 31.5, 1, 1, 1, 1, 1 });
  offset[0] += 2;
}

static void draw_buffer(Arena* arena, GapBuffer* buffer, Renderer* renderer) {
  F32 offset[2] = {};
  for (I64 i = 0; i < buffer->cursor; i++) {
    draw_character(arena, buffer, renderer, offset, buffer->data[i]);
  }
  F32 cursor_offset[2] = { offset[0], offset[1] };
  for (I64 i = buffer->back; i < buffer->capacity; i++) {
    draw_character(arena, buffer, renderer, offset, buffer->data[i]);
  }
  draw_cursor(renderer, cursor_offset);
}
