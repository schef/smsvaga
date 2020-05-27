#ifndef __VAGA_HPP__
#define __VAGA_HPP__

#include "Arduino.h"
#include "logger.hpp"
#include "HX711.h"

// y = mx + b
// m = (y - b) / x

// y is the actual weight in whatever units you want (g, kg, oz, etc)
// x is the raw value from the HX711 - from scale.read_average()
// m is your slope (multiplier)
// b is your intersection (offset) - also from scale.read_average() but with no weight, or using scale.tare()
// 4000 is 0.047 % of full scale

class Vaga {
private:
    HX711 scale;
    long y = 1000; // (vaga s teretom u mjernoj jedinici)
    long b = 569712; // (raw prazne vage) 569712
    long x = 598408; // (raw vage s teretom) 598408
    long m = 0; // (formula)

    Vaga() {
        loggif("\n");
        scale.begin(20, 21, 128);
    }

public:
    static Vaga &getInstance() {
        static Vaga instance;
        return instance;
    }

    long readMedian() {
        std::vector<long> readings;
        for (uint32_t i = 0; i < 10; i++) {
            long reading = scale.read();
            loggif("[%lu][%ld]\n", i, reading);
            readings.push_back(reading);
        }
        std::sort(readings.begin(), readings.end());
        return readings[5];
    }

    float read() {
        auto reading = (readMedian() - b) / m;
        loggif("[%ld]g\n", reading);
        return reading;
    }

    void getX() {
        x = readMedian();
        loggif("[%ld]\n", x);
    }

    void getB() {
        b = readMedian();
        loggif("[%ld]\n", b);
    }

    void calcM() {
        m = (y - b) / x;
        loggif("[%ld]\n", m);
    }

    long getUnit(long raw) {
        return (raw - b) / m;
    }

    void autoCali() {
        long rawValue = readMedian();
        m = 0.0F;
        auto unit = getUnit(rawValue);
        loggif("raw[%ld], unit[%ld]\n", rawValue, unit);
        if (unit < 0) {
            loggif("start from -\n");
        } else if (unit > 0) {
            loggif("start from +\n");
        } else {
            loggif("start from 0\n");
        }
    }

    void timerCallback() {
    }

    static void staticTimerCallback() {
        getInstance().timerCallback();
    }
};

#endif