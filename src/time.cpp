#include "time.hpp"
#include "logging.hpp"
#include <time.h>

Time::Time(double seconds) {
    __t = seconds;
}

Time TimeAdd(Time lhs, Time rhs) {
    return Time(lhs.__t + rhs.__t);
}

Time TimeAddSec(Time x, double seconds) {
    return Time(x.__t + seconds);
}

Time TimeSub(Time lhs, Time rhs) {
    return Time(lhs.__t - rhs.__t);
}

double TimeSecDiff(Time lhs, Time rhs) {
    return lhs.__t - rhs.__t;
}

Time TimePosMod(Time x, Time mod) {
    double __t = fmod(x.__t, mod.__t);
    if (__t < 0) __t += mod.__t;
    return Time(__t);
}

Time TimeEarliest(Time lhs, Time rhs) {
    return TimeIsEarlier(lhs, rhs) ? lhs : rhs;
}

Time TimeLatest(Time lhs, Time rhs) {
    return TimeIsEarlier(lhs, rhs) ? rhs : lhs;
}

bool TimeIsEarlier(Time lhs, Time rhs) {
    return lhs.__t < rhs.__t;
}

bool TimeIsPos(Time x) {
    return x.__t > 0;
}

double TimeSeconds(Time x) {
    return x.__t;
}

double TimeDays(Time x) {
    return x.__t / 86400;
}

void FormatTime(char* buffer, int buffer_len, Time time) {
    time_t time_in_s = (time_t) time.__t;

    tm time_tm = *gmtime(&time_in_s);
    time_tm.tm_year -= 70;
    if (time_tm.tm_year > 0) {
        snprintf(buffer, buffer_len, "%4dY %2dM %2dD %dH", time_tm.tm_year, time_tm.tm_mon, time_tm.tm_mday - 1, time_tm.tm_hour);
    } else {
        snprintf(buffer, buffer_len, "%2dM %2dD %dH", time_tm.tm_mon, time_tm.tm_mday - 1, time_tm.tm_hour);
    }
}

void FormatDate(char* buffer, int buffer_len, Time time){
    int start_year = 2080 - 1970;
    //time_t epoch_in_s = 65744l*86400l;  // 1900 - 2080
    time_t time_in_s = time.__t;  // + epoch_in_s;
    tm time_tm = *gmtime(&time_in_s);
    time_tm.tm_year += start_year;
    snprintf(buffer, buffer_len, "%4dY %2dM %2dD %2dH", time_tm.tm_year + 1900, time_tm.tm_mon + 1, time_tm.tm_mday, time_tm.tm_hour);
}


#define TIMETEST_EQUAL(expr, seconds_res) if (fabs(TimeSeconds(expr) - (seconds_res)) > 1e-10) { \
    ERROR("'%s' returned unexpected value: %f instead of %f", #expr, TimeSeconds(expr), (seconds_res)) \
    return 1; \
}

int TimeTests() {
    Time t1 = Time(123.456);
    Time t2 = Time(456.789);

    TIMETEST_EQUAL(TimeAdd(t1, t2), 123.456 + 456.789)
    TIMETEST_EQUAL(TimeSub(t1, t2), 123.456 - 456.789)
    TIMETEST_EQUAL(TimeSub(t2, t1), 456.789 - 123.456)
    TIMETEST_EQUAL(TimePosMod(Time(-340), Time(100)), 60)
    if (!TimeIsEarlier(t1, t2)) return 1;

    return 0;
}