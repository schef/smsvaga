#ifndef __APP_TIMER_HPP__
#define __APP_TIMER_HPP__

#include "Arduino.h"
#include "vector"

#define OVERFLOW (uint32_t)(0xffffffff)

class AppTimer {
private:
    struct TimerCallback {
        uint64_t startTimestamp;
        uint64_t minTimeout;
        uint64_t startOffset;
        const char *name;
        void (*callback)();
    };

    uint32_t _overflowCount = 0;
    uint32_t _previousRealTimeMillis = 0;
    uint64_t _alterMillisPassed = 2;
    std::vector<TimerCallback> _callbackList;

    AppTimer() {
        loggif("\n");
    }

public:

    static AppTimer &getInstance() {
        static AppTimer instance;
        return instance;
    }

    uint32_t getRealTimeMillis() {
        return millis();
    }

    void checkOverflow() {
        uint32_t currentRealTimeMillis = getRealTimeMillis();
        if (currentRealTimeMillis < _previousRealTimeMillis) {
            _overflowCount++;
            loggif("_overflowCount = %d, currentRealTimeMillis = %d, _previousRealTimeMillis = %d\n", _overflowCount,
                   currentRealTimeMillis, _previousRealTimeMillis);
        }
        _previousRealTimeMillis = currentRealTimeMillis;
    }

    uint64_t getMillis() {
        checkOverflow();
        return ((((uint64_t) _overflowCount * (uint64_t) OVERFLOW) + (uint64_t) getRealTimeMillis()));
    }

    uint64_t millisPassed(uint64_t localMillis) {
        return (getMillis() - localMillis);
    }

    void registerCallback(void (*onCallback)(), uint64_t minTimeout, const char *name, uint64_t startOffset = 0) {
        TimerCallback callback = {0};
        callback.startTimestamp = getInstance().getMillis();
        callback.minTimeout = minTimeout;
        callback.startOffset = startOffset;
        callback.callback = onCallback;
        callback.name = name;
        _callbackList.push_back(callback);
        loggif("registerCallback %s: %d ms\n", name, minTimeout);
    }

    void executeCallbacks() {
        for (auto & i : _callbackList) {
            TimerCallback &ref = i;
            if (ref.callback != nullptr && getMillis() >= ref.startOffset && millisPassed(ref.startTimestamp) >= ref.minTimeout) {
                ref.startTimestamp = getMillis();
                ref.callback();
                uint64_t miliPassed = millisPassed(ref.startTimestamp);
                if (miliPassed >= _alterMillisPassed) {
                    loggif("callback %s has duration of %d ms\n", ref.name, (uint32_t) miliPassed);
                }
            }
        }
    }

    void execute() {
        checkOverflow();
        executeCallbacks();
    }

    void setAltertMillisPassed(uint64_t millisPassed) {
        _alterMillisPassed = millisPassed;
    }
};

#endif