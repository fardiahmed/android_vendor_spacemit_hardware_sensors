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

// Sensors whose value is a sysfs attribute of a kernel driver (IIO or hwmon):
// value = attribute * scale.

#pragma once

#include "sensors-impl/Sensor.h"

#include <string>

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

// Continuous (BMP280 pressure).
class SysfsSensor : public Sensor {
  public:
    SysfsSensor(ISensorsEventCallback* callback, const SensorInfo& info, std::string path,
                float scale);

  protected:
    bool readEventPayload(EventPayload& payload) override;

  private:
    std::string mPath;
    float mScale;
};

// On-change (AHT20 temperature and humidity).
class OnChangeSysfsSensor : public OnChangeSensor {
  public:
    OnChangeSysfsSensor(ISensorsEventCallback* callback, const SensorInfo& info, std::string path,
                        float scale);

  protected:
    bool readEventPayload(EventPayload& payload) override;

  private:
    std::string mPath;
    float mScale;
};

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
