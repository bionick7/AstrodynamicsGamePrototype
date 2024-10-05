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


struct PermaString {
    int offset;

    PermaString();
    PermaString(const char* string);
    const char* GetChar() const;
};

struct StringBuilder {
    char* c_str;
    int length;  // size of buffer, not of string

    StringBuilder();
    StringBuilder(int p_length);
    StringBuilder(const char* str);
    StringBuilder(const StringBuilder& other);
    void _ReSize(int new_length);
    ~StringBuilder();
    int CountLines() const;
    void WriteToFile(const char* filename) const;

    void Clear();
    TokenList ExtractTokens(const char* start_delim, const char* end_delim);
    StringBuilder GetSubstring(int from, int to);

    StringBuilder& Add(const char* add_str);
    StringBuilder& _AddWithTerminator(const char* add_str);
    StringBuilder& AddPerma(PermaString perma_str);
    StringBuilder& AddFormat(const char* fmt, ...);
    StringBuilder& AddLine(const char* add_str);
    StringBuilder& AddF(double num);
    StringBuilder& AddI(int num);
    StringBuilder& AddTime(timemath::Time t);
    StringBuilder& AddDate(timemath::Time t, bool shorthand=false);
    StringBuilder& AddCost(int64_t cost);
    StringBuilder& AddClock(float progress);
};

int StringBuilderTests();

#endif  // STRING_BUILDER_H
