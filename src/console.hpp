#ifndef __CONSOLE_HPP__
#define __CONSOLE_HPP__

#include "string"
#include "vector"
#include "Arduino.h"
#include "logger.hpp"

#ifndef VT100_OFF
#define COLOR(__c, __x)    "\x1b[3" #__c "m" __x "\x1b[0m"
#define COLOR_BOLD(__c, __x)    "\x1b[3" #__c ";1m" __x "\x1b[0m"
#define UNDERLINE(__x) "\x1b[4m" __x "\x1b[0m"
#define CLEAR_SCREEN    "\x1b[2J\x1b[H"
#else
#define COLOR(__c,__x)    __x
#define COLOR_BOLD(__c,__x) __x
#define UNDERLINE(__x) __x
#define CLEAR_SCREEN
#endif
#define RED(__x)        COLOR(1, __x )
#define GREEN(__x)        COLOR(2, __x )
#define YELLOW(__x)        COLOR(3, __x )
#define BLUE(__x)        COLOR(4, __x )
#define MAGENTA(__x)    COLOR(5, __x )
#define CYAN(__x)        COLOR(6, __x )
#define RED_B(__x)        COLOR_BOLD(1, __x )
#define GREEN_B(__x)        COLOR_BOLD(2, __x )
#define YELLOW_B(__x)        COLOR_BOLD(3, __x )
#define BLUE_B(__x)        COLOR_BOLD(4, __x )
#define MAGENTA_B(__x)    COLOR_BOLD(5, __x )
#define CYAN_B(__x)        COLOR_BOLD(6, __x )

class Console {
public:
    typedef void (*func_t)(int argc, char *argv[]);

    typedef struct {
        const char *cmd;
        const char *argDesc;
        const char *desc;
        func_t f;
    } cmd_list_t;

private:
    char mBuffer[128];
    uint32_t mIndex;
    std::vector<cmd_list_t> mCmdTable;

    Console() {
        loggif("\n");
        mIndex = 0;
        init();
    }

    void init() {
        mCmdTable.push_back(cmd_list_t{"help", "", "Show this help message", Console::staticHelp});
    }

    void handleByte(char byte) {
        if (byte == '/') {
            loggif("buffer[%d]", mIndex);
            loggaln(mBuffer, mIndex);
            return;
        } else if (byte == 0x7f) {
            if (mIndex > 0) {
                mIndex--;
            } else {
                loggif("Ready, buffer empty\n");
            }
            return;
        }

        if (mIndex >= sizeof(mBuffer)) {
            if (byte == '\n' || byte == '\r') {
                loggif("buffer FLUSHED, ready\n");
                mIndex = 0;
            } else {
                loggif("buffer OVERFLOW, press RETURN for clean start\n");
            }
            return;
        }

        if ((byte == '\n') || (byte == '\r')) {
            mBuffer[mIndex] = 0;
            if (mIndex >= 1) {
                handleCommand(mBuffer);
            } else {
                loggif("Ready, buffer empty\n");
            }
            mIndex = 0;
        } else {
            mBuffer[mIndex] = byte;
            mIndex++;
        }
    }

    void handleCommand(char *cmd) {
        loggif("%s\n", cmd);
        char *argv[10];
        int argc = 10;

        util_parse_params(cmd, argv, argc, ' ', ' ');
        process(argc, argv);
    }

    void process(int argc, char *argv[]) {
        for (uint32_t i = 0; i < mCmdTable.size(); i++) {
            cmd_list_t &cmdList = mCmdTable[i];
            if (cmdList.f && !strcmp(argv[0], cmdList.cmd)) {
                cmdList.f(argc, argv);
                return;
            }
        }
        loggif(RED("Unknown Command \'%s\'. Type help for a list of commands\n"), argv[0]);
    }

    void util_parse_params(char *str, char *argv[], int &argc, char delim1, char delim2) {
        int max_args = argc;
        char *cmdl = str;
        bool done = false;
        argc = 0;
        char delim = delim1;
        while (!done) {
            /* Strip Leading Whitespce */
            while (isspace(*cmdl)) {
                if (*cmdl) {
                    cmdl++;
                } else {
                    done = true;
                    break;
                }
            }
            /* Now we are at an arg */
            if (!done && *cmdl) {
                argv[argc] = cmdl;
                argc++;
                if (argc >= max_args) {
                    done = true;
                    break;
                }
            }
            /* Go to the next delim */
            while (delim != *cmdl) {
                if (*cmdl) {
                    cmdl++;
                } else {
                    done = true;
                    break;
                }
            }
            if (*cmdl) {
                *cmdl = 0;
                cmdl++;
            } else {
                done = true;
            }
            if (argc) {
                delim = delim2;
            }
        }
    }

    /** implementation */

    void help(int argc, char *argv[]) {
        loggif(GREEN("TermCMD commands:\n"));
        for (uint32_t i = 0; i < mCmdTable.size(); i++) {
            cmd_list_t &cmdList = mCmdTable[i];
            if (cmdList.f) {
                loggif("%-10s %-20s - %s\n", cmdList.cmd, cmdList.argDesc, cmdList.desc);
            } else {
                loggif(BLUE("%s\n"), cmdList.cmd);
            }
        }
    }

    static void staticHelp(int argc, char *argv[]) {
        getInstance().help(argc, argv);
    }

public:

    static Console &getInstance() {
        static Console instance;
        return instance;
    }

    static void receiveSerial() {
        while (Serial.available()) {
            getInstance().handleByte(Serial.read());
        }
    }

    void registerCommand(cmd_list_t cmd) {
        mCmdTable.push_back(cmd);
    }

    static void printError(int argc, char *argv[]) {
        loggif(RED("Error Command Chain ") "[%d]", argc);
        for (int i = 0; i < argc; i++) {
            logga(argv[i], strlen(argv[i]));
        }
        loggf("\n");
    }

    static uint32_t getNum(char *str) {
        return atoi(str);
    }

    static uint32_t getHexLen(char *str) {
        return strlen(str) / 2;
    }

    static void getHex(char *str, uint8_t *buffer) {
        for (uint32_t i = 0; i + 1 < strlen(str); i += 2) {
            std::string s(str + i, str + i + 2);
            auto x = std::strtol(s.c_str(), 0, 16);
            buffer[i / 2] = x;
        }
    }
};

#endif /* CONSOLE_HPP_ */
