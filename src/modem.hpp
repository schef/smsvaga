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
        pinMode(17, OUTPUT);
        digitalWrite(17, 1);
        init();
    }

    void init() {
        Serial1.begin(9600);
    }

public:

    static Modem &getInstance() {
        static Modem instance;
        return instance;
    }

    void setPower(bool state) {
        loggif("state[%d]\n", state);
        digitalWrite(17, !state);
    }

    void read() {
        while (Serial1.available()) {
            rxBuffer[rxBufferLen] = Serial1.read();
            rxBufferLen++;
            if (rxBuffer[rxBufferLen - 2] == 0x0D && rxBuffer[rxBufferLen - 1] == 0x0A) {
                for (uint32_t i = 0; i < rxBufferLen; i++) {
                    uint8_t &c = rxBuffer[i];
                    if (c == 0x0D || c == 0x0A) {
                        c = 0;
                    }
                }
                if (rxBuffer[0] != 0) {
                    onRead((const char *) rxBuffer);
                }
                rxBufferLen = 0;
            }
        }
    }

    void onRead(const char *message) {
        loggif("%s\n", message);
    }

    void write(const char *message) {
        Serial1.println(message);
    }

    void write(uint8_t hex) {
        Serial1.write(hex);
    }

    static void receiveSerial() {
        getInstance().read();
    }
};

#endif