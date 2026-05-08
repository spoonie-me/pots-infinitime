#pragma once
#include <cstdint>

namespace Pinetime {
  namespace Components {
    class ActivityClassifier {
    public:
      enum class ActivityLevel : uint8_t { Unknown, Sedentary, Light, Moderate, Vigorous };

      // Call every second with raw accelerometer values (units: 1024 = 1g)
      void Update(int16_t x, int16_t y, int16_t z);

      // Call at midnight to reset daily counters
      void ResetDaily();

      // Feed upright time from OrthoPOTSDetector
      void AddUprightSecond();

      ActivityLevel GetCurrentLevel() const;
      uint16_t GetActiveMinutesDay() const;
      uint16_t GetSedentaryMinutesDay() const;
      uint16_t GetUprightMinutesDay() const;
      bool IsSedentaryAlert() const;

    private:
      static constexpr uint8_t windowSecs = 30;

      // Sample accumulator for 30-second window
      uint32_t windowDynSum = 0; // sum of dynamic magnitudes in window (millig units)
      uint8_t windowCount = 0;

      ActivityLevel currentLevel = ActivityLevel::Unknown;

      // Daily totals (seconds, converted to minutes on read)
      uint32_t sedentarySecs = 0;
      uint32_t lightSecs = 0;
      uint32_t activeSecs = 0;
      uint32_t uprightSecs = 0;

      // Sedentary streak for alert
      uint32_t consecutiveSedentarySecs = 0;
      static constexpr uint32_t sedentaryAlertSecs = 45 * 60; // 45 minutes

      // Integer absolute value helper
      static int32_t Abs(int32_t v) { return v < 0 ? -v : v; }

      // Fast magnitude approximation: max*0.96 + min*0.398 (alpha-max beta-min)
      // Input units: millig (1024 = 1g). Returns millig.
      static uint32_t ApproxMag(int32_t ax, int32_t ay, int32_t az);
    };
  }
}
