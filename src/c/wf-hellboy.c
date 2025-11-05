#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static GFont s_time_font;

static TextLayer *s_date_layer;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

static int s_battery_level;
static Layer *s_battery_layer;


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

static void update_time() {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Write the current hours and minutes into a buffer
    static char s_time_buffer[8];
    strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

    static char s_date_buffer[10];
    strftime(s_date_buffer, sizeof(s_date_buffer), "%b %e", tick_time);

    // Display this time on the TextLayer
    text_layer_set_text(s_time_layer, s_time_buffer);
    text_layer_set_text(s_date_layer, s_date_buffer);
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
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
    gbitmap_destroy(s_background_bitmap);
    bitmap_layer_destroy(s_background_layer);
    gbitmap_destroy(s_bt_icon_bitmap);
    bitmap_layer_destroy(s_bt_icon_layer);
    layer_destroy(s_battery_layer);
}

static void init() {
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
}

static void deinit() {
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
