#include "string_builder.hpp"
#include "basic.hpp"
#include "logging.hpp"
#include "tests.hpp"
#include "constants.hpp"

#include <time.h>

TokenList::TokenList() {
    start_positions = NULL;
    end_positions = NULL;
    length = 0;
    capacity = 0;
}

TokenList::~TokenList() {
    delete[] start_positions;
    delete[] end_positions;
}

void TokenList::AddToken(int start, int end) {
    if (length >= capacity) {
        capacity += 5;
        int* new_start_positions = new int[capacity];
        int* new_end_positions = new int[capacity];
        for(int i=0; i < length; i++) {
            new_start_positions[i] = start_positions[i];
            new_end_positions[i] = end_positions[i];
        }
        delete[] start_positions;
        delete[] end_positions;
        start_positions = new_start_positions;
        end_positions = new_end_positions;
    }
    start_positions[length] = start;
    end_positions[length] = end;
    length++;
}

StringBuffer::StringBuffer(const char* default_string) {
    length = strlen(default_string) + 1;
    c_str = new char[length];
    strcpy(c_str, default_string);
    c_str[length - 1] = '\0';
}

StringBuffer::StringBuffer(const StringBuffer& other) {
    length = other.length;
    c_str = new char[length];
    memcpy(c_str, other.c_str, length);
}

void StringBuffer::Add(const char *add_str) {
    // Reallocate
    int new_length = length + strlen(add_str) + 1;
    char* c_str2 = new char[new_length];
    memcpy(c_str2, c_str, length);
    delete[] c_str;
    c_str = c_str2;

    // Add new string
    strcpy(c_str + length, add_str);
    length = new_length;
    c_str[length - 1] = '\0';
}

static StringBuffer perma_string;

PermaString::PermaString() {
    offset = 0;
}

PermaString::PermaString(int p_offset) {
    offset = p_offset;
}

PermaString::PermaString(const char *string) {
    offset = perma_string.length;
    perma_string.Add(string);
}

const char *PermaString::GetChar() const {
    return perma_string.c_str + offset;
}

StringBuilder::StringBuilder() {
    c_str = new char[1];
    c_str[0] = '\0';
    length = 1;
}

StringBuilder::StringBuilder(int p_len) {
    if (p_len < 1) p_len = 1;
    c_str = new char[p_len];
    c_str[0] = '\0';
    length = p_len;
}

StringBuilder::StringBuilder(const char* str) {
    length = strlen(str) + 1;
    c_str = new char[length];
    strcpy(c_str, str);
}

StringBuilder::StringBuilder(const StringBuilder &other) : StringBuilder(other.c_str) {}

void StringBuilder::_ReSize(int new_length) {
    char* c_str2 = new char[new_length];
    strcpy(c_str2, c_str);
    length = new_length;
    delete[] c_str;
    c_str = c_str2;
}

StringBuilder::~StringBuilder() {
    delete[] c_str;
}

int StringBuilder::CountLines() const {
    if (length == 0) return 0;
    int res = 1;
    for (int i = 0; i < length; i++) {
        if (c_str[i] == '\n') res++;
    }
    return res;
}

void StringBuilder::WriteToFile(const char * filename) const {
    SaveFileText(filename, c_str);
}

void StringBuilder::Clear() {
    delete[] c_str;
    c_str = new char[1];
    c_str[0] = '\0';
    length = 1;
}

TokenList StringBuilder::ExtractTokens(const char *start_delim, const char *end_delim) {
    TokenList res = TokenList();

    int start_delim_len = strlen(start_delim);
    int end_delim_len = strlen(start_delim);

    int current_start = 0;
    int new_length = length;
    for(int i=0; i < length; i++) {
        int consume = 0;
        if (i < new_length - end_delim_len - start_delim_len && strncmp(&c_str[i], start_delim, start_delim_len) == 0) {
            current_start = i;
            consume = start_delim_len;
        }
        if (i < new_length - end_delim_len && strncmp(&c_str[i], end_delim, end_delim_len) == 0) {
            res.AddToken(current_start, i);
            consume = end_delim_len;
        }
        if (consume > 0) {
            for (int j=i; j < new_length - consume; j++) {
                c_str[j] = c_str[j+consume];
            }
            new_length -= consume;
        }
    }
    
    if (new_length != length) {
        _ReSize(new_length);
    }

    return res;
}

StringBuilder StringBuilder::GetSubstring(int from, int to) {
    if (to <= from) {
        return StringBuilder();
    }
    if(to > length) {
        return *this;
    }StringBuilder sb = StringBuilder(to - from + 1);
    strncpy(sb.c_str, c_str, to - from + 1);
    sb.c_str[to - from] = '\0';
    return sb;
}

StringBuilder& StringBuilder::Add(const char* add_str) {
    int write_offset = length - 1;
    _ReSize(length + strlen(add_str));
    strcpy(c_str + write_offset, add_str);
    c_str[length - 1] = '\0';
    return *this;
}

StringBuilder &StringBuilder::AddPerma(PermaString perma_str) {
    Add(perma_str.GetChar());
    return *this;
}

StringBuilder& StringBuilder::AddLine(const char *add_str) {
    int write_offset = length - 1;
    _ReSize(length + strlen(add_str) + 1);
    strcpy(c_str + write_offset, add_str);
    c_str[length - 2] = '\n';
    c_str[length - 1] = '\0';
    return *this;
}

StringBuilder& StringBuilder::AddFormat(const char* fmt, ...) {
    static char buffer[1024];  // How much is enough?
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, 1024, fmt, args);
    va_end(args);
    return Add(buffer);
}

StringBuilder& StringBuilder::AddF(double num) {
    static char buffer[12];  // How much is enough?
    if (fabs(num) > 1e4 || (fabs(num) < 1e-4 && num != 0)) {
        sprintf(buffer, "%.4e", num);
    } else {
        sprintf(buffer, "%.7f", num);
    }
    return Add(buffer);
}

StringBuilder& StringBuilder::AddI(int num) {
    static char buffer[12];
    sprintf(buffer, "%d", num);
    return Add(buffer);
}

StringBuilder& StringBuilder::AddTime(timemath::Time t) {
    if (t.IsInvalid()) {
        return Add("[INVALID TIME]");
    }
    time_t time_in_s = (time_t) t.Seconds();
    tm* time_tm = gmtime(&time_in_s);
    if (time_tm == NULL) {
        return Add("[INVALID TIME]");
    }

    time_tm->tm_year -= 70;
    if (time_tm->tm_year == 0) {
        return AddFormat("%2dM %2dD %2dH", time_tm->tm_mon, time_tm->tm_mday - 1, time_tm->tm_hour);
    } else {
        return AddFormat("%4dY %2dM %2dD %2dH", time_tm->tm_year, time_tm->tm_mon, time_tm->tm_mday - 1, time_tm->tm_hour);
    }
}

StringBuilder& StringBuilder::AddDate(timemath::Time t, bool shorthand) {
    int start_year = 2080 - 1970;
    time_t time_in_s = t.__t;
    tm* time_tm = gmtime(&time_in_s);
    if (time_tm == NULL) {
        return Add("[INVALID DATE]");
    }
    time_tm->tm_year += start_year;
    if (shorthand) {
        return AddFormat("%d-%d'%02d", time_tm->tm_mday, time_tm->tm_mon + 1, 
                                       (time_tm->tm_year + 1900) % 100);
    } else {
        return AddFormat("%02d. %02d. %4dY, %2dh", time_tm->tm_mday, time_tm->tm_mon + 1, 
                                                   time_tm->tm_year + 1900, time_tm->tm_hour);
    }
}

StringBuilder& StringBuilder::AddCost(int64_t cost) {
    // 9223372036854775807: 19 digits
    // -3 for truncation
    // +5 text
    // +1 - sign
    // +1 terminator
    static char buffer[24];
    sprintf(buffer, "M$M %lldK", (long long int) (cost / 1000));
    return Add(buffer);
}

StringBuilder &StringBuilder::AddClock(float progress) {
    if (progress <= 0)
        return Add(ICON_CLOCK_EMPTY);
    if (progress >= 1)
        return Add(ICON_CLOCK_FULL);
    int l;
    const char* clock_utf = CodepointToUTF8(0x04e1 + (int) roundf(progress * 8), &l);
    return Add(clock_utf);
}

int StringBuilderTests() {
    StringBuilder sb;
    sb.Add("abcd").Add("01234").Add("ÄÄÖÜ").AddI(-175);
    TEST_ASSERT_STREQUAL(sb.c_str, "abcd01234ÄÄÖÜ-175")
    timemath::Time t = timemath::Time(1258254);
    sb.Clear();
    sb.AddF(-1e10).Add(" - ").AddF(15.84).Add(" - ").AddTime(t).Add(" - ").AddDate(t);
    TEST_ASSERT_STREQUAL(sb.c_str, "-1.0000e+10 - 15.8400000 -  0M 14D 13H - 15. 01. 2080Y, 13h")
    sb.Clear();
    sb.AddFormat("%05d %X , %s", 17, 32, "Trololo");
    TEST_ASSERT_STREQUAL(sb.c_str, "00017 20 , Trololo")

    sb.Clear();
    sb.AddClock(0).AddClock(1).AddClock(0.01).AddClock(0.4374).AddClock(0.4376).AddClock(0.99);
    const char* test_str_clock = ICON_CLOCK_EMPTY ICON_CLOCK_FULL ICON_CLOCK_BARELY ICON_CLOCK_3_8 ICON_CLOCK_HALF ICON_CLOCK_ALMOST;
    TEST_ASSERT_STREQUAL(sb.c_str, test_str_clock)

    return 0;
}

