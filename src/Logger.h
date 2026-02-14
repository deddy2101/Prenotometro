#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <stdarg.h>

enum LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_NONE
};

class Logger {
public:
    void begin(Stream& output, LogLevel minLevel = LOG_DEBUG);
    void setLevel(LogLevel level);

    void debug(const char* fmt, ...);
    void info(const char* fmt, ...);
    void warn(const char* fmt, ...);
    void error(const char* fmt, ...);

private:
    void log(LogLevel level, const char* fmt, va_list args);

    Stream* _output = nullptr;
    LogLevel _minLevel = LOG_DEBUG;

    static constexpr const char* ANSI_RESET = "\033[0m";
    static constexpr const char* ANSI_DEBUG = "\033[1;34m";
    static constexpr const char* ANSI_INFO  = "\033[1;32m";
    static constexpr const char* ANSI_WARN  = "\033[1;33m";
    static constexpr const char* ANSI_ERROR = "\033[1;31m";

    static constexpr size_t BUFFER_SIZE = 256;
};

extern Logger Log;

#endif
