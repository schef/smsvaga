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

static inline char *f2str(float num, uint32_t bufferNum = 0, uint32_t decimalPlaces = 6)
{
#define FLOAT_BUFFER_SIZE 16
    auto numLeft = (long)num;
    auto numRight = (long)abs((num * (long)pow(10L, decimalPlaces))) - (long)abs((numLeft * (long)pow(10L, decimalPlaces)));
    if (numRight < 0) {
        numRight *= -1L;
    }
    char *buffer = NULL;
    switch (bufferNum)
    {
        case 0:
            static char buffer0[FLOAT_BUFFER_SIZE];
            buffer = buffer0;
            break;
        case 1:
            static char buffer1[FLOAT_BUFFER_SIZE];
            buffer = buffer1;
            break;
        case 2:
            static char buffer2[FLOAT_BUFFER_SIZE];
            buffer = buffer2;
            break;
    }
    if (num < 0 && num > -1) {
        snprintf(buffer, FLOAT_BUFFER_SIZE, "-%ld.%ld", numLeft, numRight);
    } else {
        snprintf(buffer, FLOAT_BUFFER_SIZE, "%ld.%ld", numLeft, numRight);
    }
    return buffer;
}

#endif //__LOGGER_HPP__