#ifndef TIME_H
#define TIME_H
#include "basic.hpp"

struct Time {
    double __t;
    Time() : Time(0.0) {};
    Time(double seconds);
};

Time TimeAdd(Time lhs, Time rhs);
Time TimeAddSec(Time x, double seconds);
Time TimeSub(Time lhs, Time rhs);
double TimeSecDiff(Time lhs, Time rhs);
Time TimeEarliest(Time lhs, Time rhs);
Time TimeLatest(Time lhs, Time rhs);
Time TimePosMod(Time x, Time mod);
bool TimeIsEarlier(Time lhs, Time rhs);
bool TimeIsPos(Time x);

double TimeSeconds(Time x);
double TimeDays(Time x);

void FormatTime(char* buffer, int buffer_len, Time time);
void FormatDate(char* buffer, int buffer_len, Time time);
#endif  // TIME_H
