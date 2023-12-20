#include "time.hpp"

#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

struct StringBuilder {
    char* c_str;
    int length;

    StringBuilder();
    StringBuilder(int p_length);
    ~StringBuilder();
    int CountLines() const;
    StringBuilder& Clear();
    StringBuilder& Add(const char* add_str);
    StringBuilder& _AddBuffer(char buffer[]);
    StringBuilder& AddFormat(const char* fmt, ...);
    StringBuilder& AddLine(const char* add_str);
    StringBuilder& AddF(double num);
    StringBuilder& AddI(int num);
    StringBuilder& AddTime(timemath::Time t);
    StringBuilder& AddDate(timemath::Time t, bool shorthand=false);
    StringBuilder& AddCost(int64_t cost);
};


int StringBuilderTests();

#endif  // STRING_BUILDER_H
