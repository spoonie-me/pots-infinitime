#include "components/pots/ActivityClassifier.h"

using namespace Pinetime::Components;

// Alpha-max beta-min magnitude approximation
// Coefficients scaled by 1024 to avoid float: 0.96 → 983, 0.398 → 408
uint32_t ActivityClassifier::ApproxMag(int32_t ax, int32_t ay, int32_t az) {
  int32_t a = Abs(ax);
  int32_t b = Abs(ay);
  int32_t c = Abs(az);
  // Sort: a >= b >= c
  if (a < b) { int32_t t = a; a = b; b = t; }
  if (a < c) { int32_t t = a; a = c; c = t; }
  if (b < c) { int32_t t = b; b = c; c = t; }
  // mag ≈ a*0.96 + b*0.398, scaled back from 1024-fixed
  return static_cast<uint32_t>((a * 983 + b * 408) >> 10);
}

void ActivityClassifier::Update(int16_t x, int16_t y, int16_t z) {
  // BMA421 at ±2g range: 1024 LSB/g → 1 LSB ≈ 1 millig
  uint32_t mag = ApproxMag(x, y, z);
  static constexpr uint32_t oneG = 1024; // millig units
  int32_t dynamic = static_cast<int32_t>(mag) - static_cast<int32_t>(oneG);
  if (dynamic < 0) dynamic = 0;

  windowDynSum += static_cast<uint32_t>(dynamic);
  windowCount++;

  if (windowCount < windowSecs) {
    return;
  }

  uint32_t avgDynamic = windowDynSum / windowSecs;
  windowDynSum = 0;
  windowCount = 0;

  if (avgDynamic < 50) {
    currentLevel = ActivityLevel::Sedentary;
    sedentarySecs += windowSecs;
    consecutiveSedentarySecs += windowSecs;
  } else if (avgDynamic < 150) {
    currentLevel = ActivityLevel::Light;
    lightSecs += windowSecs;
    consecutiveSedentarySecs = 0;
  } else if (avgDynamic < 400) {
    currentLevel = ActivityLevel::Moderate;
    activeSecs += windowSecs;
    consecutiveSedentarySecs = 0;
  } else {
    currentLevel = ActivityLevel::Vigorous;
    activeSecs += windowSecs;
    consecutiveSedentarySecs = 0;
  }
}

void ActivityClassifier::AddUprightSecond() {
  uprightSecs++;
}

void ActivityClassifier::ResetDaily() {
  sedentarySecs = 0;
  lightSecs = 0;
  activeSecs = 0;
  uprightSecs = 0;
  consecutiveSedentarySecs = 0;
  currentLevel = ActivityLevel::Unknown;
}

ActivityClassifier::ActivityLevel ActivityClassifier::GetCurrentLevel() const {
  return currentLevel;
}

uint16_t ActivityClassifier::GetActiveMinutesDay() const {
  return static_cast<uint16_t>(activeSecs / 60);
}

uint16_t ActivityClassifier::GetSedentaryMinutesDay() const {
  return static_cast<uint16_t>(sedentarySecs / 60);
}

uint16_t ActivityClassifier::GetUprightMinutesDay() const {
  return static_cast<uint16_t>(uprightSecs / 60);
}

bool ActivityClassifier::IsSedentaryAlert() const {
  return consecutiveSedentarySecs >= sedentaryAlertSecs;
}
