#include "string_builder.hpp"
#include "basic.hpp"
#include "logging.hpp"

StringBuilder::StringBuilder() {
    c_str = (char*)malloc(1);
    c_str[0] = '\0';
    length = 1;
}

StringBuilder::StringBuilder(int p_len) {
    if (p_len < 1) p_len = 1;
    c_str = (char*)malloc(p_len);
    c_str[0] = '\0';
    length = p_len;
}

StringBuilder::~StringBuilder() {
    free(c_str);
}

int StringBuilder::CountLines() const {
    if (length == 0) return 0;
    int res = 1;
    for (int i = 0; i < length; i++) {
        if (c_str[i] == '\n') res++;
    }
    return res;
}

StringBuilder& StringBuilder::Clear() {
    c_str = (char*)malloc(1);
    c_str[0] = '\0';
    length = 1;
    return *this;
}

StringBuilder& StringBuilder::Add(const char* add_str) {
    int write_offset = length - 1;
    length += strlen(add_str);
    c_str = (char*)realloc(c_str, length);
    strcpy(c_str + write_offset, add_str);
    c_str[length - 1] = '\0';
    return *this;
}

StringBuilder& StringBuilder::_AddBuffer(char buffer[]) {
    int write_offset = length - 1;
    length += strlen(buffer);
    c_str = (char*)realloc(c_str, length);
    strcpy(c_str + write_offset, buffer);
    c_str[length - 1] = '\0';
    return *this;
}

StringBuilder& StringBuilder::AddLine(const char *add_str) {
    int write_offset = length - 1;
    length += strlen(add_str) + 1;
    c_str = (char*)realloc(c_str, length);
    strcpy(c_str + write_offset, add_str);
    c_str[length - 2] = '\n';
    c_str[length - 1] = '\0';
    return *this;
}

StringBuilder& StringBuilder::AddFormat(const char* fmt, ...) {
    char buffer[100];  // How much is enough?
    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
    return _AddBuffer(buffer);
}

StringBuilder& StringBuilder::AddF(double num) {
    char buffer[12];  // How much is enough?
    if (abs(num) > 1e4 || abs(num) < 1e-4) {
        sprintf(buffer, "%.4e", num);
    } else {
        sprintf(buffer, "%.7f", num);
    }
    return _AddBuffer(buffer);
}

StringBuilder& StringBuilder::AddI(int num) {
    char buffer[12];
    sprintf(buffer, "%d", num);
    return _AddBuffer(buffer);
}

StringBuilder& StringBuilder::AddTime(timemath::Time t) {
    char buffer[20];
    t.FormatAsTime(buffer, 20);
    return _AddBuffer(buffer);
}


StringBuilder& StringBuilder::AddDate(timemath::Time t, bool shorthand) {
    char buffer[20];
    t.FormatAsDate(buffer, 20, shorthand);
    return _AddBuffer(buffer);
}


StringBuilder& StringBuilder::AddCost(int64_t cost) {
    // 9223372036854775807: 19 digits
    // -3 for truncation
    // +5 text
    // +1 - sign
    // +1 terminator
    char buffer[24];
    sprintf(buffer, "M§M %ldK", cost / 1000);
    return _AddBuffer(buffer);
}

int StringBuilderTests() {
    StringBuilder sb;
    sb.Add("abcd").Add("01234").Add("ÄÄÖÜ").AddI(-175);
    if (strcmp(sb.c_str, "abcd01234ÄÄÖÜ-175") != 0) {
        ERROR("Expected '%s', got '%s'", "abcd01234ÄÄÖÜ-175", sb.c_str);
        return 1;
    }
    timemath::Time t = timemath::Time(1258254);
    sb.Clear();
    sb.AddF(-1e10).Add(" - ").AddF(15.84).Add(" - ").AddTime(t).Add(" - ").AddDate(t);
    const char* test_str = "-1.0000e+10 - 15.8400000 -  0M 14D 13H - 15. 01. 2080Y, 13h";
    if (strcmp(sb.c_str, test_str) != 0) {
        ERROR("Expected '%s', got '%s'", test_str, sb.c_str);
        return 1;
    }
    sb.Clear();
    sb.AddFormat("%05d %X , %s", 17, 32, "Trololo");
    const char* test_str2 = "00017 20 , Trololo";
    if (strcmp(sb.c_str, test_str2) != 0) {
        ERROR("Expected '%s', got '%s'", test_str2, sb.c_str);
        return 1;
    }
    return 0;
}