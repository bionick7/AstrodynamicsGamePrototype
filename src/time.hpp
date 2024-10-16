#ifndef TIME_H
#define TIME_H
#include "basic.hpp"

struct DataNode;
struct TableKey;

namespace timemath {
    const int SECONDS_IN_DAY = 86400;

    struct Time {
        double __t;
        Time() : Time(0.0) {};
        Time(double seconds);
        
        Time operator+(Time other) const;
        Time operator+(double other) const;
        Time operator-(Time other) const;
        Time operator*(Time other) const;
        Time operator/(Time other) const;
        Time PosMod(Time mod) const;
        bool operator<(Time other) const;
        bool operator>(Time other) const;
        bool operator<=(Time other) const;
        bool operator>=(Time other) const;
        bool IsPos() const;

        double Seconds() const;
        double Days() const;

        void Serialize(DataNode* data, TableKey key) const;
        void Deserialize(const DataNode* data, TableKey key);

        bool IsInvalid() const;

        static double SecDiff(Time lhs, Time rhs);
        static Time Earliest(Time lhs, Time rhs);
        static Time Latest(Time lhs, Time rhs);
        static Time GetInvalid();

        static Time Day() { return Time(SECONDS_IN_DAY); };
    };

}
int TimeTests();
#endif  // TIME_H
