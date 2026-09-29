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

#include "sensors-impl/Sensor.h"

#include <string>

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

constexpr float kGravity = 9.80665f;
constexpr float kDegToRad = 0.017453292519943295f;

// Vendor string of every sensor on the 26-pin header I2C4.
extern const char* const kSensorVendor;

void sleepMs(int ms);

// First entry of |dir| whose name starts with |prefix| ("" if none), as "<dir>/<entry>".
std::string findSubdir(const std::string& dir, const std::string& prefix);

// Reads a numeric sysfs attribute.
bool readSysfsValue(const std::string& path, double* value);

// SensorInfo for a sensor without FIFO; the handle is assigned when it is added to the HAL.
Sensor::SensorInfo makeSensorInfo(const std::string& name, Sensor::SensorType type, float maxRange,
                                  float resolution, float powerMa, int32_t minDelayUs,
                                  int32_t maxDelayUs, bool onChange);

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
