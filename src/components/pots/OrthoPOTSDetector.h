#pragma once
#include <cstdint>

namespace Pinetime {
  namespace Components {
    // Detects the orthostatic tachycardia response — the diagnostic signature of POTS.
    // Monitors posture via BMA42x Z/Y axis and HR change on standing transition.
    class OrthoPOTSDetector {
    public:
      enum class Posture : uint8_t { Unknown, Supine, Reclined, Upright };

      struct OrthoEvent {
        uint32_t timestamp;
        uint8_t hr_baseline;
        uint8_t hr_peak;
        int8_t  delta;
        bool    flagged; // delta >= 30 bpm (POTS diagnostic threshold)
      };

      static constexpr uint8_t maxHistory = 5;

      // Call every second. z and y in millig units (1024 = 1g).
      void Update(int16_t z, int16_t y, uint8_t current_hr, uint32_t timestamp_s);

      Posture GetCurrentPosture() const;

      // Returns nullptr if no event recorded yet
      const OrthoEvent* GetLastEvent() const;

      // Live HR delta during active observation window (0 if not in window)
      uint8_t GetCurrentOrthoSessionDeltaHR() const;

      bool IsInOrthoWindow() const;

      // Total events recorded ever (monotonically increasing, wraps at 255)
      uint8_t GetTotalEventCount() const { return totalEventCount; }

    private:
      static constexpr uint32_t supineDwellMin = 60;     // seconds before transition counts
      static constexpr uint32_t orthoWindowDuration = 180; // 3-minute observation
      static constexpr uint8_t potsThreshold = 30;         // bpm delta to flag

      // Posture classification thresholds (millig, 1024 = 1g)
      static constexpr int32_t supineZMin = 871;  // 0.85g * 1024
      static constexpr int32_t uprightZMax = 307; // 0.3g * 1024
      static constexpr int32_t uprightYMin = 717; // 0.7g * 1024

      Posture currentPosture = Posture::Unknown;
      uint32_t supineDwellSecs = 0;
      bool inOrthoWindow = false;
      uint32_t orthoWindowStartSec = 0;

      // Baseline HR: rolling average of last N readings while supine
      static constexpr uint8_t hrBaselineWindow = 10;
      uint8_t hrBaselineSamples[hrBaselineWindow] = {};
      uint8_t hrBaselineIndex = 0;
      uint16_t hrBaselineSum = 0;
      uint8_t hrBaselineCount = 0;
      uint8_t hrBaseline = 0;
      uint8_t hrPeak = 0;

      // HR sample every 15s during window (up to 12 samples in 3 min)
      uint32_t lastSampleSec = 0;

      OrthoEvent history[maxHistory] = {};
      uint8_t historyHead = 0;
      uint8_t historyCount = 0;
      uint8_t totalEventCount = 0;

      static int32_t Abs(int32_t v) { return v < 0 ? -v : v; }

      Posture ClassifyPosture(int16_t z, int16_t y) const;
      void RecordBaseline(uint8_t hr);
      void FinalizeOrthoEvent(uint32_t timestamp_s);
    };
  }
}
