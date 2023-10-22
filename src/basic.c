#include "basic.h"
#include <time.h>

void ForamtTime(char* buffer, int buffer_len, time_type time){
    struct tm time_tm = {.tm_year=2080-1900, .tm_mday=1};
    time_tm.tm_sec += time;
    mktime(&time_tm);
    int print_res = 0;
    if (time_tm.tm_year > 2080-1900) {
        print_res = snprintf(buffer, buffer_len, "%4dY %2dM %2dD", time_tm.tm_year - 180, time_tm.tm_mon, time_tm.tm_mday);
    } else {
        print_res = snprintf(buffer, buffer_len, "%2dM %2dD", time_tm.tm_mon, time_tm.tm_mday);
    }
    /*if (print_res != 0) {
        printf("%d", print_res);
        FAIL("snprintf error occured")
    }*/
}
