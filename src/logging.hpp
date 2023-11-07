#ifndef LOGGING_H
#define LOGGING_H

#ifdef LOGGING_DISABLE

#define INFO(...)
#define WARNING(...)
#define ERROR(...)

#define SHOW_F(var)
#define SHOW_I(var)
#define SHOW_V2(var)

#define NOT_IMPLEMENTED {exit(1);}
#define FAIL(...) {exit(1);}

#define ASSERT(condition) 
#define ASSERT_MSG(condition, msg) 
#define ASSERT_ALOMST_EQUAL(v1, v2) 

#else

#define INFO(...) LogImpl(__FILE__, __LINE__, LOGTYPE_INFO, __VA_ARGS__);
#define WARNING(...) LogImpl(__FILE__, __LINE__, LOGTYPE_WARNING, __VA_ARGS__);
#define ERROR(...) LogImpl(__FILE__, __LINE__, LOGTYPE_ERROR, __VA_ARGS__);

#define SHOW_F(var) LogImpl(__FILE__, __LINE__, LOGTYPE_INFO, "%s = %f", #var, var);
#define SHOW_I(var) LogImpl(__FILE__, __LINE__, LOGTYPE_INFO, "%s = %d", #var, var);
#define SHOW_V2(var) LogImpl(__FILE__, __LINE__, LOGTYPE_INFO, "%s = (%f, %f)", #var, (var).x, (var).y);

#define NOT_IMPLEMENTED {ERROR("Not Implemented") exit(1);}
#define FAIL(...) {ERROR(__VA_ARGS__) exit(1);}

#define ASSERT(condition) if (!(condition)) { ERROR("Assertion failed: (%s)", #condition); }
#define ASSERT_ALOMST_EQUAL(v1, v2) if (fabs(v1 - v2) > v1 * 1e-5) { ERROR("Assertion failed: %s (%f) != %s (%f)", #v1, v1, #v2, v2); }

enum LogType{
    LOGTYPE_INFO,
    LOGTYPE_WARNING,
    LOGTYPE_ERROR,
};

void LogImpl(const char* file, int line, LogType level, const char* format, ...);

#endif  // LOGGING_DISABLE

#endif  // LOGGING_H