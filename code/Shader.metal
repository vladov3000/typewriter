typedef struct {
  float2 resolution;
} Uniforms;

typedef struct {
  float2 position [[attribute(0)]];
  float2 size     [[attribute(1)]];
  float4 color    [[attribute(2)]];
} Vertex;

typedef struct {
  float4 position [[position]];
  float4 color;
} Fragment;

constant float2 offsets[4] = {
  { 1, 1 },
  { 0, 1 },
  { 1, 0 },
  { 0, 0 },
};

vertex Fragment vertex_shader(
  Vertex             v        [[stage_in]],
  unsigned           vid      [[vertex_id]],
  constant Uniforms& uniforms [[buffer(1)]]
) {
  float2 position = v.position + offsets[vid] * v.size;
  // position        = position / float2(1024, 535);
  position        = position / uniforms.resolution;
  position.y      = 1 - position.y;
  position        = 2 * position - 1;
  
  Fragment f;
  f.position = float4(position, 0, 1);
  f.color    = v.color;
  return f;
}

fragment float4 fragment_shader(Fragment f [[stage_in]]) {
  return f.color;
}
