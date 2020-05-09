#include "Arduino.h"
#include "StandardCplusplus.h"
#include "logger.hpp"
#include "app_timer.hpp"
#include "modem.hpp"
#include "vaga.hpp"

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

void setup() {
    loggif("start\n");
    AppTimer::getInstance();
    Modem::getInstance().init();
//    Vaga::getInstance();

    AppTimer::getInstance().registerCallback(keepAliveMessage, 5000, "keepAlive");
    AppTimer::getInstance().registerCallback(ledBlink, 500, "ledBlink");
    AppTimer::getInstance().registerCallback(Modem::staticTimerCallback, 100, "modem", 5000);
//    AppTimer::getInstance().registerCallback(Vaga::staticTimerCallback, 10000, "vaga", 10000);
    loggif("end\n");
}

void loop() {
    AppTimer::getInstance().execute();
}