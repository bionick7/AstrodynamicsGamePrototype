#include "time.hpp"

#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

struct StringBuilder {
    char* c_str;
    int length;

    StringBuilder();
    StringBuilder(int p_length);
    ~StringBuilder();
    void Clear();
    StringBuilder& Add(const char* add_str);
    StringBuilder& AddLine(const char* add_str);
    StringBuilder& AddF(double num);
    StringBuilder& AddI(int num);
    StringBuilder& AddTime(Time t);
    StringBuilder& AddDate(Time t);
};

int StringBuilderTests();

#endif  // STRING_BUILDER_H
