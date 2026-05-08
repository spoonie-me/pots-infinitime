#include "components/pots/HrvCalculator.h"

using namespace Pinetime::Components;

void HrvCalculator::AddIbi(uint16_t ibi_ms) {
  if (ibi_ms < 300 || ibi_ms > 2000) {
    return; // reject physiologically impossible values
  }
  ibiBuf[bufHead] = ibi_ms;
  bufHead = (bufHead + 1) % bufSize;
  if (bufCount < bufSize) {
    bufCount++;
  }
  if (bufCount >= bufSize) {
    Recalculate();
  }
}

uint8_t HrvCalculator::GetRmssd() const {
  return rmssd;
}

HrvCalculator::HrvZone HrvCalculator::GetZone() const {
  return zone;
}

bool HrvCalculator::HasValidReading() const {
  return valid;
}

// Fast integer square root (Newton's method)
uint8_t HrvCalculator::IntSqrt(uint32_t n) {
  if (n == 0) return 0;
  uint32_t x = n;
  uint32_t y = (x + 1) >> 1;
  while (y < x) {
    x = y;
    y = (x + n / x) >> 1;
  }
  return static_cast<uint8_t>(x > 255 ? 255 : x);
}

void HrvCalculator::Recalculate() {
  uint32_t sumSqDiff = 0;
  // Walk the circular buffer in order
  for (uint8_t i = 0; i < bufSize - 1; i++) {
    uint8_t idxA = (bufHead + i) % bufSize;
    uint8_t idxB = (bufHead + i + 1) % bufSize;
    int32_t diff = static_cast<int32_t>(ibiBuf[idxB]) - static_cast<int32_t>(ibiBuf[idxA]);
    sumSqDiff += static_cast<uint32_t>(diff * diff);
  }
  uint32_t rmssdSq = sumSqDiff / (bufSize - 1);
  rmssd = IntSqrt(rmssdSq);
  valid = true;

  // Thresholds calibrated for HR-derived IBI proxy (not true R-R intervals).
  // 1 bpm change at 70 bpm → ~12ms IBI delta → proxy RMSSD of ~8ms.
  if (rmssd < 5) {
    zone = HrvZone::Low;
  } else if (rmssd <= 15) {
    zone = HrvZone::Moderate;
  } else {
    zone = HrvZone::Good;
  }
}
