#pragma once
#include <cstdint>

namespace Pinetime {
  namespace Components {
    // Computes a simplified HRV proxy (RMSSD) from successive IBI readings.
    // IBI values can come from PPG-derived estimates (60000/HR_bpm).
    // Label all UI output "HRV proxy" — not clinical HRV.
    class HrvCalculator {
    public:
      enum class HrvZone : uint8_t { Unknown, Low, Moderate, Good };

      void AddIbi(uint16_t ibi_ms);
      uint8_t GetRmssd() const;
      HrvZone GetZone() const;
      bool HasValidReading() const;

    private:
      static constexpr uint8_t bufSize = 6;
      uint16_t ibiBuf[bufSize] = {};
      uint8_t bufHead = 0;
      uint8_t bufCount = 0;
      uint8_t rmssd = 0;
      HrvZone zone = HrvZone::Unknown;
      bool valid = false;

      static uint8_t IntSqrt(uint32_t n);
      void Recalculate();
    };
  }
}
