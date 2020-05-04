#ifndef __VAGA_HPP__
#define __VAGA_HPP__

#include "Arduino.h"
#include "logger.hpp"

class Vaga {
private:

    Vaga() {
        loggif("\n");
    }
public:
    static Vaga &getInstance() {
        static Vaga instance;
        return instance;
    }

    int readSensor(uint8_t pin) {
        int value = analogRead(pin);
        loggif("pin[%d][%d]\n", pin, value);
        return value;
    }

    void timerCallback() {
        readSensor(PIN_A0);
        readSensor(PIN_A1);
        readSensor(PIN_A2);
        readSensor(PIN_A3);
    }

    static void staticTimerCallback() {
        getInstance().timerCallback();
    }
};

#endif