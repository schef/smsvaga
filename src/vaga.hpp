#ifndef __VAGA_HPP__
#define __VAGA_HPP__

#include "Arduino.h"
#include "logger.hpp"
#include "HX711.h"

class Vaga {
private:
    HX711 scale;
    float calibrationFactor = .0F;

    Vaga() {
        loggif("\n");
        scale.begin(20, 21);
        scale.set_scale(calibrationFactor);
    }

public:
    static Vaga &getInstance() {
        static Vaga instance;
        return instance;
    }

    void calibrate(long offset = 0) {
        calibrationFactor += offset;
        scale.set_scale(calibrationFactor); //Adjust to this calibration factor
        loggif("Reading[%s]kg, calibrationFactor[%s]\n", f2str(scale.get_units()), f2str(calibrationFactor, 1));
    }

    void autoCalibrate() {
    }

    long readAverage() {
        auto reading = scale.read_average();
        loggif("[%lu]kg\n", reading);
        return reading;
    }

    long read() {
        auto reading = scale.read();
        loggif("[%lu]kg\n", reading);
        return reading;
    }

    void tare() {
        scale.tare();
    }

    void timerCallback() {
    }

    static void staticTimerCallback() {
        getInstance().timerCallback();
    }
};

#endif