#include "displayapp/screens/WatchFacePOTS.h"

#include <lvgl/lvgl.h>
#include <cstdio>

#include "components/datetime/DateTimeController.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"

using namespace Pinetime::Applications::Screens;

// ------------------------------------------------------------
// Helper: create a static arc ring
// ------------------------------------------------------------
static lv_obj_t* MakeRing(lv_color_t color, int16_t x, int16_t y) {
  lv_obj_t* arc = lv_arc_create(lv_scr_act(), nullptr);
  lv_obj_set_size(arc, 56, 56);
  lv_arc_set_rotation(arc, 270);      // start from top
  lv_arc_set_bg_angles(arc, 0, 360);  // full circle background
  lv_arc_set_adjustable(arc, false);
  lv_arc_set_range(arc, 0, 100);
  lv_arc_set_value(arc, 0);
  lv_obj_set_style_local_line_color(arc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, color);
  lv_obj_set_style_local_line_width(arc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, 7);
  lv_obj_set_style_local_line_color(arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, lv_color_hex(0x2A2A2A));
  lv_obj_set_style_local_line_width(arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_bg_opa(arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  lv_obj_align(arc, nullptr, LV_ALIGN_IN_BOTTOM_LEFT, x, y);
  return arc;
}

static void SetRingValue(lv_obj_t* arc, uint8_t pct) {
  lv_arc_set_value(arc, pct); // range is 0–100
}

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
WatchFacePOTS::WatchFacePOTS(Controllers::DateTime& dateTimeController,
                              Controllers::HeartRateController& heartRateController,
                              Controllers::MotionController& motionController)
  : dateTimeController {dateTimeController},
    heartRateController {heartRateController},
    motionController {motionController} {

  // Dark background
  lv_obj_set_style_local_bg_color(lv_scr_act(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_BG));

  // --- Time label (top-left, medium font) ---
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_WHITE));
  lv_obj_align(label_time, nullptr, LV_ALIGN_IN_TOP_LEFT, 4, 4);
  lv_label_set_text_static(label_time, "00:00");

  // --- Date label (top-right) ---
  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_MUTED));
  lv_obj_align(label_date, nullptr, LV_ALIGN_IN_TOP_RIGHT, -4, 4);
  lv_label_set_text_static(label_date, "01 Jan");

  // --- HR large label (centre-left) ---
  label_hr = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_hr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_WHITE));
  lv_obj_set_style_local_text_font(label_hr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_align(label_hr, nullptr, LV_ALIGN_CENTER, -50, -30);
  lv_label_set_text_static(label_hr, "--");

  // --- "bpm" unit ---
  label_hr_unit = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_hr_unit, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_MUTED));
  lv_obj_align(label_hr_unit, label_hr, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
  lv_label_set_text_static(label_hr_unit, "bpm");

  // --- Ortho delta label (hidden until window opens) ---
  label_delta = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_delta, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_AMBER));
  lv_obj_align(label_delta, label_hr, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
  lv_label_set_text_static(label_delta, "");

  // --- HRV zone label ---
  label_hrv_text = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_hrv_text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_MUTED));
  lv_obj_align(label_hrv_text, nullptr, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text_static(label_hrv_text, "HRV: ---");

  // --- Posture label ---
  label_posture = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_posture, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_MUTED));
  lv_obj_align(label_posture, nullptr, LV_ALIGN_CENTER, 60, 0);
  lv_label_set_text_static(label_posture, "?");

  // --- Three activity rings (bottom row, equally spaced) ---
  // Ring 1: Active minutes — teal, goal 30
  arc_active = MakeRing(lv_color_hex(COL_TEAL), 10, -20);

  // Ring 2: Sedentary inverse — coral, goal stay under 480 min
  arc_sedentary = MakeRing(lv_color_hex(COL_CORAL), 92, -20);

  // Ring 3: Upright minutes — purple, goal 60
  arc_upright = MakeRing(lv_color_hex(COL_PURPLE), 174, -20);

  // --- Steps label (bottom) ---
  label_steps = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_steps, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_MUTED));
  lv_obj_align(label_steps, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -4);
  lv_label_set_text_static(label_steps, "steps: 0");

  taskRefresh = lv_task_create(RefreshTaskCallback, 1000 /* 1 Hz */, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFacePOTS::~WatchFacePOTS() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

// ------------------------------------------------------------
// Refresh — called every second
// ------------------------------------------------------------
void WatchFacePOTS::Refresh() {
  secondCounter++;

  // --- Time ---
  uint8_t hour   = dateTimeController.Hours();
  uint8_t minute = dateTimeController.Minutes();
  uint8_t day    = dateTimeController.Day();
  uint8_t month  = static_cast<uint8_t>(dateTimeController.Month());
  uint16_t dayStamp = static_cast<uint16_t>(day) * 100 + month;

  lv_label_set_text_fmt(label_time, "%02d:%02d", hour, minute);
  lv_label_set_text_fmt(label_date, "%02d %s", day, dateTimeController.MonthShortToString());

  // Daily reset at midnight
  if (dayStamp != lastDayStamp && lastDayStamp != 0) {
    activityClassifier.ResetDaily();
  }
  lastDayStamp = dayStamp;

  // --- Heart rate & HRV IBI feeding ---
  bool hrRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  uint8_t hr = hrRunning ? heartRateController.HeartRate() : 0;

  if (hr > 30 && hr < 220) {
    lastHr = hr;
    // Feed an IBI estimate every ~15 seconds (HR changes slowly)
    if (secondCounter - lastHrIbiSec >= 15) {
      // IBI (ms) ≈ 60000 / HR
      // Use integer division; avoid division by zero
      uint16_t ibi = static_cast<uint16_t>(60000u / static_cast<uint32_t>(hr));
      hrvCalc.AddIbi(ibi);
      lastHrIbiSec = secondCounter;
    }
  }

  // --- Accelerometer update ---
  int16_t ax = motionController.X();
  int16_t ay = motionController.Y();
  int16_t az = motionController.Z();

  orthoDetector.Update(az, ay, lastHr, secondCounter);
  activityClassifier.Update(ax, ay, az);

  if (orthoDetector.GetCurrentPosture() == Components::OrthoPOTSDetector::Posture::Upright) {
    activityClassifier.AddUprightSecond();
  }

  // --- Update HR display ---
  UpdateHrDisplay(lastHr);

  // --- Update HRV ---
  UpdateHrvDisplay();

  // --- Update posture ---
  UpdatePostureLabel();

  // --- Update rings ---
  UpdateActivityRings();

  // --- Steps ---
  uint32_t steps = motionController.NbSteps();
  lv_label_set_text_fmt(label_steps, "steps: %lu", steps);
}

// ------------------------------------------------------------
// HR display with colour coding
// ------------------------------------------------------------
void WatchFacePOTS::UpdateHrDisplay(uint8_t hr) {
  if (hr < 30) {
    lv_label_set_text_static(label_hr, "--");
    lv_obj_set_style_local_text_color(label_hr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_MUTED));
    lv_label_set_text_static(label_delta, "");
    return;
  }

  lv_label_set_text_fmt(label_hr, "%d", hr);

  if (orthoDetector.IsInOrthoWindow()) {
    uint8_t delta = orthoDetector.GetCurrentOrthoSessionDeltaHR();

    if (delta >= 30) {
      // Flagged POTS response
      lv_obj_set_style_local_text_color(label_hr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_RED));
      lv_obj_set_style_local_text_color(label_delta, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_RED));
    } else {
      lv_obj_set_style_local_text_color(label_hr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_AMBER));
      lv_obj_set_style_local_text_color(label_delta, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_AMBER));
    }
    lv_label_set_text_fmt(label_delta, "+%d \xce\x94", delta); // +NN Δ
  } else {
    lv_obj_set_style_local_text_color(label_hr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_WHITE));
    lv_label_set_text_static(label_delta, "");
  }
}

// ------------------------------------------------------------
// HRV zone display
// ------------------------------------------------------------
void WatchFacePOTS::UpdateHrvDisplay() {
  if (!hrvCalc.HasValidReading()) {
    lv_label_set_text_static(label_hrv_text, "HRV: ---");
    lv_obj_set_style_local_text_color(label_hrv_text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_MUTED));
    return;
  }

  switch (hrvCalc.GetZone()) {
    case Components::HrvCalculator::HrvZone::Low:
      lv_label_set_text_static(label_hrv_text, "HRV: LOW");
      lv_obj_set_style_local_text_color(label_hrv_text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_HRV_LOW));
      break;
    case Components::HrvCalculator::HrvZone::Moderate:
      lv_label_set_text_static(label_hrv_text, "HRV: MOD");
      lv_obj_set_style_local_text_color(label_hrv_text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_HRV_MOD));
      break;
    case Components::HrvCalculator::HrvZone::Good:
      lv_label_set_text_static(label_hrv_text, "HRV: GOOD");
      lv_obj_set_style_local_text_color(label_hrv_text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_HRV_GOOD));
      break;
    default:
      lv_label_set_text_static(label_hrv_text, "HRV: ---");
      lv_obj_set_style_local_text_color(label_hrv_text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(COL_MUTED));
      break;
  }
}

// ------------------------------------------------------------
// Posture label
// ------------------------------------------------------------
void WatchFacePOTS::UpdatePostureLabel() {
  switch (orthoDetector.GetCurrentPosture()) {
    case Components::OrthoPOTSDetector::Posture::Supine:
      lv_label_set_text_static(label_posture, "LYING");
      break;
    case Components::OrthoPOTSDetector::Posture::Upright:
      lv_label_set_text_static(label_posture, "UPRIGHT");
      break;
    case Components::OrthoPOTSDetector::Posture::Reclined:
      lv_label_set_text_static(label_posture, "RECLINED");
      break;
    default:
      lv_label_set_text_static(label_posture, "?");
      break;
  }
}

// ------------------------------------------------------------
// Activity rings
// ------------------------------------------------------------
void WatchFacePOTS::UpdateActivityRings() {
  uint16_t activeMins     = activityClassifier.GetActiveMinutesDay();
  uint16_t sedentaryMins  = activityClassifier.GetSedentaryMinutesDay();
  uint16_t uprightMins    = activityClassifier.GetUprightMinutesDay();

  // Ring 1: active minutes, goal 30 min
  uint8_t activePct = Clamp100((static_cast<uint32_t>(activeMins) * 100) / 30);
  SetRingValue(arc_active, activePct);

  // Ring 2: sedentary inverse, goal stay under 480 min (8 hours)
  // Ring is full when sedentaryMins == 0; drains as you sit
  uint32_t sedPct100 = (static_cast<uint32_t>(sedentaryMins) * 100) / 480;
  uint8_t sedInverse = (sedPct100 >= 100) ? 0 : static_cast<uint8_t>(100 - sedPct100);
  SetRingValue(arc_sedentary, sedInverse);

  // Change sedentary ring to coral/alert color when alert fires
  if (activityClassifier.IsSedentaryAlert()) {
    lv_obj_set_style_local_line_color(arc_sedentary, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(COL_RED));
  } else {
    lv_obj_set_style_local_line_color(arc_sedentary, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(COL_CORAL));
  }

  // Ring 3: upright minutes, goal 60 min for POTS
  uint8_t uprightPct = Clamp100((static_cast<uint32_t>(uprightMins) * 100) / 60);
  SetRingValue(arc_upright, uprightPct);
}
