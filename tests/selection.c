// Exercise the real selection and notification handlers with no WindowServer calls.
#include "../src/windows.h"
#include "../src/border.h"
#include "../src/misc/ax.h"
#include "../src/events.h"
#include <libproc.h>
#include <stdio.h>
#include <assert.h>

static uint32_t front = 101;
static uint64_t space = 1;
static bool suitable = true;
static unsigned created, destroyed, live, updates, notifications;
static int mock_main_connection(void) { return 1; }
static CGError mock_owner(int cid, uint32_t wid, int* owner) { *owner = 2; return 0; }
static CGError mock_pid(int cid, pid_t* pid) { *pid = 999; return 0; }
static int mock_name(int pid, void* name, uint32_t size) { snprintf(name, size, "Fixture"); return 7; }
static uint32_t mock_front(int cid) { return front; }
static uint64_t mock_space(int cid, uint32_t wid) { return space; }
static bool mock_suitable(CFTypeRef iterator) { return suitable; }
static CFTypeRef mock_query(int cid, CFArrayRef windows, uint32_t options) { return CFRetain(windows); }
static CFTypeRef mock_iterator(CFTypeRef query) { return CFRetain(query); }
static int mock_count(CFTypeRef iterator) { return (int)CFArrayGetCount(iterator); }
static bool mock_advance(CFTypeRef iterator) { return true; }
static CGError mock_notifications(int cid, uint32_t* ids, int count) { notifications++; return 0; }
static CFArrayRef mock_displays(int cid) { return CFArrayCreate(NULL,NULL,0,&kCFTypeArrayCallBacks); }
static CFArrayRef mock_windows(int cid, uint32_t owner, CFArrayRef spaces, uint32_t options, uint64_t* set, uint64_t* clear) { return NULL; }

#define SLSMainConnectionID mock_main_connection
#define SLSGetWindowOwner mock_owner
#define SLSConnectionGetPID mock_pid
#define proc_name mock_name
#define get_front_window mock_front
#define ax_get_front_window mock_front
#define window_space_id mock_space
#define window_suitable mock_suitable
#define SLSWindowQueryWindows mock_query
#define SLSWindowQueryResultCopyWindows mock_iterator
#define SLSWindowIteratorGetCount mock_count
#define SLSWindowIteratorAdvance mock_advance
#define SLSRequestNotificationsForWindows mock_notifications
#define SLSCopyManagedDisplays mock_displays
#define SLSCopyWindowsWithOptionsAndTags mock_windows
#include "../src/windows.c"
#include "../src/events.c"
#include "../src/parse.c"

pid_t g_pid = 123;
struct settings g_settings;
struct table g_windows;
CFArrayRef (*JBSLSWindowIteratorGetCornerRadii)(CFTypeRef);
struct border* border_create(void) { struct border* b = calloc(1,sizeof(*b)); created++; live++; return b; }
void border_destroy(struct border* b) { destroyed++; live--; free(b); }
void border_update(struct border* b, bool async) { updates++; }
void border_invalidate_props(struct border* b) { b->stale_props = true; }
void border_move(struct border* b) { }
void border_hide(struct border* b) { }
void border_unhide(struct border* b) { }
static unsigned long hash(void* key) { return *(uint32_t*)key; }
static int equal(void* a, void* b) { return *(uint32_t*)a == *(uint32_t*)b; }
static void expect_selected(uint32_t wid) {
 assert(g_windows.count == (wid ? 1 : 0));
 assert(live == (wid ? 1 : 0));
 if (wid) { struct border* b = table_find(&g_windows,&wid); assert(b && b->focused && b->target_wid == wid); }
}
int main(void) {
#ifdef TEST_TIMERS
 static struct coalesced_job job; static unsigned count; static uint64_t started;
 started=clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
 dispatch_block_t action=^{ count++; printf("callback %u at %.3f ms\n",count,(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)-started)/1e6); };
 for(unsigned i=0;i<1000;i++) schedule_coalesced(&job,50000,action);
 schedule_coalesced(&job,10000,action);
 dispatch_after(dispatch_time(DISPATCH_TIME_NOW,150*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
  assert(count==2 && !job.pending);
  puts("PASS: 1001 scheduler requests coalesce to earliest and trailing callback"); exit(0);
 });
 CFRunLoopRun(); return 1;
#endif
 table_init(&g_windows,4,hash,equal);
 g_settings.enabled = true;
#ifndef TEST_PRE_ACTIVE_ONLY
 char* on[] = { "active_only=on" };
 char* off[] = { "active_only=off" };
 assert(parse_settings(&g_settings,1,on) & BORDER_UPDATE_MASK_RECREATE_ALL);
 assert(g_settings.active_only);
 assert(parse_settings(&g_settings,1,off) & BORDER_UPDATE_MASK_RECREATE_ALL);
 assert(!g_settings.active_only);
 parse_settings(&g_settings,1,on);
#endif
 windows_determine_and_focus_active_window(&g_windows); expect_selected(101);
 front=102; windows_determine_and_focus_active_window(&g_windows); expect_selected(102);
 assert(created == 2 && destroyed == 1); // A new target must not reuse the old surface.
 unsigned previous_updates=updates;
 space=2; windows_draw_borders_on_current_spaces(&g_windows); expect_selected(102);
 struct border* b=table_find(&g_windows,&front); assert(b->sid == 2 && updates > previous_updates);
 front=103; suitable=false; windows_determine_and_focus_active_window(&g_windows); expect_selected(0);
 suitable=true; windows_determine_and_focus_active_window(&g_windows); expect_selected(103);
 front=0; windows_determine_and_focus_active_window(&g_windows); expect_selected(0);
 for(unsigned i=0;i<1000;i++) { front=200+i; windows_determine_and_focus_active_window(&g_windows); expect_selected(front); }
 // Closing the selected window must find the new focus without a reorder event.
 uint32_t closed=front; front=2000;
 window_modify_handler(EVENT_WINDOW_CLOSE,&closed,sizeof(closed),1);
 CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.1,false);
 expect_selected(2000);
 windows_remove_all(&g_windows); expect_selected(0); assert(created==destroyed);
 table_free(&g_windows);
 printf("PASS: focus switching, no surface reuse, same-ID space refresh, unsuitable/no focus, 1000 cycles, close-to-new-focus; %u allocations balanced\n",created);
}
