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

#include "sensors-impl/Utils.h"

#include <android-base/strings.h>
#include <dirent.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

const char* const kSensorVendor = "SpacemiT K1 header I2C4";

void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

std::string findSubdir(const std::string& dir, const std::string& prefix) {
    std::unique_ptr<DIR, int (*)(DIR*)> d(opendir(dir.c_str()), closedir);
    if (!d) return {};
    while (dirent* e = readdir(d.get())) {
        if (::android::base::StartsWith(e->d_name, prefix)) return dir + "/" + e->d_name;
    }
    return {};
}

bool readSysfsValue(const std::string& path, double* value) {
    std::ifstream f(path);
    std::string s;
    if (!f || !std::getline(f, s)) return false;
    char* end = nullptr;
    *value = strtod(s.c_str(), &end);
    return end != s.c_str();
}

Sensor::SensorInfo makeSensorInfo(const std::string& name, Sensor::SensorType type, float maxRange,
                                  float resolution, float powerMa, int32_t minDelayUs,
                                  int32_t maxDelayUs, bool onChange) {
    Sensor::SensorInfo info;
    info.name = name;
    info.vendor = kSensorVendor;
    info.version = 1;
    info.type = type;
    info.typeAsString = "";
    info.maxRange = maxRange;
    info.resolution = resolution;
    info.power = powerMa;
    info.minDelayUs = minDelayUs;
    info.fifoReservedEventCount = 0;
    info.fifoMaxEventCount = 0;
    info.requiredPermission = "";
    info.maxDelayUs = maxDelayUs;
    info.flags =
            onChange ? static_cast<uint32_t>(Sensor::SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE)
                     : 0;
    return info;
}

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
