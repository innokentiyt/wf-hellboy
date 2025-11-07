#include <pebble.h>

#define SETTINGS_KEY 1

static Window *s_main_window;

static TextLayer *s_time_layer;
static GFont s_time_font;

static Layer *s_am_pm_layer;
static BitmapLayer *s_am_icon_layer;
static BitmapLayer *s_pm_icon_layer;
static GBitmap *s_am_icon_bitmap;
static GBitmap *s_pm_icon_bitmap;

static TextLayer *s_date_layer;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

static int s_battery_level;
static Layer *s_battery_layer;

typedef struct ClaySettings {
  bool show_am_pm;
  bool show_leading_zero;
  bool show_weekday;
} __attribute__((__packed__)) ClaySettings;

static ClaySettings settings;


static void update_time() {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    static char s_time_buffer[8];
    char *time_format;
    if (clock_is_24h_style()) {
        if (settings.show_leading_zero) {
            time_format = "%H:%M";
        } else {
            time_format = "%k:%M";
        }
    } else {
        if (settings.show_leading_zero) {
            time_format = "%I:%M";
        } else {
            time_format = "%l:%M";
        }
    }
    strftime(s_time_buffer, sizeof(s_time_buffer), time_format, tick_time);

    static char s_date_buffer[10];
    strftime(s_date_buffer, sizeof(s_date_buffer), settings.show_weekday ? "%a %e" : "%b %e", tick_time);

    text_layer_set_text(s_time_layer, s_time_buffer);
    text_layer_set_text(s_date_layer, s_date_buffer);

    layer_set_hidden(s_am_pm_layer, !settings.show_am_pm);
    if (tick_time->tm_hour >= 0 && tick_time->tm_hour <= 11) {
        // AM
        layer_set_hidden(bitmap_layer_get_layer(s_am_icon_layer), false);
        layer_set_hidden(bitmap_layer_get_layer(s_pm_icon_layer), true);
    } else {
        // PM
        layer_set_hidden(bitmap_layer_get_layer(s_am_icon_layer), true);
        layer_set_hidden(bitmap_layer_get_layer(s_pm_icon_layer), false);
    }
}

// default settings initializer
static void prv_default_settings() {
  settings.show_am_pm = false;
  settings.show_leading_zero = true;
  settings.show_weekday = false;
}

static void prv_load_settings() {
  // Load the default settings
  prv_default_settings();
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  // force update
  update_time();
}

// AppMessage receive handler
static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *show_am_pm_t = dict_find(iter, MESSAGE_KEY_show_am_pm);
    if (show_am_pm_t) {
        settings.show_am_pm = show_am_pm_t->value->int32 == 1;
    }

    Tuple *show_leading_zero_t = dict_find(iter, MESSAGE_KEY_show_leading_zero);
    if (show_leading_zero_t) {
        settings.show_leading_zero = show_leading_zero_t->value->int32 == 1;
    }

    Tuple *show_weekday_t = dict_find(iter, MESSAGE_KEY_show_weekday);
    if (show_weekday_t) {
        settings.show_weekday = show_weekday_t->value->int32 == 1;
    }

    prv_save_settings();
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    int zero_percent_y = bounds.size.h - 2;

    GPoint start = GPoint(bounds.size.w, 0);
    GPoint end = GPoint(
        (s_battery_level * bounds.size.w * 2) / 200,
        zero_percent_y - ((zero_percent_y * s_battery_level * 2) / 200)
    );

    // Draw a line
    graphics_context_set_stroke_width(ctx, 2);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_line(ctx, start, end);
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
}

static void bluetooth_callback(bool connected) {
    if (connected) {
        layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), false);
    } else {
        layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), true);
    }
    layer_mark_dirty(bitmap_layer_get_layer(s_bt_icon_layer));
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
}

static void main_window_load(Window *window) {
    // Get information about the Window
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_time_layer = text_layer_create(GRect(0, 120, bounds.size.w - 10, 50));
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_JOST_MEDIUM_34));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_font(s_time_layer, s_time_font);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);

    s_am_pm_layer = layer_create(GRect(bounds.size.w - 14 - 11, bounds.size.h - 5 - 5, 14, 5));
    s_am_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_AM_ICON);
    s_am_icon_layer = bitmap_layer_create(GRect(0, 0, 14, 5));
    bitmap_layer_set_bitmap(s_am_icon_layer, s_am_icon_bitmap);
    s_pm_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PM_ICON);
    s_pm_icon_layer = bitmap_layer_create(GRect(0, 0, 14, 5));
    bitmap_layer_set_bitmap(s_pm_icon_layer, s_pm_icon_bitmap);
    layer_set_hidden(s_am_pm_layer, !settings.show_am_pm);

    s_date_layer = text_layer_create(GRect(0, 106, bounds.size.w - 10, 18));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorWhite);
    text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);

    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);
    s_background_layer = bitmap_layer_create(bounds);
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);

    s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_ICON);
    s_bt_icon_layer = bitmap_layer_create(GRect(136, 2, 6, 9));
    bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
    bluetooth_callback(connection_service_peek_pebble_app_connection());

    s_battery_layer = layer_create(GRect(0, 75, 14, 9));
    layer_set_update_proc(s_battery_layer, battery_update_proc);

    // order has meaning
    layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
    layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_icon_layer));
    layer_add_child(window_layer, s_battery_layer);
    layer_add_child(window_layer, s_am_pm_layer);
        layer_add_child(s_am_pm_layer, bitmap_layer_get_layer(s_am_icon_layer));
        layer_add_child(s_am_pm_layer, bitmap_layer_get_layer(s_pm_icon_layer));
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_time_layer);

    gbitmap_destroy(s_am_icon_bitmap);
    bitmap_layer_destroy(s_am_icon_layer);
    gbitmap_destroy(s_pm_icon_bitmap);
    bitmap_layer_destroy(s_pm_icon_layer);
    layer_destroy(s_am_pm_layer);

    text_layer_destroy(s_date_layer);

    gbitmap_destroy(s_background_bitmap);
    bitmap_layer_destroy(s_background_layer);

    gbitmap_destroy(s_bt_icon_bitmap);
    bitmap_layer_destroy(s_bt_icon_layer);

    layer_destroy(s_battery_layer);
}

static void init() {
    prv_load_settings();

    // Create main Window element and assign to pointer
    s_main_window = window_create();

    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });

    // Show the Window on the watch, with animated=true
    window_stack_push(s_main_window, true);
    update_time();

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = bluetooth_callback
    });
    battery_state_service_subscribe(battery_callback);
    battery_callback(battery_state_service_peek());

    // Listen for AppMessages
    app_message_register_inbox_received(prv_inbox_received_handler);
    app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, 0);
}

static void deinit() {
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
