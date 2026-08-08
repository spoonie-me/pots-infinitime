#pragma once
#define min
#define max
#include <host/ble_gap.h>
#undef max
#undef min
#include <atomic>
#include <cstdint>

// Custom 128-bit UUIDs for the POTS Data Service
// Generated from namespace: pots-companion-infinitime
// Service:     A7F3xxxx-5E4D-4B8C-9F2A-1D3E6C8B0F12
// HRV char:    A7F30001-5E4D-4B8C-9F2A-1D3E6C8B0F12
// Ortho char:  A7F30002-5E4D-4B8C-9F2A-1D3E6C8B0F12
// Daily char:  A7F30003-5E4D-4B8C-9F2A-1D3E6C8B0F12

namespace Pinetime {
  namespace Controllers {
    class NimbleController;

    class POTSDataService {
    public:
      explicit POTSDataService(NimbleController& nimble);
      void Init();

      // Called by WatchFacePOTS when HRV recalculates
      void NotifyHrv(uint8_t rmssd, uint8_t zone); // zone: 0=unknown,1=low,2=mod,3=good

      // Called by WatchFacePOTS when an ortho event finalises
      void NotifyOrthoEvent(uint32_t timestamp, uint8_t hrBaseline, uint8_t hrPeak, int8_t delta, bool flagged);

      // Called every second so daily stats stay readable
      void UpdateDailyStats(uint16_t activeMins, uint16_t sedentaryMins, uint16_t uprightMins);

      int OnAccessRequest(uint16_t attributeHandle, ble_gatt_access_ctxt* context);
      void SubscribeNotification(uint16_t attributeHandle);
      void UnsubscribeNotification(uint16_t attributeHandle);

    private:
      NimbleController& nimble;

      // --- Service UUID ---
      static constexpr ble_uuid128_t serviceUuid = {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0x12, 0x0F, 0x8B, 0x6C, 0x3E, 0x1D, 0x2A, 0x9F,
                  0x8C, 0x4B, 0x4D, 0x5E, 0x00, 0x00, 0xF3, 0xA7}
      };

      // --- HRV characteristic UUID ---
      static constexpr ble_uuid128_t hrvCharUuid = {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0x12, 0x0F, 0x8B, 0x6C, 0x3E, 0x1D, 0x2A, 0x9F,
                  0x8C, 0x4B, 0x4D, 0x5E, 0x01, 0x00, 0xF3, 0xA7}
      };

      // --- Ortho event characteristic UUID ---
      static constexpr ble_uuid128_t orthoCharUuid = {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0x12, 0x0F, 0x8B, 0x6C, 0x3E, 0x1D, 0x2A, 0x9F,
                  0x8C, 0x4B, 0x4D, 0x5E, 0x02, 0x00, 0xF3, 0xA7}
      };

      // --- Daily stats characteristic UUID ---
      static constexpr ble_uuid128_t dailyCharUuid = {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0x12, 0x0F, 0x8B, 0x6C, 0x3E, 0x1D, 0x2A, 0x9F,
                  0x8C, 0x4B, 0x4D, 0x5E, 0x03, 0x00, 0xF3, 0xA7}
      };

      struct ble_gatt_chr_def characteristicDefinition[4];
      struct ble_gatt_svc_def serviceDefinition[2];

      uint16_t hrvHandle = 0;
      uint16_t orthoHandle = 0;
      uint16_t dailyHandle = 0;

      std::atomic_bool hrvNotifyEnabled {false};
      std::atomic_bool orthoNotifyEnabled {false};

      // Cached data for READ access: [rmssd, zone, activeMins(2LE), sedMins(2LE), uprightMins(2LE)]
      uint8_t dailyBuf[8] = {};

      // Cached last ortho event for READ access: [timestamp(4LE), baseline, peak, delta(signed), flagged]
      uint8_t orthoBuf[8] = {};
    };
  }
}
