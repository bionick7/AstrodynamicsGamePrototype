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

void StringBuilder::Add(const char* add_str) {
    int write_offset = length - 1;
    length += strlen(add_str);
    c_str = (char*)realloc(c_str, length);
    strcpy(c_str + write_offset, add_str);
    c_str[length - 1] = '\0';
}


int StringBuilderTests() {
    StringBuilder sb;
    sb.Add("abcd");
    sb.Add("01234");
    sb.Add("ÄÄÖÜ");
    if (strcmp(sb.c_str, "abcd01234ÄÄÖÜ") != 0) return 1;
}