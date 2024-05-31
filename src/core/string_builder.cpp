#include "string_builder.hpp"
#include "basic.hpp"
#include "logging.hpp"
#include "tests.hpp"


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
    c_str = new char[1];
    c_str[0] = '\0';
    length = 1;
}

void StringBuilder::AutoBreak(int max_width) {
    int character_toll = 0;
    int last_space = 0;
    int num_words = 0;
    for(int i=0; i < length - 1; i++) {
        if (c_str[i] == ' ' || c_str[i] == '\t' || i == length - 2) {
            if (character_toll > max_width && num_words > 0) {
                c_str[last_space] = '\n';
                int word_length = i - last_space - 1;
                character_toll = word_length;
                num_words = 0;
            }
            num_words++;
            last_space = i;
        }
        character_toll++;
        if (c_str[i] == '\n') {
            character_toll = 0;
        }
    }
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

StringBuilder& StringBuilder::_AddBuffer(char buffer[]) {
    int write_offset = length - 1;
    _ReSize(length + strlen(buffer));
    strcpy(c_str + write_offset, buffer);
    c_str[length - 1] = '\0';
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
    char buffer[100];  // How much is enough?
    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
    return _AddBuffer(buffer);
}

StringBuilder& StringBuilder::AddF(double num) {
    char buffer[12];  // How much is enough?
    if (fabs(num) > 1e4 || (fabs(num) < 1e-4 && num != 0)) {
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
    sprintf(buffer, "M$M %lldK", (long long int) (cost / 1000));
    return _AddBuffer(buffer);
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
    sb.Add("########## ########## ############### ########## ## ## ## ## ############################## ##");
    sb.AutoBreak(21);
    const char* test_str_1 = "########## ##########\n###############\n########## ## ## ##\n##\n##############################\n##";
    TEST_ASSERT_STREQUAL(sb.c_str, test_str_1)

    return 0;
}
