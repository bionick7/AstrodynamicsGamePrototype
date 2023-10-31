#include "basic.hpp"
#include <time.h>

void FormatTime(char* buffer, int buffer_len, time_type time){
    time_t time_in_s = (time_t) time;
    tm time_tm = *gmtime(&time_in_s);
    time_tm.tm_year -= 70;
    if (time_tm.tm_year > 0) {
        snprintf(buffer, buffer_len, "%4dY %2dM %2dD %dH", time_tm.tm_year, time_tm.tm_mon, time_tm.tm_mday, time_tm.tm_hour);
    } else {
        snprintf(buffer, buffer_len, "%2dM %2dD %dH", time_tm.tm_mon, time_tm.tm_mday, time_tm.tm_hour);
    }
}

void FormatDate(char* buffer, int buffer_len, time_type time){
    int start_year = 2080 - 1970;
    //time_t epoch_in_s = 65744l*86400l;  // 1900 - 2080
    time_t time_in_s = time;  // + epoch_in_s;
    tm time_tm = *gmtime(&time_in_s);
    time_tm.tm_year += start_year;
    snprintf(buffer, buffer_len, "%4dY %2dM %2dD %2dH", time_tm.tm_year + 1900, time_tm.tm_mon + 1, time_tm.tm_mday, time_tm.tm_hour);
}
