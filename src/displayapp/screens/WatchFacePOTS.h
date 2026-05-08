#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <cstdint>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/pots/OrthoPOTSDetector.h"
#include "components/pots/HrvCalculator.h"
#include "components/pots/ActivityClassifier.h"
#include "utility/DirtyValue.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
  }

  namespace Applications {
    namespace Screens {

      class WatchFacePOTS : public Screen {
      public:
        WatchFacePOTS(Controllers::DateTime& dateTimeController,
                      Controllers::HeartRateController& heartRateController,
                      Controllers::MotionController& motionController);
        ~WatchFacePOTS() override;

        void Refresh() override;

      private:
        Controllers::DateTime& dateTimeController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;

        Components::OrthoPOTSDetector orthoDetector;
        Components::HrvCalculator hrvCalc;
        Components::ActivityClassifier activityClassifier;

        // State tracking
        uint8_t lastHr = 0;
        uint32_t secondCounter = 0;     // seconds since watchface created
        uint16_t lastDayStamp = 0;      // date stamp for daily reset detection
        uint32_t lastHrIbiSec = 0;      // when we last added an IBI sample

        // LVGL objects — all allocated once in constructor
        lv_obj_t* label_time;
        lv_obj_t* label_date;
        lv_obj_t* label_hr;
        lv_obj_t* label_hr_unit;
        lv_obj_t* label_delta;
        lv_obj_t* label_hrv_text;
        lv_obj_t* label_posture;
        lv_obj_t* arc_active;
        lv_obj_t* arc_sedentary;
        lv_obj_t* arc_upright;
        lv_obj_t* label_steps;

        lv_task_t* taskRefresh;

        // Color constants
        static constexpr uint32_t COL_BG       = 0x0A0A0A;
        static constexpr uint32_t COL_WHITE     = 0xFFFFFF;
        static constexpr uint32_t COL_MUTED     = 0x888888;
        static constexpr uint32_t COL_AMBER     = 0xFFB347;
        static constexpr uint32_t COL_RED       = 0xFF4444;
        static constexpr uint32_t COL_TEAL      = 0x4ECDC4;
        static constexpr uint32_t COL_CORAL     = 0xFF6B6B;
        static constexpr uint32_t COL_PURPLE    = 0xB69CFF;
        static constexpr uint32_t COL_HRV_LOW   = 0xFF4444;
        static constexpr uint32_t COL_HRV_MOD   = 0xFFB347;
        static constexpr uint32_t COL_HRV_GOOD  = 0x4ECDC4;

        // Clamp a value to [0, 100]
        static uint8_t Clamp100(uint32_t v) {
          return static_cast<uint8_t>(v > 100 ? 100 : v);
        }

        void UpdateHrDisplay(uint8_t hr);
        void UpdateHrvDisplay();
        void UpdateActivityRings();
        void UpdatePostureLabel();
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::POTS> {
      static constexpr WatchFace watchFace = WatchFace::POTS;
      static constexpr const char* name = "POTS Companion";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFacePOTS(controllers.dateTimeController,
                                          controllers.heartRateController,
                                          controllers.motionController);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
