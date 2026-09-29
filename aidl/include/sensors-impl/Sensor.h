/*
 * Copyright (C) 2021 The Android Open Source Project
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

// Sensor base class from the AOSP example HAL, without its fake sensors.

#pragma once

#include <aidl/android/hardware/sensors/BnSensors.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {

class ISensorsEventCallback {
  public:
    using Event = ::aidl::android::hardware::sensors::Event;

    virtual ~ISensorsEventCallback() {}
    virtual void postEvents(const std::vector<Event>& events, bool wakeup) = 0;
};

class Sensor {
  public:
    using OperationMode = ::aidl::android::hardware::sensors::ISensors::OperationMode;
    using Event = ::aidl::android::hardware::sensors::Event;
    using EventPayload = ::aidl::android::hardware::sensors::Event::EventPayload;
    using SensorInfo = ::aidl::android::hardware::sensors::SensorInfo;
    using SensorType = ::aidl::android::hardware::sensors::SensorType;
    using MetaDataEventType =
            ::aidl::android::hardware::sensors::Event::EventPayload::MetaData::MetaDataEventType;

    explicit Sensor(ISensorsEventCallback* callback);
    virtual ~Sensor();

    const SensorInfo& getSensorInfo() const;
    void setHandle(int32_t handle) { mSensorInfo.sensorHandle = handle; }
    void batch(int64_t samplingPeriodNs);
    virtual void activate(bool enable);
    ndk::ScopedAStatus flush();

    void setOperationMode(OperationMode mode);
    bool supportsDataInjection() const;
    ndk::ScopedAStatus injectEvent(const Event& event);

  protected:
    // Starts the sampling thread; called by the subclass constructor once mSensorInfo is set.
    void start();
    void run();
    virtual std::vector<Event> readEvents();
    virtual bool readEventPayload(EventPayload& payload) = 0;
    // Called on the sampling thread when the sensor starts / stops being sampled.
    virtual void onEnabled(bool /*enabled*/) {}
    static void startThread(Sensor* sensor);

    bool isWakeUpSensor();

    bool mIsEnabled;
    int64_t mSamplingPeriodNs;
    int64_t mLastSampleTimeNs;
    SensorInfo mSensorInfo;

    std::atomic_bool mStopThread{false};
    std::condition_variable mWaitCV;
    std::mutex mRunMutex;
    std::thread mRunThread;

    ISensorsEventCallback* mCallback;

    OperationMode mMode;
};

// Reports only when the value changes (temperature, humidity).
class OnChangeSensor : public Sensor {
  public:
    explicit OnChangeSensor(ISensorsEventCallback* callback);

    void activate(bool enable) override;

  protected:
    std::vector<Event> readEvents() override;

    Event mPreviousEvent;
    bool mPreviousEventSet;
};

}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
