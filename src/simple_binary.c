#include "pebble.h"

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

static Window *window;

static Layer *display_layer;

#define CIRCLE_RADIUS 12
#define CIRCLE_LINE_THICKNESS 2

void draw_cell(GContext* ctx, GPoint center, bool filled)
{
    graphics_context_set_fill_color(ctx, GColorWhite);
    
    graphics_fill_circle(ctx, center, CIRCLE_RADIUS);
    
    if (!filled) {
        graphics_context_set_fill_color(ctx, GColorBlack);
        
        graphics_fill_circle(ctx, center, CIRCLE_RADIUS - CIRCLE_LINE_THICKNESS);
    }
    
}

#define CIRCLE_PADDING 2 // Number of padding pixels on each side
#define CELL_SIZE (2 * (CIRCLE_RADIUS + CIRCLE_PADDING)) // One "cell" is the square that contains the circle.
#define SIDE_PADDING (SCREEN_WIDTH - (4 * CELL_SIZE))/2

GPoint get_center_point_from_cell_location(unsigned short x, unsigned short y)
{
    // Cell location (0,0) is upper left, location (4, 6) is lower right.
    return GPoint(SIDE_PADDING + (CELL_SIZE/2) + (CELL_SIZE * x),
                  (CELL_SIZE/2) + (CELL_SIZE * y));
}

void draw_cell_row_for_digit(GContext* ctx, unsigned short number, unsigned short min_row_to_display, unsigned short max_row_to_display, unsigned short cell_col)
{
    unsigned short shift = 0;
    for (short cell_row_index = max_row_to_display; cell_row_index >= min_row_to_display; cell_row_index--) {
        draw_cell(ctx, get_center_point_from_cell_location(cell_col, cell_row_index), (number >> shift) & 0x1);
        shift++;
    }
}

#define HOURS_COLUMN 0
#define HOURS_FIRST_ROW_24 1
#define HOURS_FIRST_ROW_12 2
#define HOURS_LAST_ROW 5

#define MINUTES_COLUMN 3
#define MINUTES_FIRST_ROW 0
#define MINUTES_LAST_ROW 5

unsigned short get_display_hour(unsigned short hour)
{
    if (clock_is_24h_style()) {
        return hour;
    }
    
    unsigned short display_hour = hour % 12;
    
    // Converts "0" to "12"
    return display_hour ? display_hour : 12;
    
}

void display_layer_update_callback(Layer *me, GContext* ctx)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    unsigned short display_hour = get_display_hour(t->tm_hour);
    unsigned short display_minutes = t->tm_min;
    
    unsigned short hours_first_row;
    
    if (clock_is_24h_style()) {
        hours_first_row = HOURS_FIRST_ROW_24;
    } else {
        hours_first_row = HOURS_FIRST_ROW_12;
    }
    
    draw_cell_row_for_digit(ctx, display_hour, hours_first_row, HOURS_LAST_ROW, HOURS_COLUMN);
    draw_cell_row_for_digit(ctx, display_minutes, MINUTES_FIRST_ROW, MINUTES_LAST_ROW, MINUTES_COLUMN);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed)
{
    layer_mark_dirty(display_layer);
}

void init()
{
    window = window_create();
    window_stack_push(window, true);
    
    window_set_background_color(window, GColorBlack);
    
    Layer *root_layer = window_get_root_layer(window);
    GRect frame = layer_get_frame(root_layer);
    
    // Init the layer for the display
    display_layer = layer_create(frame);
    layer_set_update_proc(display_layer, &display_layer_update_callback);
    layer_add_child(root_layer, display_layer);
    
    tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
}

void deinit()
{
    layer_destroy(display_layer);
    window_destroy(window);
}


int main()
{
    init();
    app_event_loop();
    deinit();
}
