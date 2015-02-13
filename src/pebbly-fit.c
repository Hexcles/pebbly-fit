#include <pebble.h>
#include <math.h>

#define TS 1
#define TSD 1

static AppTimer *timer;

const int ACCEL_STEP_MS = 475;
const int PED_ADJUST = 2;
const int STEP_INCREMENT = 50;

int X_DELTA = 45;
int Y_DELTA, Z_DELTA = 235;
int YZ_DELTA_MIN = 225;
int YZ_DELTA_MAX = 245; 
int X_DELTA_TEMP, Y_DELTA_TEMP, Z_DELTA_TEMP = 0;
int lastX, lastY, lastZ, currX, currY, currZ = 0;

long pedometerCount = 0;
long tempTotal = 0;

bool validX, validY, validZ = false;
bool startedSession = false;

static long totalSteps = TSD;

static Window *window;
static TextLayer *text_layer;

void autoCorrectZ(){
  if (Z_DELTA > YZ_DELTA_MAX){
    Z_DELTA = YZ_DELTA_MAX; 
  } else if (Z_DELTA < YZ_DELTA_MIN){
    Z_DELTA = YZ_DELTA_MIN;
  }
}

void autoCorrectY(){
  if (Y_DELTA > YZ_DELTA_MAX){
    Y_DELTA = YZ_DELTA_MAX; 
  } else if (Y_DELTA < YZ_DELTA_MIN){
    Y_DELTA = YZ_DELTA_MIN;
  }
}

void pedometer_update() {
  if (startedSession) {
    X_DELTA_TEMP = abs(abs(currX) - abs(lastX));
    if (X_DELTA_TEMP >= X_DELTA) {
      validX = true;
    }
    Y_DELTA_TEMP = abs(abs(currY) - abs(lastY));
    if (Y_DELTA_TEMP >= Y_DELTA) {
      validY = true;
      if (Y_DELTA_TEMP - Y_DELTA > 200){
        autoCorrectY();
        Y_DELTA = (Y_DELTA < YZ_DELTA_MAX) ? Y_DELTA + PED_ADJUST : Y_DELTA;
      } else if (Y_DELTA - Y_DELTA_TEMP > 175){
        autoCorrectY();
        Y_DELTA = (Y_DELTA > YZ_DELTA_MIN) ? Y_DELTA - PED_ADJUST : Y_DELTA;
      }
    }
    Z_DELTA_TEMP = abs(abs(currZ) - abs(lastZ));
    if (abs(abs(currZ) - abs(lastZ)) >= Z_DELTA) {
      validZ = true;
      if (Z_DELTA_TEMP - Z_DELTA > 200){
        autoCorrectZ();
        Z_DELTA = (Z_DELTA < YZ_DELTA_MAX) ? Z_DELTA + PED_ADJUST : Z_DELTA;
      } else if (Z_DELTA - Z_DELTA_TEMP > 175){
        autoCorrectZ();
        Z_DELTA = (Z_DELTA < YZ_DELTA_MAX) ? Z_DELTA + PED_ADJUST : Z_DELTA;
      }
    }
  } else {
    startedSession = true;
  }
}

void resetUpdate() {
  lastX = currX;
  lastY = currY;
  lastZ = currZ;
  validX = false;
  validY = false;
  validZ = false;
}

void update_ui_callback() {
  if ((validX && validY) || (validX && validZ)) {
    pedometerCount++;
    tempTotal++;
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "pedometerCount=%ld, tempTotal=%ld", pedometerCount, tempTotal);
    
    static char buf[] = "123456890abcdefghijkl";
    snprintf(buf, sizeof(buf), "%ld", pedometerCount);
  }

  resetUpdate();
}

static void timer_callback(void *data) {
  AccelData accel = (AccelData ) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);

  if (!startedSession) {
    lastX = accel.x;
    lastY = accel.y;
    lastZ = accel.z;
  } else {
    currX = accel.x;
    currY = accel.y;
    currZ = accel.z;
  }

  pedometer_update();
  update_ui_callback();

  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
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
  text_layer_set_text(text_layer, "Press a button");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
  app_timer_cancel(timer);
  accel_data_service_unsubscribe();
}

static void init(void) {
  tempTotal = totalSteps = persist_exists(TS) ? persist_read_int(TS) : TSD;
  accel_data_service_subscribe(0, NULL);
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

static void deinit(void) {
  totalSteps += pedometerCount;
  persist_write_int(TS, totalSteps);
  accel_data_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
