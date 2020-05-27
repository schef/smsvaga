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
        m = 1000;
        auto unit = getUnit(rawValue);
        auto lastUnit = unit;
        long step = 1;
        long operations = 0;
        while (unit != y && operations < 1000) {
            loggif("raw[%ld], unit[%ld], m[%ld], step[%ld]\n", rawValue, unit, m, step);
            if (unit < y) {
                loggif("start from -\n");
                m += step;
                unit = getUnit(rawValue);
                if (unit < lastUnit) { // away from zero
                    loggif("continue with --\n");
                    if (step > 0) {
                        step *= -1;
                    }
                } else if (unit > lastUnit) { // closer to zero
                    loggif("continue\n");
                } else if (unit == lastUnit && unit != y) { //to small step
                    loggif("to small step\n");
                    if (step < 0) {
                        step -= 1;
                    } else {
                        step += 1;
                    }
                } else { // already zero
                    loggif("its 0\n");
                    break;
                }
            } else if (unit > y) {
                loggif("start from +\n");
                m += 1;
                unit = getUnit(rawValue);
                if (unit < lastUnit) { // closer to zero
                    loggif("continue\n");
                } else if (unit > lastUnit) { // away from zero
                    loggif("continue with ++\n");
                    if (step < 0) {
                        step *= -1;
                    }
                } else if (unit == lastUnit && unit != y) { //to small step
                    loggif("to small step\n");
                    if (step < 0) {
                        step -= 1;
                    } else {
                        step += 1;
                    }
                } else { // already zero
                    loggif("its from 0\n");
                    break;
                }
            } else {
                loggif("its from 0\n");
                break;
            }
            if (lastUnit > y && unit < y) {
                loggif("step to big\n");
                if (step < 0) {
                    step += 1;
                } else {
                    step -= 1;
                }
            } else if (lastUnit < y && unit > y) {
                loggif("step to big\n");
                if (step < 0) {
                    step += 1;
                } else {
                    step -= 1;
                }
            }
            lastUnit = unit;
            operations++;
        }
        loggif("raw[%ld], unit[%ld], m[%ld]\n", rawValue, unit, m);
    }

    void timerCallback() {
    }

    static void staticTimerCallback() {
        getInstance().timerCallback();
    }
};

#endif