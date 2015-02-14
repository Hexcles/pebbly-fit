#include <pebble.h>

#define TS 1

static Window *window;
static TextLayer *text_layer;

static uint16_t s_worker_steps;
static char s_text[5];

static void worker_message_handler(uint16_t type, AppWorkerMessage *data) {
  if (type == TS){
    s_worker_steps = data->data0;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "s_worker_steps=%d", s_worker_steps);
    snprintf(s_text, 5, "%d", s_worker_steps);
    text_layer_set_text(text_layer, s_text);
    psleep(1000);
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(text_layer, "Pebbly");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, false);

  app_worker_message_subscribe(worker_message_handler);
  AppWorkerResult result = app_worker_launch();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "result=%d", result);
}

static void deinit(void) {
  window_destroy(window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "deinit!");
  app_worker_message_unsubscribe();
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();

  deinit();
}