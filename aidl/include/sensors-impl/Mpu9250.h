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

// GY-9250 IMU (MPU-925x/6500, AK8963 via bypass) at 0x68 over i2c-dev: the kernel
// driver has no polled mode.

#pragma once

#include "sensors-impl/AxisMap.h"
#include "sensors-impl/I2cDevice.h"

#include <memory>
#include <mutex>
#include <string>

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

class Mpu9250 {
  public:
    // Probes 0x68 on the bus; nullptr when no InvenSense MPU answers.
    static std::shared_ptr<Mpu9250> probe(int busFd);

    const std::string& name() const { return mName; }
    bool hasMagnetometer() const { return mHasMag; }

    // Reference-counted power: the chip sleeps while no IMU sensor is active.
    void acquire();
    void release();

    // Android device frame, SI units: m/s^2, rad/s, uT.
    bool readAccel(float out[3]);
    bool readGyro(float out[3]);
    bool readMag(float out[3]);

    static constexpr float kAccelLsbPerG = 8192.f;  // +-4 g
    static constexpr float kGyroLsbPerDps = 65.5f;  // +-500 dps
    static constexpr float kMagUtPerLsb = 0.15f;    // AK8963, 16-bit output

  private:
    Mpu9250(int busFd, uint8_t whoAmI);
    bool init();
    bool initMag();
    bool readMotion(int16_t raw[7]);

    I2cDevice mDev;
    I2cDevice mMag;
    std::string mName;
    bool mIsMpu925x;
    bool mHasMag = false;
    float mMagAdjust[3] = {1.f, 1.f, 1.f};
    float mLastMag[3] = {0.f, 0.f, 0.f};
    bool mMagValid = false;
    int mUsers = 0;
    AxisMap mAxes;
    std::mutex mLock;
};

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
