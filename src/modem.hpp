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
    }

public:

    static Modem &getInstance() {
        static Modem instance;
        return instance;
    }

    void init() {
        Serial1.begin(9600);
    }

    void setPower(bool state) {
        loggif("state[%d]\n", state);
        digitalWrite(17, !state);
    }

    void timerCallback() {
        static bool firstTime = true;
        if (firstTime) {
            firstTime = false;
            setPower(1);
        }

        read();

        static uint64_t writeTimestamp = AppTimer::getInstance().getMillis();
        if (AppTimer::getInstance().millisPassed(writeTimestamp) > 5000) {
            writeTimestamp = AppTimer::getInstance().getMillis();
            static uint8_t state = 0;
            switch (state) {
                case 0:
                    write("AT");
                    break;
                case 1:
                    write("AT+CSQ");
                    break;
                case 2:
                    write("at+cpin?");
                    break;
//                case 3:
//                    write("AT+CCID");
//                    break;
//                case 4:
//                    write("AT+CREG?");
//                    break;
//                case 5:
//                    write("AT+CMGF=1");
//                    break;
//                case 6:
//                    write("AT+CMGS=\"+385912895203\"");
//                    break;
//                case 7:
//                    write("DELA!");
//                    break;
//                case 8:
//                    write(0x1A);
//                    break;
                default:
                    break;
            }
            state++;
        }
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

    static void staticTimerCallback() {
        getInstance().timerCallback();
    }
};

#endif