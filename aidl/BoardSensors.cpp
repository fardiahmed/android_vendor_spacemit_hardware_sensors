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

#include "sensors-impl/BoardSensors.h"

#include <android-base/properties.h>
#include <fcntl.h>
#include <log/log.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <memory>
#include <string>

#include "sensors-impl/ImuSensor.h"
#include "sensors-impl/Mpu9250.h"
#include "sensors-impl/SysfsSensor.h"
#include "sensors-impl/Utils.h"

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace spacemit {

void registerBoardSensors(Sensors& hal) {
    ISensorsEventCallback* cb = hal.eventCallback();
    const std::string bus = ::android::base::GetProperty("ro.vendor.sensors.i2c_bus", "4");
    const std::string busSysfs = ::android::base::GetProperty(
            "ro.vendor.sensors.i2c_sysfs", "/sys/devices/platform/soc/d4012800.i2c/i2c-" + bus);

    // IMU: /dev/i2c-N appears once i2c-dev is loaded (init.k1.rc, early-boot).
    const std::string dev = "/dev/i2c-" + bus;
    int fd = -1;
    for (int i = 0; i < 30 && fd < 0; i++) {
        fd = open(dev.c_str(), O_RDWR | O_CLOEXEC);
        if (fd < 0) sleepMs(100);
    }
    if (fd < 0) {
        ALOGE("cannot open %s: %s (IMU unavailable)", dev.c_str(), strerror(errno));
    } else if (auto imu = Mpu9250::probe(fd)) {  // fd stays open for the HAL's lifetime
        hal.addSensor(std::make_shared<ImuSensor>(cb, imu, ImuSensor::Kind::ACCEL));
        hal.addSensor(std::make_shared<ImuSensor>(cb, imu, ImuSensor::Kind::GYRO));
        if (imu->hasMagnetometer()) {
            hal.addSensor(std::make_shared<ImuSensor>(cb, imu, ImuSensor::Kind::MAG));
        }
    }

    // AHT20 (hwmon "aht10" driver): temperature + humidity, milli-units. The driver caches a
    // measurement for 2 s, so 1 Hz is plenty.
    bool haveTemperature = false;
    const std::string aht = findSubdir(busSysfs + "/" + bus + "-0038/hwmon", "hwmon");
    if (!aht.empty() && access((aht + "/humidity1_input").c_str(), R_OK) == 0) {
        hal.addSensor(std::make_shared<OnChangeSysfsSensor>(
                cb,
                makeSensorInfo("AHT20 Relative Humidity", SensorType::RELATIVE_HUMIDITY, 100.f,
                               0.024f, 0.023f, 1000000, 10000000, true),
                aht + "/humidity1_input", 0.001f));
        hal.addSensor(std::make_shared<OnChangeSysfsSensor>(
                cb,
                makeSensorInfo("AHT20 Temperature", SensorType::AMBIENT_TEMPERATURE, 85.f, 0.01f,
                               0.023f, 1000000, 10000000, true),
                aht + "/temp1_input", 0.001f));
        haveTemperature = true;
    } else {
        ALOGI("no AHT20 at 0x38 (%s)", busSysfs.c_str());
    }

    // BMP280 (IIO): in_pressure_input in kPa, in_temp_input in milli-degrees C.
    for (const char* addr : {"0076", "0077"}) {
        const std::string iio = findSubdir(busSysfs + "/" + bus + "-" + addr, "iio:device");
        if (iio.empty() || access((iio + "/in_pressure_input").c_str(), R_OK) != 0) continue;
        hal.addSensor(std::make_shared<SysfsSensor>(
                cb,
                makeSensorInfo("BMP280 Pressure", SensorType::PRESSURE, 1100.f, 0.01f, 0.0027f,
                               50000, 1000000, false),
                iio + "/in_pressure_input", 10.f /* kPa -> hPa */));
        if (!haveTemperature) {
            hal.addSensor(std::make_shared<OnChangeSysfsSensor>(
                    cb,
                    makeSensorInfo("BMP280 Temperature", SensorType::AMBIENT_TEMPERATURE, 85.f,
                                   0.01f, 0.0027f, 1000000, 10000000, true),
                    iio + "/in_temp_input", 0.001f));
        }
        break;
    }

    ALOGI("%zu sensors registered", hal.sensorCount());
}

}  // namespace spacemit
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
