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
    long x = 1227; // (vaga s teretom u mjernoj jedinici)
    long b = 569947; // (raw prazne vage) 569712
    long y = 598612; // (raw vage s teretom) 598408
    float m = 23.3618F; // (formula)

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
        loggif("[%s]g\n", f2str(reading));
        return reading;
    }

    void getY() {
        y = readMedian();
        loggif("[%ld]\n", y);
    }

    void getB() {
        b = readMedian();
        loggif("[%ld]\n", b);
    }

    void calcM() {
        m = (float)(y - b) / (float)x;
        loggif("[%s]\n", f2str(m));
    }

    void autoCali() {
    }

    void timerCallback() {
    }

    static void staticTimerCallback() {
        getInstance().timerCallback();
    }
};

#endif