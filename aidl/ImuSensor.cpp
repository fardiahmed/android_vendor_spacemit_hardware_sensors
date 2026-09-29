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

#include "sensors-impl/ImuSensor.h"

#include <aidl/android/hardware/sensors/SensorStatus.h>

#include <string>

#include "sensors-impl/Utils.h"

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

ImuSensor::ImuSensor(ISensorsEventCallback* callback, std::shared_ptr<Mpu9250> imu, Kind kind)
    : Sensor(callback), mImu(std::move(imu)), mKind(kind) {
    const std::string chip = mImu->name();
    switch (kind) {
        case Kind::ACCEL:
            mSensorInfo =
                    makeSensorInfo(chip + " Accelerometer", SensorType::ACCELEROMETER, 4 * kGravity,
                                   kGravity / Mpu9250::kAccelLsbPerG, 0.45f, 5000, 1000000, false);
            break;
        case Kind::GYRO:
            mSensorInfo =
                    makeSensorInfo(chip + " Gyroscope", SensorType::GYROSCOPE, 500 * kDegToRad,
                                   kDegToRad / Mpu9250::kGyroLsbPerDps, 3.2f, 5000, 1000000, false);
            break;
        case Kind::MAG:
            mSensorInfo = makeSensorInfo("AK8963 Magnetometer", SensorType::MAGNETIC_FIELD, 4800.f,
                                         Mpu9250::kMagUtPerLsb, 0.28f, 10000, 1000000, false);
            break;
    }
    start();
}

ImuSensor::~ImuSensor() {
    // Stop the sampling thread (it releases the chip) before mImu goes away.
    {
        std::lock_guard<std::mutex> lock(mRunMutex);
        mStopThread = true;
        mIsEnabled = false;
        mWaitCV.notify_all();
    }
    if (mRunThread.joinable()) mRunThread.join();
}

void ImuSensor::onEnabled(bool enabled) {
    if (enabled) {
        mImu->acquire();
    } else {
        mImu->release();
    }
}

bool ImuSensor::readEventPayload(EventPayload& payload) {
    float v[3];
    bool ok = false;
    SensorStatus status = SensorStatus::ACCURACY_HIGH;
    switch (mKind) {
        case Kind::ACCEL:
            ok = mImu->readAccel(v);
            break;
        case Kind::GYRO:
            ok = mImu->readGyro(v);
            break;
        case Kind::MAG:
            ok = mImu->readMag(v);
            status = SensorStatus::ACCURACY_MEDIUM;  // no hard-iron calibration in the HAL
            break;
    }
    if (!ok) return false;
    EventPayload::Vec3 vec3 = {.x = v[0], .y = v[1], .z = v[2], .status = status};
    payload.set<EventPayload::Tag::vec3>(vec3);
    return true;
}

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
