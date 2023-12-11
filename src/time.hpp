#ifndef TIME_H
#define TIME_H
#include "basic.hpp"

struct DataNode;

namespace timemath {
    struct Time {
        double __t;
        Time() : Time(0.0) {};
        Time(double seconds);
        
        Time operator+(Time other) const;
        Time operator+(double other) const;
        Time operator-(Time other) const;
        Time PosMod(Time mod) const;
        bool operator<(Time other) const;
        bool operator>(Time other) const;
        bool IsPos() const;

        double Seconds() const;
        double Days() const;

        void Serialize(DataNode* data) const;
        void Deserialize(const DataNode* data);

        bool IsInvalid() const;

        char* FormatAsTime(char* buffer, int buffer_len) const;
        char* FormatAsDate(char* buffer, int buffer_len) const;

        static double SecDiff(Time lhs, Time rhs);
        static Time Earliest(Time lhs, Time rhs);
        static Time Latest(Time lhs, Time rhs);
        static Time GetInvalid();
    };

}
int TimeTests();
#endif  // TIME_H
