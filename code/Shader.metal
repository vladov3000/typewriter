#include <metal_stdlib>

using namespace metal;

typedef struct {
  float2 resolution;
  float  scale;
} Uniforms;

typedef struct {
  float2 position [[attribute(0)]];
  float2 size     [[attribute(1)]];
  float4 color    [[attribute(2)]];
  float  tint     [[attribute(3)]];
  float2 uv       [[attribute(4)]];
  float2 uv_size  [[attribute(5)]];
} Vertex;

typedef struct {
  float4 position [[position]];
  float4 color;
  float  tint;
  float2 uv;
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
  position        = position / uniforms.resolution * uniforms.scale;
  position.y      = 1 - position.y;
  position        = 2 * position - 1;

  float2 uv = v.uv + offsets[vid] * v.uv_size;

  Fragment f;
  f.position = float4(position, 0, 1);
  f.color    = v.color;
  f.tint     = v.tint;
  f.uv       = uv;
  return f;
}

fragment float4 fragment_shader(
  Fragment         f       [[stage_in]],
  texture2d<float> texture [[texture(0)]]
) {
  constexpr sampler s(coord::normalized, address::repeat, filter::linear);
  // float4 sprite = float4(f.uv.x, 0, f.uv.y, 1);
  float4 sprite = texture.sample(s, f.uv);
  float4 color  = f.color;
  return f.tint * color + (1 - f.tint) * sprite;
}
