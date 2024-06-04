#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <Quartz/Quartz.h>

#define ARENA_PAGE_SIZE 4096
#define PIXEL_FORMAT    MTLPixelFormatBGRA8Unorm
#define TRIANGLE_STRIP  MTLPrimitiveTypeTriangleStrip
#define ESCAPE          53
#define BACKSPACE       51
#define NEWLINE         36
#define LEFT_ARROW      123
#define RIGHT_ARROW     124
#define DOWN_ARROW      125
#define UP_ARROW        126

#define length(array) (sizeof(array) / sizeof((array)[0]))
#define string(s) (String) { .data = (U8*) (s), .size = strlen(s) }
#define zero_array(array) memset((a), 0, sizeof(a))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

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

typedef struct {
  U8* data;
  I64 size;
} String;

typedef struct {
  F32 position[2];
  F32 size[2];
} Rectangle;

typedef struct {
  F32 rgba[4];
} Color;

typedef struct {
  F32 position[2];
  F32 size[2];
  F32 color[4];
  F32 tint;
  F32 uv[2];
  F32 uv_size[2];
} Sprite;

#include "arena.h"
#include "atlas.h"
#include "renderer.h"
#include "gap_buffer.h"

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

static void handle_input(NSApplication* app, GapBuffer* gap_buffer, Renderer* renderer) {
  while (true) {
    NSEvent* event = get_event(app);
    if (event == nil) {
      break;
    }
    if (event.type == NSEventTypeKeyDown) {
      NSString*  characters    = [event charactersIgnoringModifiers];
      unichar    character     = [characters characterAtIndex:0];
      NSUInteger mask          = NSEventModifierFlagDeviceIndependentFlagsMask;
      NSUInteger flags         = [event modifierFlags] & mask;
      NSUInteger command_shift = NSEventModifierFlagCommand | NSEventModifierFlagShift;
      if (character == NSDeleteCharacter) {
	delete_character(gap_buffer);
      } else if (character == NSCarriageReturnCharacter) {
	add_character(gap_buffer, '\n');
      } else if (character == NSLeftArrowFunctionKey) {
	move_left(gap_buffer);
      } else if (character == NSRightArrowFunctionKey) {
	move_right(gap_buffer);
      } else if (character == NSUpArrowFunctionKey) {
	move_up(gap_buffer);
      } else if (character == NSDownArrowFunctionKey) {
	move_down(gap_buffer);
      } else if (flags == NSEventModifierFlagCommand) {
	if (character == 'q') {
	  exit(EXIT_SUCCESS);
	} else if (character == '-') {
	  renderer->scale = max(renderer->scale - 0.1, 0.5);
	}
      } else if (flags == command_shift) {
	if (character == '+') {
	  renderer->scale = min(renderer->scale + 0.1, 2);
	}
      } else if (flags == NSEventModifierFlagControl) {
	if (character == 'f') {
	  move_right(gap_buffer);
	} else if (character == 'b') {
	  move_left(gap_buffer);
	} else if (character == 'p') {
	  move_up(gap_buffer);
	} else if (character == 'n') {
	  move_down(gap_buffer);
	} else if (character == 'a') {
	  move_to_line_start(gap_buffer);
	} else if (character == 'e') {
	  move_to_line_end(gap_buffer);
	}
      } else if (flags == 0 || flags == NSEventModifierFlagShift) {
	U8* source = (U8*) [characters UTF8String];
	U64 length = [characters lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
	for (U64 i = 0; i < length; i++) {
	  add_character(gap_buffer, source[i]);
	}
      }
    } else {
      [app sendEvent:event];
    }
    // NSLog(@"%@", event);
    [app updateWindows];
  }
}

int main() {
  setenv("MTL_DEBUG_LAYER", "1", 1);
  
  Arena arena;
  arena_initialize(&arena, 1ull << 32);

  App*       app        = make_app();
  Renderer*  renderer   = make_renderer(&arena, app, 1024);
  GapBuffer* gap_buffer = make_gap_buffer(&arena, 4096);

  while (true) {
    @autoreleasepool {
      U64 used = arena.used;
      
      handle_input(app, gap_buffer, renderer);
      
      draw_start(renderer);
      draw_buffer(&arena, gap_buffer, renderer);
      draw_end(renderer);

      arena.used = used;
    }
  }
}
