#include <pebble.h>

static Window *window;
static Layer *s_simple_bg_layer, *s_simple_timer_layer;
static TextLayer *text_layer;

static int32_t s_pos = -1;
static char *s_names[20];

static int s_index = -1;
static int32_t s_warnTime = 60;
static int s_times[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static GColor s_boundColours[20];
static time_t s_timerStart;
static char *s_timeText;

///CALC SECTION

static void loadData() {
  s_pos = persist_read_int(0);
  for (int i = 0; i < 20; i++) {
    s_names[i] = (char*)malloc(21 * sizeof(char));
    int t = persist_read_string(i + 1, s_names[i], 21);
    if (t == E_DOES_NOT_EXIST) {
      s_names[i] = "\0";
    }
  }
  s_warnTime = persist_read_int(22);
  if (s_warnTime < 60 || s_warnTime > 16*60) s_warnTime = 60;
}

static void saveData() {
  if (s_pos < 0) return;
  persist_write_int(0, s_pos);
  int offset = 1;
  for (int i = 0; i < 20; i++) {
    //persist_delete(i + 1); // Clear out old data (note: does not use offset)
    persist_write_string(i + 1, "\0"); // Clear out old data (note: does not use offset)
    if (!s_names[i] || s_names[i][0] == '\0') {
      //Debubble
      offset--;
      continue;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saving: i = %d, s_names = %s", i, s_names[i]);
    persist_write_string(i + offset, s_names[i]);
  }
  persist_write_int(22, s_warnTime);
}

static void shuffle() {
  //Do the shuffle! (doot doot doot doot doot doo-doot doot doot!)
  int count = 0;
  int keepCount = 0;
  for (int i = 0; i < 20; i++) {
    if (s_names[i] && s_names[i][0] != '\0') {
      count++;
      if (s_pos >= 0 && i >= s_pos) keepCount++;
    }
  }
  if (count < 3 || (s_pos != -1 && (keepCount > count / 3))) return;

  char *sortarr[count - keepCount];
  int sai = 0;
  int i = 0;
  for (; i < 20 && sai < count - keepCount; i++) {
    if (s_names[i] && s_names[i][0] != '\0') {
      sortarr[sai] = s_names[i];
      sai++;
    }
  }
  int nai = 0;
  for (; i < 20 && nai < keepCount; i++) {
    if (s_names[i] && s_names[i][0] != '\0') {
      s_names[nai] = s_names[i];
      nai++;
    }
  }
  for (int j = nai; j < 20; j++) {
    s_names[j] = "";
  }
  for (int j = nai; j < count; j++) {
    //random element
    int p = rand() % (count - keepCount);
    while (!sortarr[p] || sortarr[p][0] == '\0') p = (p + 1) % (count - keepCount);
    s_names[nai] = sortarr[p];
    sortarr[p] = "";
    nai++;
  }
}

static void showPerson() {
  int32_t lastPos = (s_pos + 19) % 20;
  while ((!s_names[s_pos] || s_names[s_pos][0] == '\0') && s_pos != lastPos) {
    s_pos = (s_pos + 1) % 20;
  }
  text_layer_set_text(text_layer, s_names[s_pos]);
}

static void nextPerson() {
  s_pos = (s_pos + 1) % 20;
  showPerson();
  shuffle();
  saveData();
}

static int getTime() {
  int duration = 0;
  int curTime = 0;
  if (s_index >= 0 && s_index < 20) {
    curTime = s_times[s_index];
  }
  if (s_timerStart) {
    duration = time(NULL) - s_timerStart;
  }
  return curTime + duration;
}

static void restartTimer() {
  s_timerStart = time(NULL);
}

static void stopTimer() {
  s_timerStart = 0;
}

///DRAW SECTION

static void bg_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint centre = grect_center_point(&bounds);

  //Background colour
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, centre, (bounds.size.w / 2) - 20);
}

static void timer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint centre = grect_center_point(&bounds);
  GRect outer_frame = grect_inset(bounds, GEdgeInsets(1));
  GRect inner_frame = grect_inset(bounds, GEdgeInsets(11));

  int tots = 0;
  for (int i = 0; i < 20; i++) {
    int t = (i == s_index) ? getTime() : s_times[i];
    tots += t;
  }
  int32_t start_angle = 0;
  for (int i = 0; i < 20; i++) {
    int t = (i == s_index) ? getTime() : s_times[i];
    if (t == 0) continue;
    int32_t end_angle = start_angle + TRIG_MAX_ANGLE * ((t * 1.) / (tots * 1.));
    graphics_context_set_fill_color(ctx, s_boundColours[i]);
    graphics_fill_radial(ctx, outer_frame, GOvalScaleModeFitCircle, 7, start_angle, end_angle);
    start_angle = end_angle;
  }

  int time = getTime();
  int32_t end_angle;
  if (time >= s_warnTime) {
    end_angle = TRIG_MAX_ANGLE;
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorLightGray));
    graphics_fill_radial(ctx, inner_frame, GOvalScaleModeFitCircle, 5, 0, end_angle);
  }
  end_angle = TRIG_MAX_ANGLE * ((time % s_warnTime * 1.) / (s_warnTime * 1.));
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_radial(ctx, inner_frame, GOvalScaleModeFitCircle, 5, 0, end_angle);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  GPoint divStart = {
    .x = centre.x,
    .y = 0,
  };
  GPoint divEnd = {
    .x = centre.x,
    .y = 19,
  };

  graphics_draw_line(ctx, divStart, divEnd);

  if (s_timerStart) {
    if (time > 3599) time = time / 60;
    if (time > 5999) time = 5999;
    s_timeText = " -:--\0";
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
    int m = time / 60;
    int s = time % 60;
    if (m >= 10) s_timeText[0] = (m / 10) + '0';
    s_timeText[1] = (m % 10) + '0';
    s_timeText[3] = (s / 10) + '0';
    s_timeText[4] = (s % 10) + '0';
    text_layer_set_text(text_layer, s_timeText);
  }
}

///CLICK SECTION

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_timerStart) {
    //Chosen person not here
    nextPerson();
  } else {
    //Pause timer
    if (s_index < 0 || s_index == 20) return;
    s_times[s_index] = getTime();
    stopTimer();
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  //Reset timer
  if (s_index < 0 || s_index == 20) return;

  s_times[s_index] = 0;
  if (s_timerStart) restartTimer();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  //Next person
  if (s_index == 20) return;

  if (s_index >= 0 && s_times[s_index] > 0 && !s_timerStart) {
    restartTimer();
  } else {
    if (s_index >= 0) s_times[s_index] = getTime();
    s_index++;
    if (s_index == 20) {
      stopTimer();
    } else {
      restartTimer();
    }
  }
}

/// MESSAGE SECTION

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *warnTime_t = dict_find(iter, MESSAGE_KEY_WarnTime);
  if(warnTime_t) {
    s_warnTime = (warnTime_t->value->int32 % 16) * 60;
  }
  
  for (int i = 0; i < 20; i++) {
    Tuple *peep_t = dict_find(iter, MESSAGE_KEY_B01 + i);
    if(peep_t) {
      s_names[i] = peep_t->value->cstring;
    }
  }
  s_pos = -1;
  shuffle();
  saveData();
}

///INIT SECTION

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_simple_timer_layer);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_simple_timer_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_timer_layer, timer_update_proc);
  layer_add_child(window_layer, s_simple_timer_layer);

  text_layer = text_layer_create(GRect(25, 72, bounds.size.w - 50, 30));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  loadData();
  showPerson();
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_simple_timer_layer);
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
  window_stack_push(window, animated);
  for (int i = 0; i < 20; i++) {
    if (PBL_IF_COLOR_ELSE(true, false)) {
      if (i % 4 == 0) s_boundColours[i] = GColorVeryLightBlue;
      if (i % 4 == 1) s_boundColours[i] = GColorInchworm;
      if (i % 4 == 2) s_boundColours[i] = GColorSunsetOrange;
      if (i % 4 == 3) s_boundColours[i] = GColorLavenderIndigo;
    } else {
      s_boundColours[i] = (i % 2 == 0) ? GColorWhite : GColorLightGray;
    }
  }
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(512, 512);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  app_event_loop();
  deinit();
}