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

#include <cstddef>
#include <cstdint>

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

// One 7-bit I2C device on an i2c-dev bus (/dev/i2c-N). The bus fd is owned by the caller.
class I2cDevice {
  public:
    I2cDevice(int busFd, uint8_t addr) : mFd(busFd), mAddr(addr) {}

    bool readRegs(uint8_t reg, uint8_t* buf, size_t len) const;
    bool readReg(uint8_t reg, uint8_t* value) const { return readRegs(reg, value, 1); }
    bool writeReg(uint8_t reg, uint8_t value) const;

  private:
    int mFd;
    uint8_t mAddr;
};

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
