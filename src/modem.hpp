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
        AT_TIMEOUT,
        CONFIGURE_TEXT_MODE,
        CONFIGURE_TEXT_MODE_TIMEOUT,
        SMS_BEGIN,
        SMS_TEXT,
        SMS_END,
        SMS_END_TIMEOUT,
        POWER_OFF,
        END_SUCCESS,
        END_FAILED,
        NO_TIMEOUT
    } ModemState;

    uint8_t rxBuffer[256];
    uint32_t rxBufferLen = 0;
    ModemState modemState = IDLE;
    ModemState modemNextState = IDLE;
    ModemState modemTimeoutState = IDLE;
    const char *expectText = NULL;
    bool validateTimeout = false;

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
        if (Serial1.available()) {
            char rx = Serial1.read();
            if (rxBufferLen == 0 && (rx == '\r' || rx == '\n' || rx < 0x20 || rx > 0x7e)) {
                loggif("RX[%02X][%c]\n", rx, (rx < 0x20 || rx > 0x7e) ? '#' : rx);
            } else if (rxBufferLen != 0 && (rx == '\r' || rx == '\n')) {
                loggif("RX[%02X][%c]\n", rx, (rx < 0x20 || rx > 0x7e) ? '#' : rx);
                rxBuffer[rxBufferLen] = '\0';
                onRead((const char *) rxBuffer);
                rxBufferLen = 0;
            } else {
                loggif("RX[%02X][%c]\n", rx, (rx < 0x20 || rx > 0x7e) ? ' ' : rx);
                rxBuffer[rxBufferLen] = rx;
                rxBufferLen++;
            }
        }
    }

    void onRead(const char *message) {
        loggif("%s\n", message);
        if (!strncmp(expectText, (const char *) message, strlen(expectText))) {
            stateMachine((uint8_t *) message, strlen(message), false);
        }
    }

    void write(const char *message, const char *end = "\n") {
        while (!Serial1.availableForWrite());
        if (message) {
            Serial1.write(message);
        }
        if (end) {
            Serial1.write(end);
        }
    }

    void write(uint8_t num, char end = '\n') {
        while (!Serial1.availableForWrite());
        if (num) {
            Serial1.write(num);
        }
        if (end) {
            Serial1.write(end);
        }
    }

    static void staticReceiveSerial() {
        getInstance().receiveSerial();
    }

    void receiveSerial() {
        read();
    }

    static void staticStateMachineTimeoutHandler() {
        getInstance().stateMachineTimeoutHandler();
    }

    void stateMachineTimeoutHandler() {
        if (stateMachineTimeout && stateMachineTimestamp && AppTimer::getInstance().millisPassed(stateMachineTimestamp) >= stateMachineTimeout) {
            stateMachineTimeout = 0;
            stateMachine(NULL, 0, true);
        }
    }

    void sendSms(const char *number, const char *message, void (*callback)(bool status)) {
        onSmsSentCb = callback;
        smsNumber = number;
        smsMessage = message;
        nextState(START, 1000);
    }

    void endState() {
        modemNextState = IDLE;
        modemTimeoutState = IDLE;
        stateMachineTimestamp = 0;
        stateMachineTimeout = 0;
    }

    const char *getStateName(ModemState state) {
        switch (state) {
            case IDLE:
                return ("IDLE");
            case START:
                return ("START");
            case POWER_ON:
                return ("POWER_ON");
            case AT:
                return ("AT");
            case AT_TIMEOUT:
                return ("AT_TIMEOUT");
            case CONFIGURE_TEXT_MODE:
                return ("CONFIGURE_TEXT_MODE");
            case CONFIGURE_TEXT_MODE_TIMEOUT:
                return ("CONFIGURE_TEXT_MODE_TIMEOUT");
            case SMS_BEGIN:
                return ("SMS_BEGIN");
            case SMS_TEXT:
                return ("SMS_TEXT");
            case SMS_END:
                return ("SMS_END");
            case SMS_END_TIMEOUT:
                return ("SMS_END_TIMEOUT");
            case POWER_OFF:
                return ("POWER_OFF");
            case END_SUCCESS:
                return ("END_SUCCESS");
            case END_FAILED:
                return ("END_FAILED");
            case NO_TIMEOUT:
                return ("NO_TIMEOUT");
            default:
                return ("N/A");
        }
    }

    void nextState(ModemState state, uint64_t timeout = 0, ModemState timeoutState = NO_TIMEOUT, const char *expect = NULL) {
        modemNextState = state;
        modemTimeoutState = timeoutState;
        stateMachineTimestamp = AppTimer::getInstance().getMillis();
        stateMachineTimeout = timeout;
        expectText = expect;
        printState();
    }

    void stateMachine(uint8_t *buffer, uint32_t len, bool timeout = false) {
        if (timeout && modemTimeoutState != NO_TIMEOUT) {
            modemState = modemTimeoutState;
        } else {
            modemState = modemNextState;
        }

        loggif("%s\n", getStateName(modemState));

        switch (modemState) {
            case IDLE:
                return;
            case NO_TIMEOUT:
                return;
            case START:
                nextState(POWER_ON, 1000);
                break;
            case POWER_ON:
                setPower(1);
                nextState(AT, 5000);
                break;
            case AT:
                write("AT");
                nextState(CONFIGURE_TEXT_MODE, 1000, AT_TIMEOUT, "OK");
                break;
            case AT_TIMEOUT:
                setPower(false);
                nextState(END_FAILED, 1000);
                break;
            case CONFIGURE_TEXT_MODE:
                write("AT+CMGF=1");
//                nextState(SMS_BEGIN, 20000, CONFIGURE_TEXT_MODE, "SMS Ready");
                nextState(SMS_BEGIN, 20000, CONFIGURE_TEXT_MODE, "OK");
                break;
            case CONFIGURE_TEXT_MODE_TIMEOUT:
                setPower(false);
                nextState(END_FAILED, 1000);
                break;
            case SMS_BEGIN:
                write("AT+CMGS=\"");
                write(smsNumber);
                write("\"");
                nextState(SMS_TEXT, 1000);
                break;
            case SMS_TEXT:
                write(smsMessage);
                nextState(SMS_END, 1000);
                break;
            case SMS_END:
                write(26, '\0');
                nextState(POWER_OFF, 120000, SMS_END_TIMEOUT, "+CMGS");
                break;
            case SMS_END_TIMEOUT:
                setPower(false);
                nextState(END_FAILED, 1000);
                break;
            case POWER_OFF:
                setPower(false);
                nextState(END_SUCCESS, 2000);
                break;
            case END_SUCCESS:
                if (onSmsSentCb) {
                    onSmsSentCb(true);
                    onSmsSentCb = NULL;
                }
                endState();
                break;
            case END_FAILED:
                if (onSmsSentCb) {
                    onSmsSentCb(false);
                    onSmsSentCb = NULL;
                }
                endState();
                break;
        }
    }

    void printState() {
        loggif("%s -> %s timeout %s in %lu ms, millisPassed %lu ms\n", getStateName(modemState), getStateName(modemNextState), getStateName(modemTimeoutState), (uint32_t) stateMachineTimeout, (uint32_t) (AppTimer::getInstance().millisPassed(stateMachineTimestamp)));
    }
};

#endif