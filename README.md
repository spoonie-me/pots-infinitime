# POTS Companion — InfiniTime Firmware Fork

A custom [InfiniTime](https://github.com/InfiniTimeOrg/InfiniTime) firmware fork adding the **POTS Companion watchface** for the [PineTime](https://www.pine64.org/pinetime/) open smartwatch.

Built for people with **Postural Orthostatic Tachycardia Syndrome (POTS)** and dysautonomia — conditions where the autonomic nervous system's response to gravity is impaired, making every standing transition a measurable physiological event.

This is not a fitness tracker. It is a **constraint-aware body monitoring tool**.

---

## What it monitors

| Signal | Why it matters for POTS |
|--------|------------------------|
| Resting HR | Baseline sympathetic tone — shifts before symptoms do |
| Orthostatic HR delta | The diagnostic number: ≥30 bpm rise on standing = POTS threshold |
| HRV proxy (RMSSD) | Autonomic state: Low = flare risk, Good = recovery window |
| Upright minutes | Cumulative orthostatic load for the day |
| Active minutes | Capacity spent — not a goal, a budget |
| Sedentary minutes | Recovery time banked — alerts at 45 min stretch |

The orthostatic delta is the number that tells a POTS patient whether today is a flare day **before** they've committed to standing up and paying for it with hours of recovery.

---

## Watchface layout

```
┌─────────────────────────────┐
│  08:42          07 May      │  ← time + date
│                             │
│   ♥  89 bpm    +22 Δ       │  ← HR (amber during ortho window)
│                             │
│  HRV: MOD        UPRIGHT   │  ← HRV zone + posture
│                             │
│  ┌────┐  ┌────┐  ┌────┐   │
│  │████│  │████│  │    │   │  ← active / sedentary / upright rings
│  └────┘  └────┘  └────┘   │
│                             │
│       steps: 2,341          │
└─────────────────────────────┘
```

**Colour coding:**
- HR text: white → amber (ortho window open) → red (delta ≥ 30 bpm flagged)
- HRV: teal (Good) / amber (Moderate) / red (Low)
- Ring 1 teal: active minutes, goal 30/day
- Ring 2 coral: sedentary inverse — full when resting, drains as you sit, turns red at 45-min streak
- Ring 3 purple: upright minutes, goal 60/day for POTS management

---

## Orthostatic detection

The firmware watches for **standing transitions**:

1. Detects supine posture via BMA42x Z-axis (|Z| > 0.85g)
2. Requires ≥60 seconds of lying down before arming
3. On standing (|Z| < 0.3g, |Y| > 0.7g): opens a 3-minute observation window
4. Tracks peak HR during window, computes delta vs. reclined baseline
5. Flags event if delta ≥ 30 bpm (established POTS diagnostic criterion)
6. Stores last 5 events in a static ring buffer — no heap allocation

All computation runs on-watch. No companion app required for core function.

---

## Architecture

```
BMA42x accelerometer → MotionController → OrthoPOTSDetector
HRS3300 PPG sensor   → HeartRateController → OrthoPOTSDetector
                                           → HrvCalculator
OrthoPOTSDetector + HrvCalculator + ActivityClassifier → WatchFacePOTS (LVGL)
```

**New components:**
- `src/components/pots/OrthoPOTSDetector` — posture state machine + orthostatic event detection
- `src/components/pots/HrvCalculator` — RMSSD proxy from IBI estimates, zone classification
- `src/components/pots/ActivityClassifier` — BMA42x magnitude classifier, daily minute counters
- `src/displayapp/screens/WatchFacePOTS` — 240×240 LVGL watchface, 1Hz refresh, zero heap in render loop

---

## Hardware constraints

Runs on PineTime: ARM Cortex-M4 @ 64MHz, 64KB RAM, 512KB flash.

Design choices:
- No heap allocation in the render loop — all LVGL objects created once in constructor
- No floating point in hot paths — integer fixed-point math throughout
- No new HR session started — reads from the existing `HeartRateController`
- HRV labeled "proxy" throughout — HRS3300 + FFT pipeline is approximate

---

## Build

### Dependencies
- `arm-none-eabi-gcc` and `cmake >= 3.16`
- [nRF5 SDK 15.3.0](https://www.nordicsemi.com/Software-and-tools/Software/nRF5-SDK)
- [InfiniTime build guide](https://github.com/InfiniTimeOrg/InfiniTime/blob/develop/doc/buildAndProgram.md)

```bash
git clone https://github.com/spoonie-me/pots-infinitime.git
cd pots-infinitime
mkdir build && cd build

cmake \
  -DARM_NONE_EABI_TOOLCHAIN_PATH=/usr \
  -DNRF5_SDK_PATH=/path/to/nrf5_sdk \
  -DCMAKE_BUILD_TYPE=Release \
  ..

make -j$(nproc) pinetime-app
# Output: build/src/pinetime-app-*.zip  (OTA-flashable via Gadgetbridge / Amazfish)
```

### Simulator (no hardware required)

```bash
# Requires: libsdl2-dev
cmake -DUSE_SDL2_SIMULATOR=ON ..
make pinetime-sim
./pinetime-sim
```

---

## Flash

Flash the `.zip` OTA package via:
- [Gadgetbridge](https://gadgetbridge.org/) (Android)
- [Amazfish](https://github.com/piggz/harbour-amazfish) (SailfishOS / Linux)
- [InfiniLink](https://github.com/InfiniTimeOrg/InfiniLink) (iOS)
- OpenOCD (development hardware)

---

## Phased roadmap

- [x] **Phase 1** — Core watchface: live HR, steps, activity rings, simulator-ready
- [x] **Phase 2** — Orthostatic detection: posture state machine, delta display, 3-min window
- [x] **Phase 3** — HRV proxy: RMSSD from IBI estimates, zone classification
- [x] **Phase 4** — Polish: sedentary alert, daily reset, edge cases (no HR, cold start)
- [ ] **Phase 5** — True IBI: peak detection in PPG pipeline for clinical-grade RMSSD
- [ ] **Phase 6** — Gadgetbridge logging: BLE export of orthostatic events for trend review
- [ ] **Phase 7** — Flare prediction: rolling baseline shift detection

---

## Contributing

PRs welcome, especially from people with POTS who use this day-to-day. Please open an issue before large changes.

If you have dysautonomia and want to help shape the feature roadmap, open a Discussion — lived experience drives better design than any spec.

---

## Upstream

This is a fork of [InfiniTime](https://github.com/InfiniTimeOrg/InfiniTime) by the InfiniTime contributors, licensed GPL-3.0. All upstream changes are tracked on the `main` branch; POTS-specific work lives on `feature/pots-companion-watchface`.

## License

GPL-3.0-or-later — same as InfiniTime upstream. See [LICENSE](LICENSE).
