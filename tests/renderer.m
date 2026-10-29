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
 struct layer_border* b=calloc(1,sizeof(*b)); b->scale=2; b->current=-1;
 b->root=[[CALayer layer] retain]; b->base=[[CALayer layer] retain];
 layer_configure_content(b->base,2); [b->root addSublayer:b->base];
 struct layer_style s={.color={.stype=COLOR_STYLE_SOLID,.color=0xffb4befe},.border_width=5,.outer_offset=13,.inner_radius=27,.corner_radius=26,.border_style=BORDER_STYLE_ROUND};
 assert(layer_border_update(b,&s,CGSizeMake(1200,800)));
 CGImageRef opaque=b->art[b->current].base; assert(nonzero(opaque)>0);
 size_t width=CGImageGetWidth(opaque),height=CGImageGetHeight(opaque);
 for(int i=0;i<1000;i++) {
  s.color.color=(i%2) ? 0xffb4befe : 0;
  assert(layer_border_update(b,&s,CGSizeMake(1200+i,800+i)));
  CGImageRef img=b->art[b->current].base;
  assert(CGImageGetWidth(img)==width && CGImageGetHeight(img)==height);
  assert((nonzero(img)>0)==(s.color.color!=0));
  if(s.color.color) assert(img==opaque);
 }
 printf("PASS: 1000 alternating opaque/transparent resizes reuse two %zux%zu images\n",width,height);
 layer_border_destroy(b);
 } return 0; }
