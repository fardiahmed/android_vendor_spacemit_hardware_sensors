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

#include "sensors-impl/I2cDevice.h"

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

bool I2cDevice::readRegs(uint8_t reg, uint8_t* buf, size_t len) const {
    i2c_msg msgs[2] = {
            {.addr = mAddr, .flags = 0, .len = 1, .buf = &reg},
            {.addr = mAddr, .flags = I2C_M_RD, .len = static_cast<uint16_t>(len), .buf = buf},
    };
    i2c_rdwr_ioctl_data xfer = {.msgs = msgs, .nmsgs = 2};
    return ioctl(mFd, I2C_RDWR, &xfer) == 2;
}

bool I2cDevice::writeReg(uint8_t reg, uint8_t value) const {
    uint8_t data[2] = {reg, value};
    i2c_msg msg = {.addr = mAddr, .flags = 0, .len = 2, .buf = data};
    i2c_rdwr_ioctl_data xfer = {.msgs = &msg, .nmsgs = 1};
    return ioctl(mFd, I2C_RDWR, &xfer) == 1;
}

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
