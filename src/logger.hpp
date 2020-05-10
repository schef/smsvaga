#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include "Arduino.h"

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define loggi(format, ...) LoggerPrintf("[%s] " format , __FILENAME__, ##__VA_ARGS__)
#define loggif(format, ...) LoggerPrintf("[%s:%s] " format , __FILENAME__, __FUNCTION__, ##__VA_ARGS__)
#define loggifl(format, ...) LoggerPrintf("[%s:%s:%d] " format , __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define loggf(format, ...) LoggerPrintf(format, ##__VA_ARGS__)
#define loggb(data, len, separator) LoggerPrintBytes(data, len, separator, false)
#define loggbln(data, len, separator) LoggerPrintBytes(data, len, separator, true)
#define logga(data, len) LoggerPrintAscii(data, len, false)
#define loggaln(data, len) LoggerPrintAscii(data, len, true)

static inline void LoggerPrintf(const char *format, ...) {
    static bool init = false;
    if (!init) {
        init = true;
        Serial.begin(115200);
    }

    char buf[128]; // resulting string limited to 128 chars
    va_list args;
    va_start (args, format);
    vsnprintf(buf, 128, format, args);
    va_end (args);
    Serial.print(buf);
}

static inline void LoggerPrintAscii(const void *object, size_t size, bool newLine) {
    const unsigned char *const bytes = (const unsigned char *const) object;
    size_t i;

    LoggerPrintf("[");
    for (i = 0; i < size; i++) {
        LoggerPrintf("%c", bytes[i]);
    }
    LoggerPrintf("]");
    if (newLine) {
        LoggerPrintf("\n");
    } else {
        LoggerPrintf(" ");
    }
}

static inline void LoggerPrintBytes(const void *object, size_t size, char separator, bool newLine) {
    const unsigned char *const bytes = (const unsigned char *const) object;
    size_t i;

    LoggerPrintf("[");
    for (i = 0; i < size; i++) {
        LoggerPrintf("%02X", bytes[i]);
        if (i < size - 1) {
            LoggerPrintf("%c", separator);
        }
    }
    LoggerPrintf("]");

    if (newLine) {
        LoggerPrintf("\n");
    }
}

#endif //__LOGGER_HPP__