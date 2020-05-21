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

void testTimer() {
    static uint64_t timestamp = AppTimer::getInstance().getMillis();
    if (AppTimer::getInstance().millisPassed(timestamp) >= 2 * 60 * 60 * 1000L) {
        timestamp = AppTimer::getInstance().getMillis();
        loggif("\n");
    }
}

void setup() {
    loggif("start\n");
    AppTimer::getInstance();
    AppTimer::getInstance().setAltertMillisPassed(100);
    Console::getInstance();
    ConsoleTest::registerCommands();
    Modem::getInstance();
    Vaga::getInstance();

    AppTimer::getInstance().registerCallback("keepAliveMessage", 60000L, keepAliveMessage);
    AppTimer::getInstance().registerCallback("ledBlink", 500, ledBlink);
    AppTimer::getInstance().registerCallback("Console::receiveSerial", 10, Console::receiveSerial);
    AppTimer::getInstance().registerCallback("Modem::receiveSerial", 10, Modem::staticTimerCallback);
    AppTimer::getInstance().registerCallback("testTimer", 1000, testTimer);
    AppTimer::getInstance().registerCallback("Vaga::staticTimerCallback", 10000, Vaga::staticTimerCallback, 10000);
    loggif("end\n");
}

void loop() {
    AppTimer::getInstance().execute();
}