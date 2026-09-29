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

// From the AOSP example HAL; sampling starts in the subclass and failed reads post nothing.

#define LOG_TAG "sensors-spacemit"

#include "sensors-impl/Sensor.h"

#include <log/log.h>
#include <sched.h>
#include <utils/SystemClock.h>

#include <cmath>
#include <cstring>

using ::ndk::ScopedAStatus;

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {

Sensor::Sensor(ISensorsEventCallback* callback)
    : mIsEnabled(false),
      mSamplingPeriodNs(0),
      mLastSampleTimeNs(0),
      mCallback(callback),
      mMode(OperationMode::NORMAL) {}

Sensor::~Sensor() {
    {
        std::lock_guard<std::mutex> lock(mRunMutex);
        mStopThread = true;
        mIsEnabled = false;
        mWaitCV.notify_all();
    }
    if (mRunThread.joinable()) mRunThread.join();
}

void Sensor::start() {
    mSamplingPeriodNs = static_cast<int64_t>(mSensorInfo.minDelayUs) * 1000;
    mRunThread = std::thread(startThread, this);
}

const Sensor::SensorInfo& Sensor::getSensorInfo() const {
    return mSensorInfo;
}

void Sensor::batch(int64_t samplingPeriodNs) {
    if (samplingPeriodNs < mSensorInfo.minDelayUs * 1000LL) {
        samplingPeriodNs = mSensorInfo.minDelayUs * 1000LL;
    } else if (samplingPeriodNs > mSensorInfo.maxDelayUs * 1000LL) {
        samplingPeriodNs = mSensorInfo.maxDelayUs * 1000LL;
    }

    std::lock_guard<std::mutex> lock(mRunMutex);
    if (mSamplingPeriodNs != samplingPeriodNs) {
        mSamplingPeriodNs = samplingPeriodNs;
        // Wake up the 'run' thread to check if a new event should be generated now
        mWaitCV.notify_all();
    }
}

void Sensor::activate(bool enable) {
    std::lock_guard<std::mutex> lock(mRunMutex);
    if (mIsEnabled != enable) {
        mIsEnabled = enable;
        mWaitCV.notify_all();
    }
}

ScopedAStatus Sensor::flush() {
    // Only generate a flush complete event if the sensor is enabled and if the sensor is not a
    // one-shot sensor.
    if (!mIsEnabled ||
        (mSensorInfo.flags & static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_ONE_SHOT_MODE))) {
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(BnSensors::ERROR_BAD_VALUE));
    }

    // No batching (fifoMaxEventCount = 0): nothing is buffered, report completion right away.
    Event ev;
    ev.sensorHandle = mSensorInfo.sensorHandle;
    ev.sensorType = SensorType::META_DATA;
    EventPayload::MetaData meta = {
            .what = MetaDataEventType::META_DATA_FLUSH_COMPLETE,
    };
    ev.payload.set<EventPayload::Tag::meta>(meta);
    std::vector<Event> evs{ev};
    mCallback->postEvents(evs, isWakeUpSensor());

    return ScopedAStatus::ok();
}

void Sensor::startThread(Sensor* sensor) {
    sensor->run();
}

void Sensor::run() {
    std::unique_lock<std::mutex> runLock(mRunMutex);
    constexpr int64_t kNanosecondsInSeconds = 1000 * 1000 * 1000;

    struct sched_param params;
    // Set the thread to the lowest real-time priority.
    params.sched_priority = 1;
    if (sched_setscheduler(/*pid=*/0, SCHED_FIFO, &params)) {
        ALOGW("Unable to set SCHED_FIFO: error %s", strerror(errno));
    }
    bool sampling = false;
    while (!mStopThread) {
        const bool wantSampling = mIsEnabled && mMode == OperationMode::NORMAL;
        if (wantSampling != sampling) {
            sampling = wantSampling;
            onEnabled(sampling);
            mLastSampleTimeNs = 0;
        }
        if (!sampling) {
            mWaitCV.wait(runLock, [&] {
                return ((mIsEnabled && mMode == OperationMode::NORMAL) || mStopThread);
            });
        } else {
            timespec curTime;
            clock_gettime(CLOCK_BOOTTIME, &curTime);
            int64_t now = (curTime.tv_sec * kNanosecondsInSeconds) + curTime.tv_nsec;
            int64_t nextSampleTime = mLastSampleTimeNs + mSamplingPeriodNs;

            if (now >= nextSampleTime) {
                mLastSampleTimeNs = now;
                nextSampleTime = mLastSampleTimeNs + mSamplingPeriodNs;
                std::vector<Event> events = readEvents();
                if (!events.empty()) mCallback->postEvents(events, isWakeUpSensor());
            }

            mWaitCV.wait_for(runLock, std::chrono::nanoseconds(nextSampleTime - now));
        }
    }
    if (sampling) onEnabled(false);
}

bool Sensor::isWakeUpSensor() {
    return mSensorInfo.flags & static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_WAKE_UP);
}

std::vector<Sensor::Event> Sensor::readEvents() {
    std::vector<Event> events;
    Event event;
    event.sensorHandle = mSensorInfo.sensorHandle;
    event.sensorType = mSensorInfo.type;
    event.timestamp = ::android::elapsedRealtimeNano();
    memset(&event.payload, 0, sizeof(event.payload));
    if (readEventPayload(event.payload)) events.push_back(event);
    return events;
}

void Sensor::setOperationMode(OperationMode mode) {
    std::lock_guard<std::mutex> lock(mRunMutex);
    if (mMode != mode) {
        mMode = mode;
        mWaitCV.notify_all();
    }
}

bool Sensor::supportsDataInjection() const {
    return mSensorInfo.flags & static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_DATA_INJECTION);
}

ScopedAStatus Sensor::injectEvent(const Event& event) {
    if (event.sensorType == SensorType::ADDITIONAL_INFO) {
        // When in OperationMode::NORMAL, SensorType::ADDITIONAL_INFO is used to push operation
        // environment data into the device.
        return ScopedAStatus::ok();
    }

    if (!supportsDataInjection()) {
        return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    if (mMode == OperationMode::DATA_INJECTION) {
        mCallback->postEvents(std::vector<Event>{event}, isWakeUpSensor());
        return ScopedAStatus::ok();
    }

    return ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(BnSensors::ERROR_BAD_VALUE));
}

OnChangeSensor::OnChangeSensor(ISensorsEventCallback* callback)
    : Sensor(callback), mPreviousEventSet(false) {}

void OnChangeSensor::activate(bool enable) {
    Sensor::activate(enable);
    if (!enable) {
        mPreviousEventSet = false;
    }
}

std::vector<Sensor::Event> OnChangeSensor::readEvents() {
    std::vector<Event> events = Sensor::readEvents();
    std::vector<Event> outputEvents;

    for (auto iter = events.begin(); iter != events.end(); ++iter) {
        Event ev = *iter;
        if (!mPreviousEventSet ||
            memcmp(&mPreviousEvent.payload, &ev.payload, sizeof(ev.payload)) != 0) {
            outputEvents.push_back(ev);
            mPreviousEvent = ev;
            mPreviousEventSet = true;
        }
    }
    return outputEvents;
}

}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
