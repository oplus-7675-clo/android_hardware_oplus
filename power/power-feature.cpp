/*
 * Copyright (C) 2022 Paranoid Android
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aidl/vendor/aospa/power/BnPowerFeature.h>
#include <aidl/vendor/oplus/hardware/touch/IOplusTouch.h>
#include <android-base/file.h>
#include <android-base/strings.h>
#include <android/binder_manager.h>
#include <unordered_map>

#include <OplusTouchConstants.h>

#define GESTURE_ENABLE_PATH "/proc/touchpanel/double_tap_enable_indep"

using aidl::vendor::oplus::hardware::touch::IOplusTouch;
using ::android::base::ReadFileToString;
using ::android::base::Trim;
using ::android::base::WriteStringToFile;

namespace aidl {
namespace vendor {
namespace aospa {
namespace power {

// based on values in touchpanel_common.h
static const std::unordered_map<Feature, int> GESTURE_MAP = {
    {Feature::DRAW_V, 1},
    {Feature::DRAW_INVERSE_V, 2},
    {Feature::DRAW_O, 6},
    {Feature::DRAW_M, 12},
    {Feature::DRAW_W, 13},
    {Feature::DRAW_ARROW_LEFT, 4},
    {Feature::DRAW_ARROW_RIGHT, 3},
    {Feature::ONE_FINGER_SWIPE_UP, 11},
    {Feature::ONE_FINGER_SWIPE_RIGHT, 8},
    {Feature::ONE_FINGER_SWIPE_DOWN, 10},
    {Feature::ONE_FINGER_SWIPE_LEFT, 9},
    {Feature::TWO_FINGER_SWIPE, 7},
    {Feature::DRAW_S, 18},
};

static std::shared_ptr<IOplusTouch> getOplusTouch() {
    static std::shared_ptr<IOplusTouch> mOplusTouch = nullptr;
    static bool initialized = false;

    if (!initialized) {
        const std::string instance = std::string() + IOplusTouch::descriptor + "/default";
        if (AServiceManager_isDeclared(instance.c_str())) {
            mOplusTouch = IOplusTouch::fromBinder(ndk::SpAIBinder(
                    AServiceManager_waitForService(instance.c_str())));
        }
        initialized = true;
    }
    return mOplusTouch;
}

bool setDeviceSpecificFeature(Feature feature, bool enabled) {
    int contents = 0;
    auto gesture = GESTURE_MAP.find(feature);

    if (gesture == GESTURE_MAP.end()) {
        // Unsupported gesture
        return false;
    }

    auto oplusTouch = getOplusTouch();

    if (std::string tmp; oplusTouch) {
        oplusTouch->touchReadNodeFile(OplusTouchConstants::DEFAULT_TP_IC_ID,
                                       OplusTouchConstants::DOUBLE_TAP_INDEP_NODE, &tmp);
        contents = std::stoi(Trim(tmp), nullptr, 16);
    } else if (ReadFileToString(GESTURE_ENABLE_PATH, &tmp)) {
        contents = std::stoi(Trim(tmp), nullptr, 16);
    }

    if (enabled) {
        contents |= (1 << gesture->second);
    } else {
        contents &= ~(1 << gesture->second);
    }

    if (oplusTouch) {
        oplusTouch->touchWriteNodeFileOneWay(OplusTouchConstants::DEFAULT_TP_IC_ID,
                                              OplusTouchConstants::DOUBLE_TAP_ENABLE_NODE, "1");
        oplusTouch->touchWriteNodeFileOneWay(OplusTouchConstants::DEFAULT_TP_IC_ID,
                                              OplusTouchConstants::DOUBLE_TAP_INDEP_NODE,
                                              std::to_string(contents));
        return true;
    } else {
        return WriteStringToFile(std::to_string(contents), GESTURE_ENABLE_PATH, true);
    }
}

}  // namespace power
}  // namespace aospa
}  // namespace vendor
}  // namespace aidl
