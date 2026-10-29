// Exercise the real message handler and parser without starting the daemon.
#define main borders_program_main
#include "../src/main.c"
#undef main
#include <assert.h>

static unsigned window_updates, all_updates, active_updates, inactive_updates, recreates;
static struct border* last_updated;
void border_update(struct border* border, bool try_async) {
  assert(border->needs_redraw && try_async);
  last_updated = border;
  window_updates++;
}
void windows_update_all(struct table* windows) { assert(windows == &g_windows); all_updates++; }
void windows_update_active(struct table* windows) { active_updates++; }
void windows_update_inactive(struct table* windows) { inactive_updates++; }
void windows_recreate_all_borders(struct table* windows) { recreates++; }

static void send_message(char* message) {
  window_updates = all_updates = active_updates = inactive_updates = recreates = 0;
  last_updated = NULL;
  message_handler(message, message ? strlen(message) + 1 : 0);
}

int main(void) {
  table_init(&g_windows, 4, hash_windows, cmp_windows);
  struct border overridden = { .target_wid = 101 };
  struct border ordinary = { .target_wid = 102 };
  overridden.setting_override = g_settings;
  overridden.setting_override.enabled = true;
  table_add(&g_windows, &overridden.target_wid, &overridden);
  table_add(&g_windows, &ordinary.target_wid, &ordinary);

  send_message(NULL);
  assert(!window_updates && !all_updates && !recreates);
  send_message("");
  assert(!window_updates && !all_updates && !recreates);
  send_message("unknown=fixture\0");
  assert(!window_updates && !all_updates && !recreates);

  send_message("width=7\0width=9\0style=s\0");
  assert(g_settings.border_width == 9 && g_settings.border_style == BORDER_STYLE_SQUARE);
  assert(overridden.setting_override.border_width == 9);
  assert(overridden.setting_override.border_style == BORDER_STYLE_SQUARE);
  assert(!ordinary.setting_override.enabled && ordinary.setting_override.border_width == 0);
  assert(all_updates == 1 && !window_updates && !recreates);

  // Preserve existing mask precedence: either color bit matches MASK_ALL.
  send_message("active_color=0xff123456\0");
  assert(g_settings.active_window.color == 0xff123456);
  assert(overridden.setting_override.active_window.color == 0xff123456);
  assert(all_updates == 1 && !active_updates && !window_updates);
  send_message("inactive_color=0xff654321\0");
  assert(all_updates == 1 && !inactive_updates && !window_updates);

  send_message("ax_focus=on\0");
  assert(g_settings.ax_focus && overridden.setting_override.ax_focus);
  assert(window_updates == 1 && last_updated == &overridden && !all_updates);

  send_message("width=11\0hidpi=on\0active_only=on\0");
  assert(g_settings.hidpi && g_settings.active_only && g_settings.border_width == 11);
  assert(overridden.setting_override.hidpi && overridden.setting_override.active_only);
  assert(recreates == 1 && !all_updates && !window_updates);

  send_message("apply-to=102\0width=13\0");
  assert(g_settings.apply_to == 0 && g_settings.border_width == 11);
  assert(ordinary.setting_override.enabled && ordinary.setting_override.apply_to == 102);
  assert(ordinary.setting_override.border_width == 13);
  assert(window_updates == 1 && last_updated == &ordinary && !all_updates && !recreates);

  send_message("apply-to=999\0width=17\0");
  assert(g_settings.border_width == 11 && ordinary.setting_override.border_width == 13);
  assert(!window_updates && !all_updates && !recreates);

  send_message("width=19\0\0width=23\0");
  assert(g_settings.border_width == 19 && ordinary.setting_override.border_width == 19);
  assert(overridden.setting_override.border_width == 19 && all_updates == 1);
  table_free(&g_windows);
  puts("PASS: settings messages preserve token order, overrides, targeting and update precedence");
  return 0;
}
