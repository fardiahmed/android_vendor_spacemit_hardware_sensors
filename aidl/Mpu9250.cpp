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

#include "sensors-impl/Mpu9250.h"

#include <log/log.h>

#include "sensors-impl/Utils.h"

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

namespace {

// MPU-6050/6500/9250 registers
constexpr uint8_t kMpuAddr = 0x68;
constexpr uint8_t kSmplrtDiv = 0x19;
constexpr uint8_t kConfig = 0x1a;
constexpr uint8_t kGyroConfig = 0x1b;
constexpr uint8_t kAccelConfig = 0x1c;
constexpr uint8_t kAccelConfig2 = 0x1d;
constexpr uint8_t kIntPinCfg = 0x37;
constexpr uint8_t kAccelXoutH = 0x3b;
constexpr uint8_t kUserCtrl = 0x6a;
constexpr uint8_t kPwrMgmt1 = 0x6b;
constexpr uint8_t kPwrMgmt2 = 0x6c;
constexpr uint8_t kWhoAmI = 0x75;
constexpr uint8_t kPwr1ClkPll = 0x01;
constexpr uint8_t kPwr1Sleep = 0x40;
constexpr uint8_t kPwr1Reset = 0x80;
// +-4 g, +-500 dps, DLPF 41 Hz, 1 kHz / (1 + 4) = 200 Hz: see Mpu9250::init()

// AK8963 magnetometer (inside the MPU-9250/9255), reached through the MPU's I2C bypass
constexpr uint8_t kMagAddr = 0x0c;
constexpr uint8_t kMagWia = 0x00;
constexpr uint8_t kMagSt1 = 0x02;
constexpr uint8_t kMagHxl = 0x03;
constexpr uint8_t kMagCntl1 = 0x0a;
constexpr uint8_t kMagCntl2 = 0x0b;
constexpr uint8_t kMagAsax = 0x10;
constexpr uint8_t kMagModePowerDown = 0x00;
constexpr uint8_t kMagModeFuseRom = 0x0f;
constexpr uint8_t kMagModeCont100Hz16Bit = 0x16;
constexpr float kMagUtPerLsb = 0.15f;  // 16-bit output

}  // namespace

std::shared_ptr<Mpu9250> Mpu9250::probe(int busFd) {
    I2cDevice dev(busFd, kMpuAddr);
    uint8_t who = 0;
    if (!dev.readReg(kWhoAmI, &who)) {
        ALOGI("no IMU at 0x%02x", kMpuAddr);
        return nullptr;
    }
    switch (who) {
        case 0x68:  // MPU-6050
        case 0x70:  // MPU-6500
        case 0x71:  // MPU-9250
        case 0x73:  // MPU-9255
        case 0x74:  // MPU-6515
        case 0x75:
            break;
        default:
            ALOGW("IMU at 0x%02x: unknown WHO_AM_I 0x%02x, treating it as an MPU-6500", kMpuAddr,
                  who);
    }
    std::shared_ptr<Mpu9250> imu(new Mpu9250(busFd, who));
    if (!imu->init()) return nullptr;
    return imu;
}

Mpu9250::Mpu9250(int busFd, uint8_t whoAmI)
    : mDev(busFd, kMpuAddr),
      mMag(busFd, kMagAddr),
      mIsMpu925x(whoAmI == 0x71 || whoAmI == 0x73),
      mAxes(AxisMap::fromProperty()) {
    switch (whoAmI) {
        case 0x68:
            mName = "MPU-6050";
            break;
        case 0x71:
            mName = "MPU-9250";
            break;
        case 0x73:
            mName = "MPU-9255";
            break;
        case 0x74:
            mName = "MPU-6515";
            break;
        default:
            mName = "MPU-6500";
            break;
    }
    ALOGI("IMU: %s (WHO_AM_I 0x%02x)", mName.c_str(), whoAmI);
}

bool Mpu9250::init() {
    bool ok = mDev.writeReg(kPwrMgmt1, kPwr1Reset);
    sleepMs(100);
    ok = ok && mDev.writeReg(kPwrMgmt1, kPwr1ClkPll);
    sleepMs(10);
    ok = ok && mDev.writeReg(kPwrMgmt2, 0x00)  // all axes on
         && mDev.writeReg(kConfig, 0x03)       // gyro DLPF 41 Hz
         && mDev.writeReg(kSmplrtDiv, 0x04)    // 200 Hz
         && mDev.writeReg(kGyroConfig, 0x08)   // +-500 dps
         && mDev.writeReg(kAccelConfig, 0x08)  // +-4 g
         && mDev.writeReg(kUserCtrl, 0x00)     // I2C master off
         && mDev.writeReg(kIntPinCfg, 0x02);   // bypass: AK8963 on the host bus
    if (!ok) {
        ALOGE("IMU init failed");
        return false;
    }
    if (mName != "MPU-6050") mDev.writeReg(kAccelConfig2, 0x03);  // accel DLPF 41 Hz
    mHasMag = initMag();
    mDev.writeReg(kPwrMgmt1, kPwr1Sleep | kPwr1ClkPll);  // sleep until a sensor is enabled
    return true;
}

bool Mpu9250::initMag() {
    uint8_t wia = 0;
    if (!mMag.readReg(kMagWia, &wia) || wia != 0x48) {
        if (mIsMpu925x) ALOGW("%s: AK8963 magnetometer not found", mName.c_str());
        return false;
    }
    mMag.writeReg(kMagCntl2, 0x01);  // soft reset
    sleepMs(10);
    mMag.writeReg(kMagCntl1, kMagModePowerDown);
    sleepMs(10);
    mMag.writeReg(kMagCntl1, kMagModeFuseRom);
    sleepMs(10);
    uint8_t asa[3] = {128, 128, 128};
    if (!mMag.readRegs(kMagAsax, asa, 3)) ALOGW("AK8963: cannot read the sensitivity ROM");
    for (int i = 0; i < 3; i++) mMagAdjust[i] = (asa[i] - 128) / 256.f + 1.f;
    mMag.writeReg(kMagCntl1, kMagModePowerDown);
    ALOGI("AK8963 magnetometer: sensitivity adjustment %.3f %.3f %.3f", mMagAdjust[0],
          mMagAdjust[1], mMagAdjust[2]);
    return true;
}

void Mpu9250::acquire() {
    std::lock_guard<std::mutex> lock(mLock);
    if (mUsers++ > 0) return;
    mDev.writeReg(kPwrMgmt1, kPwr1ClkPll);
    if (mHasMag) mMag.writeReg(kMagCntl1, kMagModeCont100Hz16Bit);
    mMagValid = false;
    sleepMs(40);  // gyro start-up (MPU-6500: 35 ms)
}

void Mpu9250::release() {
    std::lock_guard<std::mutex> lock(mLock);
    if (mUsers == 0 || --mUsers > 0) return;
    if (mHasMag) mMag.writeReg(kMagCntl1, kMagModePowerDown);
    mDev.writeReg(kPwrMgmt1, kPwr1Sleep | kPwr1ClkPll);
}

bool Mpu9250::readMotion(int16_t raw[7]) {
    uint8_t buf[14];
    {
        std::lock_guard<std::mutex> lock(mLock);
        if (!mDev.readRegs(kAccelXoutH, buf, sizeof(buf))) return false;
    }
    for (int i = 0; i < 7; i++) raw[i] = static_cast<int16_t>((buf[2 * i] << 8) | buf[2 * i + 1]);
    return true;
}

bool Mpu9250::readAccel(float out[3]) {
    int16_t raw[7];
    if (!readMotion(raw)) return false;
    const float in[3] = {raw[0] * kGravity / kAccelLsbPerG, raw[1] * kGravity / kAccelLsbPerG,
                         raw[2] * kGravity / kAccelLsbPerG};
    mAxes.apply(in, out);
    return true;
}

bool Mpu9250::readGyro(float out[3]) {
    int16_t raw[7];
    if (!readMotion(raw)) return false;
    const float k = kDegToRad / kGyroLsbPerDps;
    const float in[3] = {raw[4] * k, raw[5] * k, raw[6] * k};
    mAxes.apply(in, out);
    return true;
}

bool Mpu9250::readMag(float out[3]) {
    std::lock_guard<std::mutex> lock(mLock);
    uint8_t st1 = 0;
    uint8_t buf[7];  // HXL..HZH, ST2 (reading ST2 releases the data registers)
    if (mMag.readReg(kMagSt1, &st1) && (st1 & 0x01) && mMag.readRegs(kMagHxl, buf, sizeof(buf)) &&
        !(buf[6] & 0x08) /* no overflow */) {
        float m[3];
        for (int i = 0; i < 3; i++) {
            const int16_t v = static_cast<int16_t>(buf[2 * i] | (buf[2 * i + 1] << 8));
            m[i] = v * kMagUtPerLsb * mMagAdjust[i];
        }
        // AK8963 axes vs the MPU accel/gyro frame: X_mpu = Y_ak, Y_mpu = X_ak, Z_mpu = -Z_ak
        mLastMag[0] = m[1];
        mLastMag[1] = m[0];
        mLastMag[2] = -m[2];
        mMagValid = true;
    }
    if (!mMagValid) return false;
    mAxes.apply(mLastMag, out);  // no new sample yet (100 Hz): repeat the last one
    return true;
}

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
