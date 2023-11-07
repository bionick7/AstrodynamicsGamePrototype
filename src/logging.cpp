#include "logging.hpp"

#ifndef LOGGING_DISABLE

#include <stdio.h>
#include <stdarg.h>

void LogImpl(const char* file, int line, LogType level, const char* format, ...) {
    printf("%s:%d :: ", file, line);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

#endif  // LOGGING_DISABLE