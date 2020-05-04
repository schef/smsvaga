#ifndef __MODEM_HPP__
#define __MODEM_HPP__

#include "Arduino.h"
#include <SoftwareSerial.h>
#include "logger.hpp"

class Modem {
private:
    uint8_t rxBuffer[256];
    uint32_t rxBufferLen = 0;

    Modem() {
        loggif("\n");
    }

public:

    static Modem &getInstance() {
        static Modem instance;
        return instance;
    }

    void init() {
        Serial1.begin(115200);
    }

    void timerCallback() {
        read();

        static uint64_t writeTimestamp = AppTimer::getInstance().getMillis();
        if (AppTimer::getInstance().millisPassed(writeTimestamp) > 5000) {
            writeTimestamp = AppTimer::getInstance().getMillis();
            write();
        }
    }

    void read() {
        read(NULL);
    }

    uint32_t read(uint8_t *buffer) {
        if (Serial1.available() > 0) {
            rxBuffer[rxBufferLen] = Serial1.read();
        }
        uint32_t len = 0;
        if (rxBufferLen) {
            loggif("");
            loggbln(rxBuffer, rxBufferLen, '\0');
            if (buffer != NULL) {
                memcpy(rxBuffer, buffer, rxBufferLen);
                len = rxBufferLen;
            }
            rxBufferLen = 0;
        }
        return len;
    }

    void write() {
        loggif("\n");
        Serial1.print("\r\nAT\r\n");
    }

    static void staticTimerCallback() {
        getInstance().timerCallback();
    }
};

#endif