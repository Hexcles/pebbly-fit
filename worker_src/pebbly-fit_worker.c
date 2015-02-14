#include <pebble_worker.h>
#include <math.h>

#define TS 1

static AppTimer *timer;
static AppTimer *timer1;

const int ACCEL_STEP_MS = 475;
const int PED_ADJUST = 2;
const int WAKEUP_INTERVAL = 10000;

int X_DELTA = 45;
int Y_DELTA, Z_DELTA = 235;
int YZ_DELTA_MIN = 225;
int YZ_DELTA_MAX = 245; 
int X_DELTA_TEMP, Y_DELTA_TEMP, Z_DELTA_TEMP = 0;
int lastX, lastY, lastZ, currX, currY, currZ = 0;

uint16_t pedometerCount = 0;

bool validX, validY, validZ = false;
bool startedSession = false;

static uint16_t totalSteps = 1;

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

static void send_to_foreground() {
	AppWorkerMessage msg_data = {.data0 = totalSteps};
	app_worker_send_message(TS, &msg_data);
}

void update_callback(){
	if ((validX && validY) || (validX && validZ)) {
		pedometerCount++;
		totalSteps += pedometerCount; 
		pedometerCount = 0;
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
  update_callback();

  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

static void timer_callback1(){
	worker_launch_app();
	psleep(1000);
	send_to_foreground();

	timer1 = app_timer_register(WAKEUP_INTERVAL, timer_callback1, NULL);
}

static void worker_init(){
	totalSteps = 1;
	accel_data_service_subscribe(0, NULL);
	timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
	timer1 = app_timer_register(WAKEUP_INTERVAL, timer_callback1, NULL);
}

static void worker_deinit(){
	app_timer_cancel(timer);
	accel_data_service_unsubscribe();
}

int main(void) {
	worker_init();
	worker_event_loop();
	worker_deinit();
}
