#include "logging.hpp"
#include <stdio.h>
#include <cstring>
#include <time.h>

// Avoid raylib functions to avoid logging recursion

namespace logging {   
    #define LOGGING_BUFFER_SIZE 1  // Force no buffer
    char buffer[LOGGING_BUFFER_SIZE];
    char target_file[FILENAME_MAX] = "";

    long time_since_last_save = 0;
    bool log_to_console = true;
    int buffer_offset = 0;
}

// TODO: periodically save logs

void LogSetOutput(const char* filename) {
    strcpy(logging::target_file, filename);
}

void LogToStdout(bool value) {
    logging::log_to_console = value;
    FILE* file = fopen(logging::target_file, "wt");
    fprintf(file, "LOGGING START\n\n");
    fclose(file);
}

void VLogImpl(const char* file, int line, LogType level, const char* format, va_list args) {
    if (!logging::log_to_console && !logging::target_file[0]) {
        return;
    }
    int print_size = 0;
    va_list args_copy;
    va_copy(args_copy, args);
    if (logging::log_to_console) {
        if (line > 0) {
            print_size += printf("%s:%d :: ", file, line);
        }
        print_size += vprintf(format, args_copy);
        print_size += printf("\n");
    } else {
        // Just count the length
        char c = '1';
        if (line > 0) {
            print_size += snprintf(&c, 1, "%s:%d :: ", file, line);
        }
        print_size += vsnprintf(&c, 1, format, args_copy);
        print_size += 1;  // '\n'
    }

    if (logging::target_file[0]) {
        timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);
        if (logging::buffer_offset + print_size > LOGGING_BUFFER_SIZE || (logging::time_since_last_save - time.tv_nsec) > 1e8) {
            FILE* f = fopen(logging::target_file, "at");
            fprintf(f, logging::buffer);
            if (line > 0) {
                fprintf(f, "%s:%d :: ", file, line);
            }
            vfprintf(f, format, args);
            fprintf(f, "\n");
            fclose(f);

            logging::buffer_offset = 0;
            logging::time_since_last_save = time.tv_nsec;
        } else {
            if (line > 0) {
                logging::buffer_offset += sprintf(&logging::buffer[logging::buffer_offset], "%s:%d :: ", file, line);
            }
            logging::buffer_offset += vsprintf(&logging::buffer[logging::buffer_offset], format, args);
            logging::buffer_offset += sprintf(&logging::buffer[logging::buffer_offset], "\n");
        }
    }
}

void LogImpl(const char* file, int line, LogType level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    VLogImpl(file, line, level, format, args);
    va_end(args);
}

void CustomRaylibLog(int msgType, const char *text, va_list args) {
    VLogImpl(LOG_INVALID_FILE, LOG_INVALID_LINE, (LogType) (msgType + (int) LOGTYPE_RAYALL), text, args);
}