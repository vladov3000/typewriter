// -*- mode: objc -*-
typedef struct Letter Letter;

struct Letter {
  U8      codepoint;
  Sprite  sprite;
  Letter* next;
};

typedef struct {
  I64             width;
  I64             height;
  F32             scale;
  F32             max_height;
  F32             next_x;
  F32             next_y;
  U8*             data;
  CFDictionaryRef attributes;
  CGContextRef    context;
  Letter*         letters;
  id<MTLTexture>  texture;
} Atlas;

static id<MTLTexture> make_texture(id<MTLDevice> device, I64 width, I64 height) {
  MTLPixelFormat pixel_format = MTLPixelFormatRGBA8Unorm;
  return [device newTextureWithDescriptor:
	     [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixel_format
								width:width
							       height:height
							    mipmapped:NO]];
}

static CFURLRef append_to_url(CFURLRef url, CFStringRef path) {
  return CFURLCreateCopyAppendingPathComponent(NULL, url, path, false);
}

static CTFontRef make_font(CFStringRef path) {
  CFBundleRef       bundle     = CFBundleGetMainBundle();
  CFURLRef          bundle_url = CFBundleCopyResourcesDirectoryURL(bundle);
  CFURLRef          url        = append_to_url(bundle_url, path);
  CGDataProviderRef provider   = CGDataProviderCreateWithURL(url);
  CGFontRef         cgFont     = CGFontCreateWithDataProvider(provider);
  CTFontRef         font       = CTFontCreateWithGraphicsFont(cgFont, 48, NULL, NULL);
  CFRelease(cgFont);
  CFRelease(provider);
  CFRelease(url);
  CFRelease(bundle_url);
  CFRelease(bundle);
  return font;
}

static CFDictionaryRef make_attributes() {
  CTFontRef font = make_font(CFSTR("FiraCode.ttf"));
  
  CFStringRef keys  [] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
  CFTypeRef   values[] = { font, CGColorGetConstantColor(kCGColorWhite) };

  CFDictionaryRef attributes = CFDictionaryCreate(
    kCFAllocatorDefault,
    (const void**) &keys,
    (const void**) &values,
    length(keys),
    &kCFTypeDictionaryKeyCallBacks,
    &kCFTypeDictionaryValueCallBacks
  );
  
  return attributes;
}

static CGContextRef make_context(U8* data, I64 width, I64 height) {
  CGContextRef context = CGBitmapContextCreate(
    data,
    width,
    height,
    8,
    width * 4,
    CGColorSpaceCreateWithName(kCGColorSpaceGenericRGBLinear),
    kCGImageAlphaPremultipliedLast
  );
  assert(context != NULL);
  return context;
}

static Sprite cache_text(Atlas* atlas, String text) {
  CFDictionaryRef attributes = atlas->attributes;
  CGContextRef    context    = atlas->context;
  id<MTLTexture>  texture    = atlas->texture;

  CFStringRef string = CFStringCreateWithBytesNoCopy(
    NULL,
    (UInt8*) text.data,
    text.size,
    kCFStringEncodingUTF8,
    false,
    kCFAllocatorNull
  );

  CFAttributedStringRef attributed_string = CFAttributedStringCreate(
    NULL, string, atlas->attributes
  );

  CGFloat   ascent, descent, leading;
  CTLineRef line   = CTLineCreateWithAttributedString(attributed_string);
  F32       width  = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
  F32       height = ascent + descent;

  if (width > 0) {
    CGRect clear = { atlas->next_x, atlas->next_y - height, width, height };
    CGContextClearRect(context, clear);
    CGContextSetTextPosition(context, atlas->next_x, atlas->next_y - ascent);
    CTLineDraw(line, context);
    
    MTLRegion region = {
      { atlas->next_x, atlas->height - atlas->next_y, 0 },
      { width, height, 1 }
    };
    U64 index = (atlas->next_x + atlas->width * (atlas->height - atlas->next_y)) * 4;
    U8* data  = &atlas->data[index];
    [texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:atlas->width * 4];
  }
  
  CFRelease(line);
  CFRelease(attributed_string);
  CFRelease(string);

  Sprite sprite     = {};
  sprite.size[0]    = width         / atlas->scale;
  sprite.size[1]    = height        / atlas->scale;
  sprite.uv[0]      = atlas->next_x / atlas->width;
  sprite.uv[1]      = atlas->next_y / atlas->height;
  sprite.uv_size[0] = floor(width)  / atlas->width;
  sprite.uv_size[1] = height        / atlas->height;

  if (height > atlas->max_height) {
    atlas->max_height = height;
  }

  atlas->next_x += width;
  if (atlas->next_x > atlas->width) {
    atlas->next_x     = 0;
    atlas->next_y    -= atlas->max_height;
    atlas->max_height = 0;
  }
  assert(atlas->next_y >= 0);
  return sprite;
}

static Sprite draw_letter(Arena* arena, Atlas* atlas, U8 codepoint) {
  for (Letter* i = atlas->letters; i; i = i->next) {
    if (i->codepoint == codepoint) {
      return i->sprite;
    }
  }
  String  text   = { &codepoint, 1 };
  Letter* new    = arena_allocate(arena, Letter);
  new->codepoint = codepoint;
  new->sprite    = cache_text(atlas, text);
  new->next      = atlas->letters;
  atlas->letters = new;
  return new->sprite;
}

static Atlas* make_atlas(Arena* arena, id<MTLDevice> device, F32 scale, I64 width, I64 height) {
  Atlas* atlas      = arena_allocate_zeroed(arena, Atlas);
  atlas->width      = width;
  atlas->height     = height;
  atlas->scale      = scale;
  atlas->data       = arena_allocate_bytes(arena, 4 * width * height, 1);
  atlas->attributes = make_attributes();
  atlas->context    = make_context(atlas->data, width, height);
  atlas->texture    = make_texture(device, width, height);
  return atlas;
}
