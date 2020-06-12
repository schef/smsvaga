#include "Arduino.h"
#include "StandardCplusplus.h"
#include "logger.hpp"
#include "app_timer.hpp"
#include "modem.hpp"
#include "vaga.hpp"
#include "console.hpp"
#include "console_test.hpp"
#include <avr/wdt.h>

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

#define TELEFON "+385912895203"
static uint32_t retryCount = 0;
static const uint32_t retryCountMax = 3;

void onSms(bool status);

void sendSms() {
    static char buffer[256];
    snprintf(buffer, sizeof(buffer), "Stanje vage %sg\n", f2str(Vaga::getInstance().read()));
    Modem::getInstance().sendSms(TELEFON, buffer, onSms);
}

void onSms(bool status) {
    if (!status) {
        if (retryCount < retryCountMax) {
            retryCount++;
            sendSms();
        } else {
            loggif("Send failed\n");
            retryCount = 0;
        }
    } else {
        loggif("Send successful\n");
        retryCount = 0;
    }
}

void testTimer() {
    static uint64_t timestamp = AppTimer::getInstance().getMillis();
    static bool firstTime = true;
    static uint64_t timeoutFirst = 10L * 60L * 1000L;
    static uint64_t timeoutNext = 24L * 60L * 60L * 1000L;
    if (firstTime && AppTimer::getInstance().millisPassed(timestamp) >= timeoutFirst) {
        loggif("first time\n");
        firstTime = false;
        loggif("%lu timestamp, millispassed %lu, timeout %lu\n", (uint32_t) AppTimer::getInstance().getMillis(), (uint32_t) AppTimer::getInstance().millisPassed(timestamp), (uint32_t) timeoutFirst);
        timestamp = AppTimer::getInstance().getMillis();
        sendSms();

    } else if (AppTimer::getInstance().millisPassed(timestamp) >= timeoutNext) {
        loggif("next time\n");
        loggif("%lu timestamp, millispassed %lu, timeout %lu\n", (uint32_t) AppTimer::getInstance().getMillis(), (uint32_t) AppTimer::getInstance().millisPassed(timestamp), (uint32_t) timeoutNext);
        timestamp = AppTimer::getInstance().getMillis();
        sendSms();

    }
}

void watchDogFeed() {
    wdt_reset();
}

void setup() {
    loggif("start\n");
    AppTimer::getInstance();
    AppTimer::getInstance().setAltertMillisPassed(100);
    Console::getInstance();
    ConsoleTest::registerCommands();
    Modem::getInstance();
    Vaga::getInstance();

    Modem::getInstance().setPower(0);

    AppTimer::getInstance().registerCallback("keepAliveMessage", 60000L, keepAliveMessage);
    AppTimer::getInstance().registerCallback("watchDogFeed", 1000L, watchDogFeed);
    AppTimer::getInstance().registerCallback("ledBlink", 500, ledBlink);
    AppTimer::getInstance().registerCallback("Console::receiveSerial", 10, Console::receiveSerial);
    AppTimer::getInstance().registerCallback("Modem::receiveSerial", 10, Modem::staticReceiveSerial);
    AppTimer::getInstance().registerCallback("Modem::stateMachineTimeoutHandler", 500, Modem::staticStateMachineTimeoutHandler);
    AppTimer::getInstance().registerCallback("testTimer", 1000, testTimer);
    AppTimer::getInstance().registerCallback("Vaga::staticTimerCallback", 10000, Vaga::staticTimerCallback, 10000);

    wdt_enable(WDTO_8S);
    loggif("end\n");
}

void loop() {
    AppTimer::getInstance().execute();
}