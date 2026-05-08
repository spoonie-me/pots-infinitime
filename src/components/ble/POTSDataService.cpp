#include "components/ble/POTSDataService.h"
#include "components/ble/NimbleController.h"
#include <nrf_log.h>

using namespace Pinetime::Controllers;

constexpr ble_uuid128_t POTSDataService::serviceUuid;
constexpr ble_uuid128_t POTSDataService::hrvCharUuid;
constexpr ble_uuid128_t POTSDataService::orthoCharUuid;
constexpr ble_uuid128_t POTSDataService::dailyCharUuid;

namespace {
  int POTSDataServiceCallback(uint16_t /*conn_handle*/, uint16_t attr_handle, ble_gatt_access_ctxt* ctxt, void* arg) {
    auto* svc = static_cast<POTSDataService*>(arg);
    return svc->OnAccessRequest(attr_handle, ctxt);
  }
}

POTSDataService::POTSDataService(NimbleController& nimble)
  : nimble {nimble},
    characteristicDefinition {{.uuid = &hrvCharUuid.u,
                               .access_cb = POTSDataServiceCallback,
                               .arg = this,
                               .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                               .val_handle = &hrvHandle},
                              {.uuid = &orthoCharUuid.u,
                               .access_cb = POTSDataServiceCallback,
                               .arg = this,
                               .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                               .val_handle = &orthoHandle},
                              {.uuid = &dailyCharUuid.u,
                               .access_cb = POTSDataServiceCallback,
                               .arg = this,
                               .flags = BLE_GATT_CHR_F_READ,
                               .val_handle = &dailyHandle},
                              {0}},
    serviceDefinition {{.type = BLE_GATT_SVC_TYPE_PRIMARY,
                        .uuid = &serviceUuid.u,
                        .characteristics = characteristicDefinition},
                       {0}} {
}

void POTSDataService::Init() {
  int res = ble_gatts_count_cfg(serviceDefinition);
  ASSERT(res == 0);
  res = ble_gatts_add_svcs(serviceDefinition);
  ASSERT(res == 0);
}

int POTSDataService::OnAccessRequest(uint16_t attributeHandle, ble_gatt_access_ctxt* context) {
  if (attributeHandle == hrvHandle) {
    // Return last HRV reading: [rmssd, zone]
    uint8_t buf[2] = {dailyBuf[0], dailyBuf[1]};
    int res = os_mbuf_append(context->om, buf, 2);
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  if (attributeHandle == dailyHandle) {
    int res = os_mbuf_append(context->om, dailyBuf + 2, 6);
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  return 0;
}

void POTSDataService::NotifyHrv(uint8_t rmssd, uint8_t zone) {
  // Cache for READ
  dailyBuf[0] = rmssd;
  dailyBuf[1] = zone;

  if (!hrvNotifyEnabled) return;

  uint16_t connHandle = nimble.connHandle();
  if (connHandle == 0 || connHandle == BLE_HS_CONN_HANDLE_NONE) return;

  uint8_t buf[2] = {rmssd, zone};
  auto* om = ble_hs_mbuf_from_flat(buf, sizeof(buf));
  ble_gattc_notify_custom(connHandle, hrvHandle, om);
}

void POTSDataService::NotifyOrthoEvent(uint32_t timestamp, uint8_t hrBaseline, uint8_t hrPeak, int8_t delta, bool flagged) {
  // Always log to nRF log for debugging
  NRF_LOG_INFO("POTS ortho: base=%d peak=%d delta=%d flagged=%d", hrBaseline, hrPeak, delta, flagged ? 1 : 0);

  if (!orthoNotifyEnabled) return;

  uint16_t connHandle = nimble.connHandle();
  if (connHandle == 0 || connHandle == BLE_HS_CONN_HANDLE_NONE) return;

  // Packet: [timestamp(4LE), baseline, peak, delta(signed), flagged]
  uint8_t buf[8];
  buf[0] = static_cast<uint8_t>(timestamp & 0xFF);
  buf[1] = static_cast<uint8_t>((timestamp >> 8) & 0xFF);
  buf[2] = static_cast<uint8_t>((timestamp >> 16) & 0xFF);
  buf[3] = static_cast<uint8_t>((timestamp >> 24) & 0xFF);
  buf[4] = hrBaseline;
  buf[5] = hrPeak;
  buf[6] = static_cast<uint8_t>(delta);
  buf[7] = flagged ? 1 : 0;

  auto* om = ble_hs_mbuf_from_flat(buf, sizeof(buf));
  ble_gattc_notify_custom(connHandle, orthoHandle, om);
}

void POTSDataService::UpdateDailyStats(uint16_t activeMins, uint16_t sedentaryMins, uint16_t uprightMins) {
  // Bytes 2-7: daily stats (little-endian uint16 each)
  dailyBuf[2] = static_cast<uint8_t>(activeMins & 0xFF);
  dailyBuf[3] = static_cast<uint8_t>((activeMins >> 8) & 0xFF);
  dailyBuf[4] = static_cast<uint8_t>(sedentaryMins & 0xFF);
  dailyBuf[5] = static_cast<uint8_t>((sedentaryMins >> 8) & 0xFF);
  dailyBuf[6] = static_cast<uint8_t>(uprightMins & 0xFF);
  dailyBuf[7] = static_cast<uint8_t>((uprightMins >> 8) & 0xFF);
}

void POTSDataService::SubscribeNotification(uint16_t attributeHandle) {
  if (attributeHandle == hrvHandle) hrvNotifyEnabled = true;
  else if (attributeHandle == orthoHandle) orthoNotifyEnabled = true;
}

void POTSDataService::UnsubscribeNotification(uint16_t attributeHandle) {
  if (attributeHandle == hrvHandle) hrvNotifyEnabled = false;
  else if (attributeHandle == orthoHandle) orthoNotifyEnabled = false;
}
