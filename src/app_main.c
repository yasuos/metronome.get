#include <pebble.h>

#define WAKEUP_REASON 0
#define PERSIST_KEY_WAKEUP_ID 42

static Window *s_window = NULL;

static TextLayer *s_text_layer = NULL;
static TextLayer *s_count_layer = NULL;

static ActionBarLayer *s_action_bar;
static GBitmap *s_icon_start;
static GBitmap *s_icon_stop;

static WakeupId s_wakeup_id = 0;
static char s_buffer[8];


static void check_wakeup() {
    // Get the ID
    s_wakeup_id = persist_read_int(PERSIST_KEY_WAKEUP_ID);
    if (s_wakeup_id > 0) {
        // There is a wakeup scheduled soon
        time_t timestamp = 0;
        wakeup_query(s_wakeup_id, &timestamp);
        int seconds_remaining = timestamp - time(NULL);
        
        // Show how many seconds to go
        snprintf(s_buffer, sizeof(s_buffer), "%d", seconds_remaining);
        if( s_count_layer != NULL ){
            text_layer_set_text(s_count_layer, s_buffer);            
        }
    }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    check_wakeup();
}

static void wakeup_start(){
    wakeup_cancel_all();        
    persist_delete(PERSIST_KEY_WAKEUP_ID);

    //Check the event is not already scheduled
    if (!wakeup_query(s_wakeup_id, NULL)) {
        time_t future_time = time(NULL) + 300;
        //time_t future_time = time(NULL) + 15;

        // Schedule wakeup event and keep the WakeupId
        s_wakeup_id = wakeup_schedule(future_time, WAKEUP_REASON, false );
        persist_write_int(PERSIST_KEY_WAKEUP_ID, s_wakeup_id);
    }
}

static void wakeup_handler(WakeupId id, int32_t reason) {
    // バイブを2回鳴らす
    vibes_double_pulse();
    
    // Delete the ID
    //persist_delete(PERSIST_KEY_WAKEUP_ID);
    
    // もういちど実行する
    wakeup_start();

    APP_LOG( APP_LOG_LEVEL_DEBUG, "waveup!" );
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    int id = persist_read_int(PERSIST_KEY_WAKEUP_ID);
    
    if( id <= 0 ) {
        wakeup_start();
        
        text_layer_set_text(s_text_layer, "vibes work to");
        action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_stop );
    } else {
        text_layer_set_text(s_text_layer, "Press select.");
        text_layer_set_text(s_count_layer, "000");

        action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_start );

        wakeup_cancel_all();
        persist_delete(PERSIST_KEY_WAKEUP_ID);
    }
}

/*
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    wakeup_start();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    _text_layer_set_text("Press Up button\nVibes works.");

    persist_delete(PERSIST_KEY_WAKEUP_ID);
    wakeup_cancel_all();
}
*/

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  //window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  //window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Initialize the action bar:
    s_action_bar = action_bar_layer_create();

    // Associate the action bar with the window:
    action_bar_layer_add_to_window(s_action_bar, window);

    // Set the click config provider:
    action_bar_layer_set_click_config_provider(s_action_bar,
                                             click_config_provider);

    // icons load
    s_icon_start = gbitmap_create_with_resource(RESOURCE_ID_IMG_ACT_START);
    s_icon_stop = gbitmap_create_with_resource(RESOURCE_ID_IMG_ACT_STOP);
    
    // メッセージフォント
    s_text_layer = text_layer_create((GRect) { .origin = { 0, 52 }, .size = { bounds.size.w-ACTION_BAR_WIDTH , 20 } });
    text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
    
    // カウンターフォント
    s_count_layer = text_layer_create((GRect) { .origin = { 0, 68 }, .size = { bounds.size.w-ACTION_BAR_WIDTH , 46 } });
    text_layer_set_font(s_count_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
    text_layer_set_text_alignment(s_count_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_count_layer));

    // 動作しているのか確認する
    s_wakeup_id = persist_read_int(PERSIST_KEY_WAKEUP_ID);
    if( s_wakeup_id <= 0) {
        text_layer_set_text(s_text_layer, "Press select.");
        text_layer_set_text(s_count_layer, "000");
        
        action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_start );
    } else {
        text_layer_set_text(s_text_layer, "vibes work to");

        action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_stop );        
    }
   
    APP_LOG( APP_LOG_LEVEL_DEBUG, "window_load" );
}

static void window_unload(Window *window) {    
    text_layer_destroy(s_text_layer);
    text_layer_destroy(s_count_layer);

    gbitmap_destroy(s_icon_start);
    gbitmap_destroy(s_icon_stop);

    action_bar_layer_destroy( s_action_bar );

    APP_LOG( APP_LOG_LEVEL_DEBUG, "window_unload" );
}

static void init(void) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });

/*
    // Subscribe to Wakeup API
    wakeup_service_subscribe(wakeup_handler);
    
    // Subscribe to tick timer
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
*/
    
    // Subscribe to tick timer : 動かすな
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

    // Was this a wakeup launch?
    if (launch_reason() == APP_LAUNCH_WAKEUP) {
        // The app was started by a wakeup
        WakeupId id = 0;
        int32_t reason = 0;

        // Get details and handle the wakeup
        wakeup_get_launch_event(&id, &reason);
        wakeup_handler(id, reason);
    } else {
        //const bool animated = true;
        // 動かすな
        window_stack_push(s_window, true );
    
        // Subscribe to Wakeup API：動かすな
        wakeup_service_subscribe(wakeup_handler);
    }

    APP_LOG( APP_LOG_LEVEL_DEBUG, "init" );
}

static void deinit(void) {
    tick_timer_service_unsubscribe();
    
    window_destroy(s_window);
    
    APP_LOG( APP_LOG_LEVEL_DEBUG, "deinit" );
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}