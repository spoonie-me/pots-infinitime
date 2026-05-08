#include "components/pots/OrthoPOTSDetector.h"

using namespace Pinetime::Components;

OrthoPOTSDetector::Posture OrthoPOTSDetector::ClassifyPosture(int16_t z, int16_t y) const {
  int32_t absZ = Abs(static_cast<int32_t>(z));
  int32_t absY = Abs(static_cast<int32_t>(y));

  if (absZ >= supineZMin) {
    return Posture::Supine;
  }
  if (absZ <= uprightZMax && absY >= uprightYMin) {
    return Posture::Upright;
  }
  return Posture::Unknown;
}

void OrthoPOTSDetector::RecordBaseline(uint8_t hr) {
  if (hr < 30 || hr > 220) return; // reject invalid HR
  if (hrBaselineCount < 10) {
    hrBaselineSum += hr;
    hrBaselineCount++;
    if (hrBaselineCount > 0) {
      hrBaseline = static_cast<uint8_t>(hrBaselineSum / hrBaselineCount);
    }
  } else {
    // Rolling: drop oldest (approximate — we don't track individual values)
    hrBaselineSum = hrBaselineSum - hrBaseline + hr;
    hrBaseline = static_cast<uint8_t>(hrBaselineSum / 10);
  }
}

void OrthoPOTSDetector::FinalizeOrthoEvent(uint32_t timestamp_s) {
  OrthoEvent evt;
  evt.timestamp = timestamp_s;
  evt.hr_baseline = hrBaseline;
  evt.hr_peak = hrPeak;
  int32_t delta = static_cast<int32_t>(hrPeak) - static_cast<int32_t>(hrBaseline);
  evt.delta = static_cast<int8_t>(delta > 127 ? 127 : (delta < -128 ? -128 : delta));
  evt.flagged = (delta >= potsThreshold);

  history[historyHead] = evt;
  historyHead = (historyHead + 1) % maxHistory;
  if (historyCount < maxHistory) historyCount++;
}

void OrthoPOTSDetector::Update(int16_t z, int16_t y, uint8_t current_hr, uint32_t timestamp_s) {
  Posture newPosture = ClassifyPosture(z, y);

  if (inOrthoWindow) {
    // Track peak HR during window
    if (current_hr > hrPeak && current_hr > 30) {
      hrPeak = current_hr;
    }
    // Sample every 15 seconds
    if (timestamp_s - lastSampleSec >= 15) {
      lastSampleSec = timestamp_s;
    }
    // End window after duration
    if (timestamp_s - orthoWindowStartSec >= orthoWindowDuration) {
      FinalizeOrthoEvent(timestamp_s);
      inOrthoWindow = false;
      supineDwellSecs = 0;
    }
    currentPosture = newPosture;
    return;
  }

  // State machine outside observation window
  if (newPosture == Posture::Supine) {
    if (currentPosture == Posture::Supine) {
      supineDwellSecs++;
      RecordBaseline(current_hr);
    } else {
      // Freshly went supine
      supineDwellSecs = 1;
      hrBaselineCount = 0;
      hrBaselineSum = 0;
      RecordBaseline(current_hr);
    }
  } else if (newPosture == Posture::Upright) {
    if (currentPosture == Posture::Supine && supineDwellSecs >= supineDwellMin && hrBaseline > 30) {
      // Standing transition detected — start ortho window
      inOrthoWindow = true;
      orthoWindowStartSec = timestamp_s;
      lastSampleSec = timestamp_s;
      hrPeak = current_hr;
    } else if (currentPosture != Posture::Supine) {
      supineDwellSecs = 0;
    }
  } else {
    // Unknown/transition — don't reset supine dwell, just don't accumulate
  }

  currentPosture = newPosture;
}

OrthoPOTSDetector::Posture OrthoPOTSDetector::GetCurrentPosture() const {
  return currentPosture;
}

const OrthoPOTSDetector::OrthoEvent* OrthoPOTSDetector::GetLastEvent() const {
  if (historyCount == 0) return nullptr;
  uint8_t lastIdx = (historyHead + maxHistory - 1) % maxHistory;
  return &history[lastIdx];
}

uint8_t OrthoPOTSDetector::GetCurrentOrthoSessionDeltaHR() const {
  if (!inOrthoWindow || hrBaseline == 0) return 0;
  int32_t delta = static_cast<int32_t>(hrPeak) - static_cast<int32_t>(hrBaseline);
  if (delta < 0) return 0;
  return static_cast<uint8_t>(delta > 99 ? 99 : delta);
}

bool OrthoPOTSDetector::IsInOrthoWindow() const {
  return inOrthoWindow;
}
