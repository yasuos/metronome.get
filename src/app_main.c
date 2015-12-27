#include <pebble.h>

#define WAKEUP_REASON 0
#define PERSIST_KEY_WAKEUP_ID 42

static Window *window = NULL;
static TextLayer *s_text_layer = NULL;

static ActionBarLayer *s_action_bar;
static GBitmap *s_icon_start;
static GBitmap *s_icon_stop;

static WakeupId s_wakeup_id = 0;

static void _text_layer_set_text( const char *text ){
    if( s_text_layer != NULL ){
        text_layer_set_text(s_text_layer, text );        
    }
}

static void check_wakeup() {
    // Get the ID
    s_wakeup_id = persist_read_int(PERSIST_KEY_WAKEUP_ID);

    if (s_wakeup_id > 0) {
        // There is a wakeup scheduled soon
        time_t timestamp = 0;
        wakeup_query(s_wakeup_id, &timestamp);
        int seconds_remaining = timestamp - time(NULL);
        
        // Show how many seconds to go
        static char s_buffer[64];
        snprintf(s_buffer, sizeof(s_buffer), "Vibes works\nafter %d sec.", seconds_remaining);
        _text_layer_set_text(s_buffer);

        //APP_LOG( APP_LOG_LEVEL_DEBUG, s_buffer );
    }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    check_wakeup();
}

static void wakeup_start(){
    //Check the event is not already scheduled
    if (!wakeup_query(s_wakeup_id, NULL)) {
        // Current time + 30 seconds
        time_t future_time = time(NULL) + 15;

        // Schedule wakeup event and keep the WakeupId
        s_wakeup_id = wakeup_schedule(future_time, WAKEUP_REASON, true);
        persist_write_int(PERSIST_KEY_WAKEUP_ID, s_wakeup_id);

        APP_LOG( APP_LOG_LEVEL_DEBUG, "waveup_start_1" );
    } else {
        //check_wakeup();
        //APP_LOG( APP_LOG_LEVEL_DEBUG, "waveup_start_2" );
    }    
}

static void wakeup_handler(WakeupId id, int32_t reason) {
    // The app has woken!
    //text_layer_set_text(s_text_layer, "It's time.");

    vibes_short_pulse();
    
    // Delete the ID
    persist_delete(PERSIST_KEY_WAKEUP_ID);
    
    APP_LOG( APP_LOG_LEVEL_DEBUG, "waveup_woken" );

    //
    wakeup_start();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    //_text_layer_set_text( "Vibes works\nevery 5 minutes.");
    
    wakeup_start();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    //_text_layer_set_text("Stop vibes.");
    _text_layer_set_text("Press Up button\nVibes works.");

    persist_delete(PERSIST_KEY_WAKEUP_ID);
    wakeup_cancel_all();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
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

    // Set the icons:
    s_icon_start = gbitmap_create_with_resource(RESOURCE_ID_IMG_ACT_START);
    s_icon_stop = gbitmap_create_with_resource(RESOURCE_ID_IMG_ACT_STOP);
    
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_start);
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_stop );

    //
    s_text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w-ACTION_BAR_WIDTH , 40 } });
    text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_text_layer));

    // 動作しているのか確認する
    s_wakeup_id = persist_read_int(PERSIST_KEY_WAKEUP_ID);
    if (s_wakeup_id > 0) {
        _text_layer_set_text( "Vibes works\nevery 5 minutes.");
    }else{
        _text_layer_set_text("Press Up button\nVibes works.");
    }
   
    APP_LOG( APP_LOG_LEVEL_DEBUG, "window_load" );
}

static void window_unload(Window *window) {
    action_bar_layer_destroy( s_action_bar );
    text_layer_destroy(s_text_layer);
    gbitmap_destroy(s_icon_start);
    gbitmap_destroy(s_icon_stop);

    APP_LOG( APP_LOG_LEVEL_DEBUG, "window_unload" );
}

static void init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });

    // Subscribe to Wakeup API
    wakeup_service_subscribe(wakeup_handler);
    
    //
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
        const bool animated = true;
        window_stack_push(window, animated);
    }
}

static void deinit(void) {
    tick_timer_service_unsubscribe();
    
    window_destroy(window);
    APP_LOG( APP_LOG_LEVEL_DEBUG, "deinit" );
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}