#ifndef __APP_TIMER_HPP__
#define __APP_TIMER_HPP__

#include "Arduino.h"
#include "vector"

#define OVERFLOW (uint32_t)(0xffffffff)

class AppTimer {
private:
    typedef struct {
        const char *name;
        uint64_t timestamp;
        uint64_t timeout;
        uint64_t postponeStart;
        bool suspended;

        void (*callback)() = NULL;
    } RepetativeCallback;

    typedef struct {
        const char *name;
        uint64_t timestamp;
        uint64_t timeout;
        uint8_t *buffer;
        uint32_t bufferLen;

        void (*callback)(uint8_t *buffer, uint32_t len) = NULL;
    } AsyncDelay;

    typedef struct {
        const char *name;
        uint64_t timestamp;
        uint64_t timeout;
        uint64_t postponeStart;
        uint8_t *buffer;
        uint32_t bufferLen;
        bool inProgress;

        void (*startCallback)(uint8_t *buffer, uint32_t len) = NULL;

        void (*endCallback)(bool status, uint8_t *buffer, uint32_t len) = NULL;
    } AsyncFunction;

    uint32_t _overflowCount = 0;
    uint32_t _previousRealTimeMillis = 0;
    uint64_t _alterMillisPassed = 2;
    std::vector<RepetativeCallback> repetativeCallbacks;
    std::vector<AsyncDelay> asyncDelays;
    std::vector<AsyncFunction> asyncFunctions;

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

    void setAltertMillisPassed(uint64_t millisPassed) {
        _alterMillisPassed = millisPassed;
    }

    void executeCallbacks() {
        for (uint32_t i = 0; i < repetativeCallbacks.size(); i++) {
            RepetativeCallback &rc = repetativeCallbacks[i];
            if (!rc.suspended && millisPassed(rc.timestamp) >= rc.timeout) {
                rc.timestamp = getMillis();
                if (rc.callback) {
                    rc.callback();
                }
                uint64_t miliPassed = millisPassed(rc.timestamp);
                if (miliPassed >= _alterMillisPassed) {
                    loggif("callback %s has duration of %d ms\n", rc.name, (uint32_t) miliPassed);
                }
            }
        }
    }

    void executeAsyncDelay() {
        for (uint8_t i = 0; i < asyncDelays.size(); i++) {
            AsyncDelay &ad = asyncDelays[i];
            if (millisPassed(ad.timestamp) >= ad.timeout) {
                if (ad.callback) {
                    ad.callback(ad.buffer, ad.bufferLen);
                }
                delete[] ad.buffer;
                asyncDelays.erase(asyncDelays.begin() + i);
                break;
            }
        }
    }


    void executeAsyncFunction() {
        for (uint8_t i = 0; i < asyncFunctions.size(); i++) {
            AsyncFunction &af = asyncFunctions[i];
            if (!af.inProgress && millisPassed(af.timestamp) >= af.postponeStart) {
                loggif("%s start\n", af.name);
                af.timestamp = getMillis();
                af.inProgress = true;
                if (af.startCallback) {
                    af.startCallback(af.buffer, af.bufferLen);
                }
            } else {
                if (millisPassed(af.timestamp) >= af.timeout) {
                    loggif("%s end: TIMEOUT %d\n", af.name, (uint32_t) af.timeout);
                    if (af.endCallback) {
                        af.endCallback(false, af.buffer, af.bufferLen);
                    }
                    delete[] af.buffer;
                    asyncFunctions.erase(asyncFunctions.begin() + i);
                    break;
                }
            }
        }
    }

    void execute() {
        uint64_t startTimestamp = getMillis();
        executeCallbacks();
        executeAsyncDelay();
        executeAsyncFunction();
        uint64_t milisPassed = millisPassed(startTimestamp);
        if (milisPassed > _alterMillisPassed) {
            loggif("total callback duration of %d ms\n", (uint32_t) milisPassed);
        }
    }

    void registerCallback(const char *name, uint64_t timeout, void (*callback)(), uint64_t postponeStart = 0) {

        RepetativeCallback rc;
        rc.name = name;
        rc.timestamp = getMillis();
        rc.timeout = timeout;
        rc.postponeStart = postponeStart;
        rc.suspended = false;
        rc.callback = callback;

        repetativeCallbacks.push_back(rc);

        loggif("repeat %s every %d ms after %d ms\n", name, (uint32_t) timeout, (uint32_t) postponeStart);
    }

    void asyncDelay(const char *name, uint64_t timeout, void (*callback)(uint8_t *buffer, uint32_t len),
                    uint8_t *buffer = NULL, uint32_t bufferLen = 0) {
        AsyncDelay ad;
        ad.name = name;
        ad.timestamp = getMillis();
        ad.timeout = timeout;
        ad.buffer = new uint8_t[bufferLen];
        ad.bufferLen = bufferLen;
        memcpy(ad.buffer, buffer, bufferLen);
        ad.callback = callback;

        asyncDelays.push_back(ad);

        loggif("exec %s after %d ms\n", name, (uint32_t) timeout);
    }

    void suspendCallback(const char *name, bool suspended) {
        for (uint32_t i = 0; i < repetativeCallbacks.size(); i++) {
            RepetativeCallback &rc = repetativeCallbacks[i];
            if (strcmp(rc.name, name) == 0) {
                rc.suspended = suspended;
                loggif("%s %s\n", name, suspended ? "suspended" : "resumed");
                break;
            }
        }
    }

    void changeCallbackTimeout(const char *name, uint64_t timeout) {
        for (uint32_t i = 0; i < repetativeCallbacks.size(); i++) {
            RepetativeCallback &rc = repetativeCallbacks[i];
            if (strcmp(rc.name, name) == 0) {
                rc.timeout = timeout;
                loggif("%s timeout set to %d\n", name, (uint32_t) timeout);
                break;
            }
        }
    }


    void registerAsyncFunction(const char *name, uint64_t timeout,
                               void(*startCallback)(uint8_t *buffer, uint32_t len),
                               void(*endCallback)(bool status, uint8_t *buffer, uint32_t len),
                               uint8_t *buffer = NULL, uint32_t bufferLen = 0, uint64_t postponeStart = 0) {

        AsyncFunction func;
        func.name = name;
        func.timestamp = getMillis();
        func.timeout = timeout;
        func.postponeStart = postponeStart;
        func.buffer = new uint8_t[bufferLen];
        func.bufferLen = bufferLen;
        memcpy(func.buffer, buffer, bufferLen);
        func.startCallback = startCallback;
        func.endCallback = endCallback;
        func.inProgress = false;

        asyncFunctions.push_back(func);
        loggif("wait for %s with timeout %d ms after %d ms\n", name, (uint32_t) timeout, (uint32_t) postponeStart);
    }


    void deregisterAsyncFunction(void(*startCallback)(uint8_t *buffer, uint32_t len),
                                 void(*endCallback)(bool status, uint8_t *buffer, uint32_t len)) {
        for (uint32_t i = 0; i < asyncFunctions.size(); i++) {
            AsyncFunction &af = asyncFunctions[i];
            if (af.startCallback == startCallback && af.endCallback == endCallback) {
                loggif("%s\n", af.name);
                delete[] af.buffer;
                asyncFunctions.erase(asyncFunctions.begin() + i);
                break;
            }
        }
    }

    void deregisterAsyncFunction(const char *name) {
        for (uint32_t i = 0; i < asyncFunctions.size(); i++) {
            AsyncFunction &af = asyncFunctions[i];
            if (strcmp(af.name, name) == 0) {
                loggif("%s\n", af.name);
                delete[] af.buffer;
                asyncFunctions.erase(asyncFunctions.begin() + i);
                break;
            }
        }
    }

    bool isRegisteredAsyncFunction(void(*startCallback)(uint8_t *buffer, uint32_t len),
                                   void(*endCallback)(bool status, uint8_t *buffer, uint32_t len)) {
        bool found = false;
        for (uint32_t i = 0; i < asyncFunctions.size(); i++) {
            AsyncFunction &af = asyncFunctions[i];
            if (af.startCallback == startCallback && af.endCallback == endCallback) {
                found = true;
                break;
            }
        }
        return found;
    }

    bool isRegisteredAsyncFunction(const char *name) {
        bool found = false;
        for (uint32_t i = 0; i < asyncFunctions.size(); i++) {
            AsyncFunction &af = asyncFunctions[i];
            if (strcmp(af.name, name) == 0) {
                found = true;
                break;
            }
        }
        return found;
    }

};

#endif