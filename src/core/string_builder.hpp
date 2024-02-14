#include "time.hpp"

#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

struct TokenList {
    int length;
    int capacity;
    int* start_positions;
    int* end_positions;

    TokenList();
    ~TokenList();
    void AddToken(int start, int end);
};


struct StringBuilder {
    char* c_str;
    int length;  // size of buffer, not of  string

    StringBuilder();
    StringBuilder(int p_length);
    StringBuilder(const char* str);
    ~StringBuilder();
    int CountLines() const;
    void WriteToFile(const char* filename) const;
    

    void Clear();
    void AutoBreak(int max_width);
    TokenList ExtractTokens(const char* start_delim, const char* end_delim);
    StringBuilder GetSubstring(int from, int to);

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
