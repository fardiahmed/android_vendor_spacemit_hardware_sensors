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

#pragma once

#include <array>

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

// Axis remapping from a module's silk-screen frame to the Android device frame, read from
// ro.vendor.sensors.imu.axes = "<X>,<Y>,<Z>", each [-]x|y|z (default "x,y,z").
struct AxisMap {
    std::array<int, 3> src{0, 1, 2};
    std::array<float, 3> sign{1.f, 1.f, 1.f};

    static AxisMap fromProperty();

    void apply(const float in[3], float out[3]) const {
        for (int i = 0; i < 3; i++) out[i] = sign[i] * in[src[i]];
    }
};

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
