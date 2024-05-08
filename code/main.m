#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <Quartz/Quartz.h>

#define ARENA_PAGE_SIZE 4096
#define ESCAPE          53
#define PIXEL_FORMAT    MTLPixelFormatBGRA8Unorm
#define TRIANGLE_STRIP  MTLPrimitiveTypeTriangleStrip

typedef NSApplication                          App;
typedef MTLVertexAttributeDescriptor           Attribute;
typedef MTLVertexAttributeDescriptorArray      AttributeArray;
typedef MTLVertexBufferLayoutDescriptor        BufferLayout;
typedef id<MTLRenderPipelineState>             PipelineState;
typedef id<MTLCommandQueue>                    CommandQueue;
typedef id<MTLCommandBuffer>                   CommandBuffer;
typedef id<MTLRenderCommandEncoder>            CommandEncoder;
typedef MTLRenderPassDescriptor                RenderPass;
typedef MTLRenderPassColorAttachmentDescriptor ColorAttachment;
typedef dispatch_semaphore_t                   Semaphore;

typedef unsigned char U8;
typedef unsigned long U64;
typedef int           I32;
typedef long          I64;
typedef float         F32;

#include "arena.h"
#include "renderer.h"

static App* make_app() {
  App* app = [NSApplication sharedApplication];
  [app setActivationPolicy:NSApplicationActivationPolicyRegular];
  [app activateIgnoringOtherApps:YES];
  [app finishLaunching];
  return app;
}

static NSEvent* get_event(NSApplication* app) {
  return [app nextEventMatchingMask:NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
}

static void handle_input(NSApplication* app) {
  while (true) {
    NSEvent* event = get_event(app);
    if (event == nil) {
      break;
    }
    if (event.type == NSEventTypeKeyDown && event.keyCode == ESCAPE) {
      exit(EXIT_SUCCESS);
    }
    [app sendEvent:event];
    [app updateWindows];
  }
}

int main() {
  Arena arena;
  arena_initialize(&arena, 1ull << 32);

  App*      app      = make_app();
  Renderer* renderer = make_renderer(&arena, app, 1024);

  while (true) {
    @autoreleasepool {
      handle_input(app);
      draw_start(renderer);
      draw_sprite(renderer, (Sprite) {   0,   0, 100, 100, 1, 0, 0, 0 });
      draw_sprite(renderer, (Sprite) { 100,   0, 100, 100, 0, 1, 0, 0 });
      draw_sprite(renderer, (Sprite) {   0, 100, 100, 100, 0, 0, 1, 0 });
      draw_sprite(renderer, (Sprite) { 100, 100, 100, 100, 1, 1, 0, 0 });
      draw_end(renderer);
    }
  }
}
