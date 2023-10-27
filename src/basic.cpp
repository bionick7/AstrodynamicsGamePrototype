#include "basic.hpp"
#include <time.h>

void ForamtTime(char* buffer, int buffer_len, time_type time){
    struct tm time_tm = {.tm_year=2080-1900, .tm_mday=1};
    time_tm.tm_sec += time;
    mktime(&time_tm);
    if (time_tm.tm_year > 2080-1900) {
        snprintf(buffer, buffer_len, "%4dY %2dM %2dD %dH", time_tm.tm_year - 180, time_tm.tm_mon, time_tm.tm_mday, time_tm.tm_hour);
    } else {
        snprintf(buffer, buffer_len, "%2dM %2dD %dH", time_tm.tm_mon, time_tm.tm_mday, time_tm.tm_hour);
    }
}
