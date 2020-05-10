#ifndef __MODEM_HPP__
#define __MODEM_HPP__

#include "Arduino.h"
#include <SoftwareSerial.h>
#include "logger.hpp"
#include "TinyGsmClientSIM800.h"

class Modem {
private:
    typedef enum {
        IDLE,
        START,
        POWER_ON,
        AT,
        CONFIGURE_TEXT_MODE,
        SMS_BEGIN,
        SMS_TEXT,
        SMS_END,
        POWER_OFF,
        END_SUCCESS,
        END_FAILED
    } ModemState;

    uint8_t rxBuffer[256];
    uint32_t rxBufferLen = 0;
    ModemState modemState = IDLE;
    void (*onSmsSentCb)(bool status) = NULL;
    const char *smsNumber = NULL;
    const char *smsMessage = NULL;
    uint64_t stateMachineTimestamp = 0;
    uint64_t stateMachineTimeout = 0;

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
        stateMachine(true, (uint8_t*)message, strlen(message));
    }

    void write(const char *message, bool newLine = true) {
        while(!Serial1.availableForWrite());
        Serial1.write(message);
        if (newLine) {
            Serial1.write("\n");
        }
    }

    void write(uint8_t num, bool newLine = true) {
        while(!Serial1.availableForWrite());
        Serial1.write(num);
        if (newLine) {
            Serial1.write("\n");
        }
    }

    static void staticTimerCallback() {
        getInstance().timerCallback();
    }

    void timerCallback() {
        read();
        if (stateMachineTimeout && AppTimer::getInstance().millisPassed(stateMachineTimestamp) >= stateMachineTimeout) {
            stateMachineTimeout = 0;
            stateMachine(false, NULL, 0);
        }
    }

    void sendSms(const char *number, const char *message, void (*callback)(bool status)) {
        onSmsSentCb = callback;
        smsNumber = number;
        smsMessage = message;
        nextState(START, 1000);
    }

    void endState() {
        nextState(IDLE);
    }

    void nextState(ModemState state, uint64_t timeout = 0) {
        switch (state) {
            case IDLE:
                loggif("IDLE");
                break;
            case START:
                loggif("START");
                break;
            case POWER_ON:
                loggif("POWER_ON");
                break;
            case AT:
                loggif("AT");
                break;
            case CONFIGURE_TEXT_MODE:
                loggif("CONFIGURE_TEXT_MODE");
                break;
            case SMS_BEGIN:
                loggif("SMS_BEGIN");
                break;
            case SMS_TEXT:
                loggif("SMS_TEXT");
                break;
            case SMS_END:
                loggif("SMS_END");
                break;
            case POWER_OFF:
                loggif("POWER_OFF");
                break;
            case END_SUCCESS:
                loggif("END_SUCCESS");
                break;
            case END_FAILED:
                loggif("END_FAILED");
                break;
        }
        loggf(" in %d ms\n", (uint32_t)timeout);
        modemState = state;
        stateMachineTimestamp = AppTimer::getInstance().getMillis();
        stateMachineTimeout = timeout;
    }

    void stateMachine(bool status, uint8_t *buffer, uint32_t len) {
        switch (modemState) {
            case IDLE:
                return;
            case START:
                loggif("START\n");
                nextState(POWER_ON, 1000);
                break;
            case POWER_ON:
                loggif("POWER_ON\n");
                setPower(1);
                nextState(AT, 5000);
                break;
            case AT:
                loggif("AT\n");
                write("AT");
                nextState(CONFIGURE_TEXT_MODE, 1000);
                break;
            case CONFIGURE_TEXT_MODE:
                loggif("CONFIGURE_TEXT_MODE\n");
                if (status) {
                    if (!strcmp("OK", (const char*)buffer)) {
                        write("AT+CMGF=1");
                        nextState(SMS_BEGIN, 20000);
                    }
                } else {
                    loggif("TIMEOUT\n");
                    nextState(POWER_OFF);
                }
                break;
            case SMS_BEGIN:
                loggif("SMS_BEGIN\n");
                if (status) {
                    if (!strcmp("SMS Ready", (const char*)buffer)) {
                        write("AT+CMGS=\"", false);
                        write(smsNumber, false);
                        write("\"");
                        nextState(SMS_TEXT, 1000);
                    }
                } else {
                    loggif("TIMEOUT\n");
                    nextState(POWER_OFF);
                }
                break;
            case SMS_TEXT:
                loggif("SMS_TEXT\n");
                Modem::getInstance().write(smsMessage, false);
                nextState(SMS_END, 1000);
                break;
            case SMS_END:
                loggif("SMS_END\n");
                Modem::getInstance().write(26, false);
                nextState(POWER_OFF, 120000);
                break;
            case POWER_OFF:
                loggif("POWER_OFF\n");
                if (status) {
                    if (!strncmp("+CMGS", (const char*)buffer, 5)) {
                        setPower(false);
                        nextState(END_SUCCESS, 2000);
                    }
                } else {
                    loggif("TIMEOUT\n");
                    setPower(false);
                    nextState(END_FAILED, 1000);
                }
                break;
            case END_SUCCESS:
                loggif("END_SUCCESS\n");
                if (onSmsSentCb) {
                    onSmsSentCb(true);
                    onSmsSentCb = NULL;
                }
                endState();
                break;
            case END_FAILED:
                loggif("END_FAILED\n");
                if (onSmsSentCb) {
                    onSmsSentCb(false);
                    onSmsSentCb = NULL;
                }
                endState();
                break;
        }
    }
};

#endif