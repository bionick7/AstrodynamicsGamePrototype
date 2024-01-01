#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>

#ifdef LOGGING_DISABLE

#define INFO(...)
#define WARNING(...)
#define ERROR(...)
#define USER_INFO(...)
#define USER_ERROR(...)

#define SHOW_F(var)
#define SHOW_I(var)
#define SHOW_V2(var)

#define NOT_IMPLEMENTED {exit(1);}
#define NOT_REACHABLE {exit(1);}
#define FAIL(...) {exit(1);}

#define ASSERT(condition) 
#define ASSERT_EQUAL_INT(v1, v2)
#define ASSERT_ALOMST_EQUAL_FLOAT(v1, v2) 

#else

#define INFO(...) LogImpl(__FILE__, __LINE__, LOGTYPE_INFO, __VA_ARGS__);
#define WARNING(...) LogImpl(__FILE__, __LINE__, LOGTYPE_WARNING, __VA_ARGS__);
#define ERROR(...) LogImpl(__FILE__, __LINE__, LOGTYPE_ERROR, __VA_ARGS__);
#define USER_INFO(...) LogImpl(__FILE__, __LINE__, LOGTYPE_PLAYER, __VA_ARGS__);
#define USER_ERROR(...) LogImpl(__FILE__, __LINE__, LOGTYPE_USERERROR, __VA_ARGS__);

#define SHOW_F(var) LogImpl(__FILE__, __LINE__, LOGTYPE_INFO, "%s = %f", #var, var);
#define SHOW_I(var) LogImpl(__FILE__, __LINE__, LOGTYPE_INFO, "%s = %d", #var, var);
#define SHOW_V2(var) LogImpl(__FILE__, __LINE__, LOGTYPE_INFO, "%s = (%f, %f)", #var, (var).x, (var).y);

#define NOT_IMPLEMENTED {ERROR("Not Implemented") exit(1);}
#define NOT_REACHABLE {ERROR("Should not be reachable") exit(1);}
#define FAIL(...) {ERROR(__VA_ARGS__) exit(1);}

#define ASSERT(condition) if (!(condition)) { ERROR("Assertion failed: (%s)", #condition); }
#define ASSERT_EQUAL_INT(v1, v2) if (v1 != v2) { ERROR("Assertion failed: %s (%d)  != %s (%d)", #v1, v1, #v2, v2); }
#define ASSERT_ALOMST_EQUAL_FLOAT(v1, v2) if (fabs(v1 - v2) > v1 * 1e-5) { ERROR("Assertion failed: %s (%f) != %s (%f)", #v1, v1, #v2, v2); }

#endif  // LOGGING_DISABLE

// Wren output shall exist even in release versions
enum LogType{
    LOGTYPE_INFO,
    LOGTYPE_WARNING,
    LOGTYPE_ERROR,
    LOGTYPE_PLAYER,
    LOGTYPE_USERERROR,
    LOGTYPE_WRENINFO,
    LOGTYPE_WRENERROR,

    // Keep continuous
    LOGTYPE_RAYALL,
    LOGTYPE_RAYTRACE,
    LOGTYPE_RAYDEBUG,
    LOGTYPE_RAYINFO,
    LOGTYPE_RAYWARNING,
    LOGTYPE_RAYERROR,
    LOGTYPE_RAYFATAL,
    LOGTYPE_RAYNONE,
};

#define LOG_INVALID_FILE ""
#define LOG_INVALID_LINE -1

void LogSetOutput(const char* filename);
void LogSetOutputs(const char* filenames[]);
void LogCloseOutputs();
void LogToStdout(bool value);

void VLogImpl(const char* file, int line, LogType level, const char* format, va_list args);
void LogImpl(const char* file, int line, LogType level, const char* format, ...);

void CustomRaylibLog(int msgType, const char *text, va_list args);

#endif  // LOGGING_H