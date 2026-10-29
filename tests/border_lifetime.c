// Exercise the production lifetime path without creating WindowServer windows.
#include "../src/border.h"
#include "../src/layer.h"
#include <Block.h>

struct settings g_settings = { .border_width = 5.f };
mach_port_t g_server_port;
static dispatch_block_t pending[8];
static int pending_count, reads, released_windows, released_connections;

static void capture_async(dispatch_queue_t queue, dispatch_block_t block) {
  assert(pending_count < 8);
  pending[pending_count++] = Block_copy(block);
}
static CGError fake_bounds(int cid, uint32_t wid, CGRect* frame) {
  reads++;
  *frame = CGRectMake(100, 200, 800, 600);
  return kCGErrorSuccess;
}
static CFTypeRef fake_transaction(int cid) { return NULL; }
static int fake_main_connection(void) { return 1; }
static CGError fake_release_window(int cid, uint32_t wid) {
  assert(released_connections == 0);
  released_windows++;
  return kCGErrorSuccess;
}
static CGError fake_release_connection(int cid) {
  assert(released_windows == 2);
  released_connections++;
  return kCGErrorSuccess;
}
#define dispatch_async capture_async
#define SLSGetWindowBounds fake_bounds
#define SLSTransactionCreate fake_transaction
#define SLSMainConnectionID fake_main_connection
#define SLSReleaseWindow fake_release_window
#define SLSReleaseConnection fake_release_connection
#include "../src/border.c"

int main(int argc, char** argv) {
  struct border* border = calloc(1, sizeof(*border));
  border_init(border, 2);
  border->wid = 10;
  border->target_wid = 20;
  if (argc > 1 && strcmp(argv[1], "move") == 0) {
    border_move(border);
    assert(reads == 1 && "move must finish before caller can retire border");
    assert(pending_count == 0);
    assert(border->origin.x == 87 && border->origin.y == 187);
    pthread_mutex_destroy(&border->mutex);
    free(border);
  } else {
    border->proxy = calloc(1, sizeof(*border->proxy));
    border_init(border->proxy, 2);
    border->proxy->is_proxy = true;
    border->proxy->wid = 11;
    border_destroy(border);
    assert(pending_count == 1);
    dispatch_block_t cleanup = pending[0];
    pending_count = 0;
    cleanup();
    Block_release(cleanup);
    assert(pending_count == 0 && "proxy cleanup must precede connection release");
    assert(released_windows == 2 && released_connections == 1);
  }
  puts("border lifetime: passed");
}
