#include "logging.hpp"
#include "raylib.h"

#include <stdio.h>
#include <time.h>

FILE** outputs = NULL;
int outputs_len = 0;
bool log_to_console = true;

void LogSetOutput(const char* filename) {
    LogCloseOutputs();
    outputs_len = 1;
    outputs = new FILE*[1];
    outputs[0] = fopen(filename, "w");
}

void LogSetOutputs(const char* filenames[]) {
    LogCloseOutputs();
    outputs_len = sizeof(filenames) / sizeof(const char*);
    if (outputs_len == 0) return;
    outputs = new FILE*[outputs_len];
    for (int i=0; i < outputs_len; i++){
        outputs[i] = fopen(filenames[i], "w");
    }
}

void LogCloseOutputs() {
    for (int i=0; i < outputs_len; i++){
        fclose(outputs[i]);
    }
    delete[] outputs;
}

void LogToStdout(bool value) {
    log_to_console = value;
}

void VLogImpl(const char* file, int line, LogType level, const char* format, va_list args) {
    if (log_to_console) {
        if (line > 0) {
            printf("%s:%d :: ", file, line);
        }
        vprintf(format, args);
        printf("\n");
    }
    for (int i=0; i < outputs_len; i++) {
        char timeStr[64] = { 0 };
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);

        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(outputs[i], "[%s] ", timeStr);

        if (line > 0) {
            fprintf(outputs[i], "%s:%d :: ", file, line);
        }
        vfprintf(outputs[i], format, args);
        fprintf(outputs[i], "\n");
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