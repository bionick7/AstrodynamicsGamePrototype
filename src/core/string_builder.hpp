#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

struct StringBuilder {
    char* c_str;
    int length;

    StringBuilder();
    StringBuilder(int p_length);
    ~StringBuilder();
    void Add(const char*);
};

int StringBuilderTests();

#endif  // STRING_BUILDER_H
