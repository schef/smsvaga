#include "Arduino.h"
#include "StandardCplusplus.h"
#include "logger.hpp"
#include "app_timer.hpp"
#include "modem.hpp"
#include "vaga.hpp"
#include "console.hpp"
#include "console_test.hpp"

static void keepAliveMessage() {
    static uint32_t powerManagerCounter = 0;
    static uint32_t aliveMessageCounter = 0;
    aliveMessageCounter++;
    loggif("%d, powerManage %d\n", aliveMessageCounter, powerManagerCounter);
    powerManagerCounter = 0;
}

static void ledBlink() {
    static bool init = false;
    static bool state = false;
    if (!init) {
        init = true;
        pinMode(LED_BUILTIN, OUTPUT);
    }
    digitalWrite(LED_BUILTIN, state);
    state = !state;
}

void onSms(bool status);

void sendSms() {
    loggif("\n");
    char message[128];
    sprintf(message, "DELA %llu", (uint32_t)AppTimer::getInstance().getMillis());
    Modem::getInstance().sendSms("+385912895203", message, onSms);
}

void onSms(bool status) {
    loggif("%s\n", status ? "true" : "false");
    static uint32_t retry = 0;
    if (!status && retry < 2) {
        retry++;
        sendSms();
    } else {
        retry = 0;
    }
}

void smsTimer() {
    static uint64_t timestamp = AppTimer::getInstance().getMillis();
    if (AppTimer::getInstance().millisPassed(timestamp) >= 2 * 60 * 60 * 1000) {
        timestamp = AppTimer::getInstance().getMillis();
        sendSms();
    }
}

void setup() {
    loggif("start\n");
    AppTimer::getInstance();
    AppTimer::getInstance().setAltertMillisPassed(100);
    Console::getInstance();
    ConsoleTest::registerCommands();
    Modem::getInstance();
//    Vaga::getInstance();

    AppTimer::getInstance().registerCallback("keepAliveMessage", 5000, keepAliveMessage);
    AppTimer::getInstance().registerCallback("ledBlink", 500, ledBlink);
    AppTimer::getInstance().registerCallback("Console::receiveSerial", 10, Console::receiveSerial);
    AppTimer::getInstance().registerCallback("Modem::receiveSerial", 10, Modem::staticTimerCallback);
    AppTimer::getInstance().registerCallback("smsTimer", 1000, smsTimer);
//    AppTimer::getInstance().registerCallback("Vaga::staticTimerCallback", 10000, Vaga::staticTimerCallback, 10000);
    loggif("end\n");
}

void loop() {
    AppTimer::getInstance().execute();
}