#include "../src/layer.m"
#include <assert.h>
static int nonzero(CGImageRef image) {
 CFDataRef data = CGDataProviderCopyData(CGImageGetDataProvider(image));
 const UInt8 *p = CFDataGetBytePtr(data); int count=0;
 for(CFIndex i=0;i<CFDataGetLength(data);i++) count += p[i]!=0;
 CFRelease(data); return count;
}
int main(void) { @autoreleasepool {
 printf("layer_supported=%d\n", layer_supported());
 struct layer_border* b=calloc(1,sizeof(*b)); b->scale=2; b->current_art_index=-1;
 b->root=[[CALayer layer] retain]; b->base=[[CALayer layer] retain];
 layer_configure_content(b->base,2); [b->root addSublayer:b->base];
 struct layer_style s={.color={.stype=COLOR_STYLE_SOLID,.color=0xffb4befe},.border_width=5,.outer_offset=13,.inner_radius=27,.corner_radius=26,.border_style=BORDER_STYLE_ROUND};
 assert(layer_border_update(b,&s,CGSizeMake(1200,800)));
 CGImageRef opaque=b->art[b->current_art_index].base; assert(nonzero(opaque)>0);
 size_t width=CGImageGetWidth(opaque),height=CGImageGetHeight(opaque);
 for(int i=0;i<1000;i++) {
  s.color.color=(i%2) ? 0xffb4befe : 0;
  assert(layer_border_update(b,&s,CGSizeMake(1200+i,800+i)));
  CGImageRef img=b->art[b->current_art_index].base;
  assert(CGImageGetWidth(img)==width && CGImageGetHeight(img)==height);
  assert((nonzero(img)>0)==(s.color.color!=0));
  if(s.color.color) assert(img==opaque);
 }
 printf("PASS: 1000 alternating opaque/transparent resizes reuse two %zux%zu images\n",width,height);
 // A repeated update must preserve both artwork and bounds.
 CGSize large=CGSizeMake(1200,800);
 assert(layer_border_update(b,&s,large));
 CGImageRef first=CGImageRetain((CGImageRef)b->base.contents);
 CGRect unchanged_bounds=b->root.bounds;
 assert(layer_border_update(b,&s,large));
 assert((CGImageRef)b->base.contents==first);
 assert(CGRectEqualToRect(b->root.bounds,unchanged_bounds));
 // Retain observed images so an allocator cannot reuse their addresses.
 s.color.color=0xff123456;
 assert(layer_border_update(b,&s,large));
 CGImageRef second=CGImageRetain((CGImageRef)b->base.contents);
 s.color.color=0xffb4befe;
 assert(layer_border_update(b,&s,large));
 assert((CGImageRef)b->base.contents==first);
 s.color.color=0xffabcdef;
 assert(layer_border_update(b,&s,large));
 s.color.color=0xffb4befe;
 assert(layer_border_update(b,&s,large));
 assert((CGImageRef)b->base.contents==first);
 s.color.color=0xff123456;
 assert(layer_border_update(b,&s,large));
 assert((CGImageRef)b->base.contents!=second);
 CGImageRelease(first); CGImageRelease(second);
 // Corners cannot be stretched at small sizes: exact image, then template.
 assert(layer_border_update(b,&s,CGSizeMake(40,40)));
 assert(CGImageGetWidth((CGImageRef)b->base.contents)==80);
 assert(layer_border_update(b,&s,large));
 assert(CGImageGetWidth((CGImageRef)b->base.contents)==width);
 // Gradient artwork uses a mask; a solid update must clear the old mask/fill.
 s.color.stype=COLOR_STYLE_GRADIENT;
 s.color.gradient=(struct gradient){.direction=TL_TO_BR,.color1=0xffff0000,.color2=0xff0000ff};
 s.show_background=true;
 s.background=(struct color_style){.stype=COLOR_STYLE_SOLID,.color=0x55223344};
 assert(layer_border_update(b,&s,large));
 assert(b->base.hidden && !b->gradient.hidden && b->gradient_mask.contents);
 assert(!b->background.hidden && b->background.contents);
 CGRect expected=CGRectMake(0,0,large.width,large.height);
 assert(CGRectEqualToRect(b->root.bounds,expected));
 assert(CGRectEqualToRect(b->gradient.bounds,expected));
 assert(CGRectEqualToRect(b->gradient_mask.bounds,expected));
 assert(CGRectEqualToRect(b->background.bounds,expected));
 s.color=(struct color_style){.stype=COLOR_STYLE_SOLID,.color=0};
 s.show_background=false;
 assert(layer_border_update(b,&s,large));
 assert(!b->base.hidden && nonzero((CGImageRef)b->base.contents)==0);
 assert(b->gradient.hidden && !b->gradient_mask.contents);
 assert(b->background.hidden && !b->background.contents);
 assert(layer_border_update(b,&s,CGSizeMake(0,0)));
 assert(CGRectEqualToRect(b->root.bounds,expected));
 puts("PASS: unchanged artwork, two-entry eviction, exact-size/template transition, gradient mask/background cleanup");
 layer_border_destroy(b);
 } return 0; }
