typedef struct {
  F32 resolution[2];
} Uniforms;

typedef struct {
  F32 position[2];
  F32 size[2];
  F32 color[4];
} Sprite;

typedef struct {
  Semaphore     semaphore;
  NSView*       view;
  CAMetalLayer* layer;
  PipelineState pipeline_state;
  CommandQueue  command_queue;
  id<MTLBuffer> uniform_buffer;
  Uniforms*     uniforms;
  id<MTLBuffer> sprite_buffer;
  I64           sprite_limit;
  I64           sprite_count;
  Sprite*       sprites;
} Renderer;

static NSWindow* make_window() {
  NSRect rectangle = [[NSScreen mainScreen] frame];

  NSUInteger style
    = NSWindowStyleMaskTitled
    | NSWindowStyleMaskClosable
    | NSWindowStyleMaskResizable
    | NSWindowStyleMaskMiniaturizable;

  NSWindow* window;
  window = [NSWindow alloc];
  window = [window initWithContentRect:rectangle styleMask:style backing:NSBackingStoreBuffered defer:NO];
  [window setTitle:@"Typewriter"];
  [window makeKeyAndOrderFront:nil];
  return window;
}

static CAMetalLayer* make_layer(id<MTLDevice> device, NSWindow* window) {
  CAMetalLayer* layer = [CAMetalLayer layer];
  layer.device        = device;
  layer.pixelFormat   = PIXEL_FORMAT;
  layer.contentsScale = window.backingScaleFactor;
  return layer;
}

static NSView* make_view(NSWindow* window, CAMetalLayer* layer) {
  NSView* view       = [NSView new];
  view.wantsLayer    = YES;
  view.layer         = layer;
  window.contentView = view;
  return view;
}

static void add_attribute(Attribute* attribute, MTLVertexFormat format, NSUInteger offset) {
  attribute.format      = format;
  attribute.offset      = offset;
  attribute.bufferIndex = 0;
}

static void add_attributes(AttributeArray* attributes) {
  add_attribute(attributes[0], MTLVertexFormatFloat2, offsetof(Sprite, position));
  add_attribute(attributes[1], MTLVertexFormatFloat2, offsetof(Sprite, size));
  add_attribute(attributes[2], MTLVertexFormatFloat4, offsetof(Sprite, color));
}

static void add_layout(BufferLayout* layout) {
  layout.stepFunction = MTLVertexStepFunctionPerInstance;
  layout.stepRate     = 1;
  layout.stride       = sizeof(Sprite);
}

static MTLVertexDescriptor* make_vertex() {
  MTLVertexDescriptor* vertex = [MTLVertexDescriptor new];
  add_attributes(vertex.attributes);
  add_layout(vertex.layouts[0]);
  return vertex;
}

static PipelineState make_pipeline_state(id<MTLDevice> device, id<MTLLibrary> library) {
  MTLRenderPipelineDescriptor* pipeline    = [MTLRenderPipelineDescriptor new];
  pipeline.vertexFunction                  = [library newFunctionWithName:@"vertex_shader"];
  pipeline.fragmentFunction                = [library newFunctionWithName:@"fragment_shader"];
  pipeline.vertexDescriptor                = make_vertex();
  pipeline.colorAttachments[0].pixelFormat = PIXEL_FORMAT;

  NSError*      error          = nil;
  PipelineState pipeline_state = [device newRenderPipelineStateWithDescriptor:pipeline error:&error];
  if (error != nil) {
    NSLog(@"%@", error);
  }
  return pipeline_state;
}

static Renderer* make_renderer(Arena* arena, App* app, int sprite_limit) {
  NSWindow*      window  = make_window();
  id<MTLDevice>  device  = MTLCreateSystemDefaultDevice();
  id<MTLLibrary> library = [device newDefaultLibrary];
  CAMetalLayer*  layer   = make_layer(device, window);
  NSView*        view    = make_view(window, layer);

  Renderer* renderer      = arena_allocate(arena, Renderer);
  renderer->semaphore      = dispatch_semaphore_create(1);
  renderer->view           = view;
  renderer->layer          = layer;
  renderer->pipeline_state = make_pipeline_state(device, library);
  renderer->command_queue  = [device newCommandQueue];
  renderer->uniform_buffer = [device newBufferWithLength:sizeof(Uniforms) options:0];
  renderer->uniforms       = [renderer->uniform_buffer contents];
  renderer->sprite_limit   = sprite_limit;
  renderer->sprite_count   = 0;
  renderer->sprite_buffer  = [device newBufferWithLength:sizeof(Sprite) * sprite_limit options:0];
  renderer->sprites        = [renderer->sprite_buffer contents];
  return renderer;
}

static void draw_start(Renderer* renderer) {
  dispatch_semaphore_wait(renderer->semaphore, DISPATCH_TIME_FOREVER);
}

static void draw_sprite(Renderer* renderer, Sprite sprite) {
  I64 sprite_limit = renderer->sprite_limit;
  I64 sprite_count = renderer->sprite_count;
  if (sprite_count < sprite_limit) {
    renderer->sprites[sprite_count] = sprite;
    renderer->sprite_count++;
  }
}

static RenderPass* make_render_pass(id<MTLTexture> texture) {
  RenderPass*      pass       = [MTLRenderPassDescriptor renderPassDescriptor];
  ColorAttachment* attachment = pass.colorAttachments[0];
  attachment.texture          = texture;
  attachment.loadAction       = MTLLoadActionClear;
  attachment.storeAction      = MTLStoreActionStore;
  attachment.clearColor       = MTLClearColorMake(0, 0, 0, 1);
  return pass;
}

static void draw_end(Renderer* renderer) {
  NSView*       view           = renderer->view;
  CAMetalLayer* layer          = renderer->layer;
  PipelineState pipeline_state = renderer->pipeline_state;
  CommandQueue  command_queue  = renderer->command_queue;
  id<MTLBuffer> uniform_buffer = renderer->uniform_buffer;
  Uniforms*     uniforms       = renderer->uniforms;
  id<MTLBuffer> sprite_buffer  = renderer->sprite_buffer;
  I64           sprite_count   = renderer->sprite_count;

  uniforms->resolution[0] = view.frame.size.width;
  uniforms->resolution[1] = view.frame.size.height;
  
  id<CAMetalDrawable> drawable = [layer nextDrawable];
  if (drawable == nil) {
    return;
  }
  
  RenderPass*    pass           = make_render_pass(drawable.texture);
  CommandBuffer  command_buffer = [command_queue commandBuffer];
  CommandEncoder encoder        = [command_buffer renderCommandEncoderWithDescriptor:pass];
  [encoder setRenderPipelineState:pipeline_state];
  if (sprite_count > 0) {
    [encoder setVertexBuffer:sprite_buffer  offset:0 atIndex:0];
    [encoder setVertexBuffer:uniform_buffer offset:0 atIndex:1];
    [encoder drawPrimitives:TRIANGLE_STRIP vertexStart:0 vertexCount:4 instanceCount:sprite_count];
  }
  [encoder endEncoding];
  [command_buffer presentDrawable:drawable];
  [command_buffer addCompletedHandler:^(CommandBuffer command_buffer) {
      dispatch_semaphore_signal(renderer->semaphore);
    }];
  [command_buffer commit];
  [layer display];

  renderer->sprite_count = 0;
}
