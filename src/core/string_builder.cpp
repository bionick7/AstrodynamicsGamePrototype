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

void StringBuilder::Clear() {
    c_str = (char*)malloc(1);
    c_str[0] = '\0';
    length = 1;
}

StringBuilder& StringBuilder::Add(const char* add_str) {
    int write_offset = length - 1;
    length += strlen(add_str);
    c_str = (char*)realloc(c_str, length);
    strcpy(c_str + write_offset, add_str);
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

StringBuilder& StringBuilder::AddF(double num) {
    char buffer[12];  // How much is enough?
    if (abs(num) > 1e3 || abs(num) < 1e-3) {
        sprintf(buffer, "%.4e", num);
    } else {
        sprintf(buffer, "%.7f", num);
    }
    int write_offset = length - 1;
    length += strlen(buffer);
    c_str = (char*)realloc(c_str, length);
    strcpy(c_str + write_offset, buffer);
    c_str[length - 1] = '\0';
    return *this;
}

StringBuilder& StringBuilder::AddI(int num) {
    char buffer[12];
    sprintf(buffer, "%d", num);
    int write_offset = length - 1;
    length += strlen(buffer);
    c_str = (char*)realloc(c_str, length);
    strcpy(c_str + write_offset, buffer);
    c_str[length - 1] = '\0';
    return *this;
}

StringBuilder& StringBuilder::AddTime(Time t) {
    char buffer[20];
    FormatTime(buffer, 20, t);
    int write_offset = length - 1;
    length += strlen(buffer);
    c_str = (char*)realloc(c_str, length);
    strcpy(c_str + write_offset, buffer);
    c_str[length - 1] = '\0';
    return *this;
}


StringBuilder& StringBuilder::AddDate(Time t) {
    char buffer[20];
    FormatDate(buffer, 20, t);
    int write_offset = length - 1;
    length += strlen(buffer);
    c_str = (char*)realloc(c_str, length);
    strcpy(c_str + write_offset, buffer);
    c_str[length - 1] = '\0';
    return *this;
}


int StringBuilderTests() {
    StringBuilder sb;
    sb.Add("abcd").Add("01234").Add("ÄÄÖÜ").AddI(-175);
    if (strcmp(sb.c_str, "abcd01234ÄÄÖÜ-175") != 0) {
        ERROR("Expected '%s', got '%s'", "abcd01234ÄÄÖÜ-175", sb.c_str);
        return 1;
    }
    Time t = Time(1258254);
    sb.Clear();
    sb.AddF(-1e10).Add(" - ").AddF(15.84).Add(" - ").AddTime(t).Add(" - ").AddDate(t);
    const char* test_str = "-1.0000e+10 - 15.8400000 -  0M 14D 13H - 2080Y  1M 15D 13H";
    if (strcmp(sb.c_str, test_str) != 0) {
        ERROR("Expected '%s', got '%s'", test_str, sb.c_str);
        return 1;
    }
}