// Record production update side effects without connecting to WindowServer.
#include "../src/border.h"
#include "../src/layer.h"
#include <stdio.h>

struct settings g_settings = { .border_width=5, .border_order=BORDER_ORDER_BELOW };
mach_port_t g_server_port;
static char calls[64];
static unsigned call_count;
static bool transaction_fails;
static void record(char call) { assert(call_count+1<sizeof(calls)); calls[call_count++]=call; calls[call_count]=0; }
static CGError fake_bounds(int cid,uint32_t wid,CGRect* frame) { *frame=CGRectMake(100,200,100,100); return 0; }
static uint64_t fake_tags(int cid,uint32_t wid) { return 0; }
static bool fake_visible(int cid,uint64_t sid) { return true; }
static CGError fake_ordered(int cid,uint32_t wid,bool* shown) { *shown=true; return 0; }
static int fake_level(int cid,uint32_t wid) { return 4; }
static int32_t fake_sublevel(int cid,uint32_t wid) { return 7; }
static CGError fake_disable(int cid) { record('D'); return 0; }
static CGError fake_enable(int cid) { record('E'); return 0; }
static CGError fake_region(CGRect* rect,CFTypeRef* region) { *region=CFRetain(kCFBooleanTrue); return 0; }
static CGError fake_shape(int cid,uint32_t wid,float x,float y,CFTypeRef shape) { record('S'); return 0; }
static bool fake_render(struct layer_border* layer,const struct layer_style* style,CGSize size) { record('R'); return true; }
static CFTypeRef fake_transaction(int cid) { record('T'); return transaction_fails ? NULL : CFRetain(kCFBooleanTrue); }
static CGError fake_move(CFTypeRef t,uint32_t wid,CGPoint origin) { assert(origin.x==87 && origin.y==187); record('M'); return 0; }
static CGError fake_transform(CFTypeRef t,uint32_t wid,int a,int b,CGAffineTransform transform) { assert(transform.tx==-87 && transform.ty==-187); record('X'); return 0; }
static CGError fake_set_level(CFTypeRef t,uint32_t wid,int level) { assert(level==4); record('V'); return 0; }
static CGError fake_set_sublevel(CFTypeRef t,uint32_t wid,int level) { assert(level==7); record('U'); return 0; }
static CGError fake_order(CFTypeRef t,uint32_t wid,int order,uint32_t target) { assert(order==BORDER_ORDER_BELOW && target==20); record('O'); return 0; }
static CGError fake_commit(CFTypeRef t,int sync) { record('K'); return 0; }
static CGError fake_set_tags(int cid,uint32_t wid,uint64_t* tags,int count) { assert(*tags==((1ULL<<1)|(1ULL<<9))); record('G'); return 0; }
static CGError fake_clear_tags(int cid,uint32_t wid,uint64_t* tags,int count) { assert(*tags==0); record('g'); return 0; }
#define SLSGetWindowBounds fake_bounds
#define window_tags fake_tags
#define is_space_visible fake_visible
#define SLSWindowIsOrderedIn fake_ordered
#define window_level fake_level
#define window_sub_level fake_sublevel
#define SLSDisableUpdate fake_disable
#define SLSReenableUpdate fake_enable
#define CGSNewRegionWithRect fake_region
#define SLSSetWindowShape fake_shape
#define layer_border_update fake_render
#define SLSTransactionCreate fake_transaction
#define SLSTransactionMoveWindowWithGroup fake_move
#define SLSTransactionSetWindowTransform fake_transform
#define SLSTransactionSetWindowLevel fake_set_level
#define SLSTransactionSetWindowSubLevel fake_set_sublevel
#define SLSTransactionOrderWindow fake_order
#define SLSTransactionCommit fake_commit
#define SLSSetWindowTags fake_set_tags
#define SLSClearWindowTags fake_clear_tags
#include "../src/border.c"

static void run(bool fail_transaction,bool resize) {
 struct border border; border_init(&border,2);
 border.wid=10; border.target_wid=20; border.sid=1;
 border.use_layer=true; border.layer=(struct layer_border*)1;
 border.frame=resize ? CGRectZero : CGRectMake(0,0,126,126);
 border.level=4; border.sub_level=7;
 if(!resize) { border.stale_props=false; border.tags_applied=true; }
 transaction_fails=fail_transaction; call_count=0; calls[0]=0;
 border_update_internal(&border,&g_settings);
 const char* expected=fail_transaction ? "DSRTE" : resize ? "DSRTMXVUOKGgE" : "RTMXVUOK";
 if(strcmp(calls,expected)) fprintf(stderr,"expected %s, received %s\n",expected,calls);
 assert(strcmp(calls,expected)==0);
 assert(border.tags_applied==!fail_transaction);
 pthread_mutex_destroy(&border.mutex);
}
int main(void) {
 run(false,true); run(true,true); run(false,false);
 puts("PASS: resize/order/tag sequence, failed transaction reenables updates, unchanged geometry skips reshape/tags");
}
