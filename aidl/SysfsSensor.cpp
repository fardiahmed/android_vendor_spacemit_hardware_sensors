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

#include "sensors-impl/SysfsSensor.h"

#include "sensors-impl/Utils.h"

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

namespace {

bool setScalar(Sensor::EventPayload& payload, const std::string& path, float scale) {
    double v;
    if (!readSysfsValue(path, &v)) return false;
    payload.set<Sensor::EventPayload::Tag::scalar>(static_cast<float>(v * scale));
    return true;
}

}  // namespace

SysfsSensor::SysfsSensor(ISensorsEventCallback* callback, const SensorInfo& info, std::string path,
                         float scale)
    : Sensor(callback), mPath(std::move(path)), mScale(scale) {
    mSensorInfo = info;
    start();
}

bool SysfsSensor::readEventPayload(EventPayload& payload) {
    return setScalar(payload, mPath, mScale);
}

OnChangeSysfsSensor::OnChangeSysfsSensor(ISensorsEventCallback* callback, const SensorInfo& info,
                                         std::string path, float scale)
    : OnChangeSensor(callback), mPath(std::move(path)), mScale(scale) {
    mSensorInfo = info;
    start();
}

bool OnChangeSysfsSensor::readEventPayload(EventPayload& payload) {
    return setScalar(payload, mPath, mScale);
}

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
