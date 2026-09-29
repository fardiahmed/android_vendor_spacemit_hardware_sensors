/*
 * Copyright (C) 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "sensors-spacemit"

#include "sensors-impl/AxisMap.h"

#include <android-base/properties.h>
#include <android-base/strings.h>
#include <log/log.h>

#include <string>

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

AxisMap AxisMap::fromProperty() {
    AxisMap map;
    const std::string prop = ::android::base::GetProperty("ro.vendor.sensors.imu.axes", "x,y,z");
    auto parts = ::android::base::Split(prop, ",");
    if (parts.size() != 3) {
        ALOGE("ro.vendor.sensors.imu.axes='%s': expected 3 axes, using x,y,z", prop.c_str());
        return map;
    }
    for (int i = 0; i < 3; i++) {
        std::string a = ::android::base::Trim(parts[i]);
        float sign = 1.f;
        if (!a.empty() && (a[0] == '-' || a[0] == '+')) {
            sign = a[0] == '-' ? -1.f : 1.f;
            a = a.substr(1);
        }
        if (a != "x" && a != "y" && a != "z") {
            ALOGE("ro.vendor.sensors.imu.axes='%s': bad axis '%s', using x,y,z", prop.c_str(),
                  parts[i].c_str());
            return AxisMap();
        }
        map.src[i] = a[0] - 'x';
        map.sign[i] = sign;
    }
    ALOGI("IMU axes mapped as %s", prop.c_str());
    return map;
}

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
