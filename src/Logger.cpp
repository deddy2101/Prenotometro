#include "Logger.h"

Logger Log;

void Logger::begin(Stream& output, LogLevel minLevel) {
    _output = &output;
    _minLevel = minLevel;
}

void Logger::setLevel(LogLevel level) {
    _minLevel = level;
}

void Logger::debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LOG_DEBUG, fmt, args);
    va_end(args);
}

void Logger::info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LOG_INFO, fmt, args);
    va_end(args);
}

void Logger::warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LOG_WARN, fmt, args);
    va_end(args);
}

void Logger::error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LOG_ERROR, fmt, args);
    va_end(args);
}

void Logger::log(LogLevel level, const char* fmt, va_list args) {
    if (!_output || level < _minLevel) return;

    const char* color;
    const char* prefix;

    switch (level) {
        case LOG_DEBUG: color = ANSI_DEBUG; prefix = "[D] "; break;
        case LOG_INFO:  color = ANSI_INFO;  prefix = "[I] "; break;
        case LOG_WARN:  color = ANSI_WARN;  prefix = "[W] "; break;
        case LOG_ERROR: color = ANSI_ERROR; prefix = "[E] "; break;
        default: return;
    }

    char buf[BUFFER_SIZE];
    vsnprintf(buf, BUFFER_SIZE, fmt, args);

    _output->print(color);
    _output->print(prefix);
    _output->print(buf);
    _output->println(ANSI_RESET);
}
