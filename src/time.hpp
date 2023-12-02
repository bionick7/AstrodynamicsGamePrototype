#ifndef TIME_H
#define TIME_H
#include "basic.hpp"
#include "datanode.hpp"
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

void TimeSerialize(Time x, DataNode* data);
void TimeDeserialize(Time* x, const DataNode* data);

Time GetInvalidTime();
bool IsTimeInvalid();

char* FormatTime(char* buffer, int buffer_len, Time time);
char* FormatDate(char* buffer, int buffer_len, Time time);

int TimeTests();
#endif  // TIME_H
