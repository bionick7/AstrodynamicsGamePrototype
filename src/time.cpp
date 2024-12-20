#include "time.hpp"
#include "logging.hpp"
#include "datanode.hpp"

using namespace timemath;

Time::Time(double seconds) {
    __t = seconds;
}

Time Time::operator+(Time other) const {
    return Time(__t + other.__t);
}

Time Time::operator+(double seconds) const {
    return Time(__t + seconds);
}

Time Time::operator-(Time other) const {
    return Time(__t - other.__t);
}

Time Time::operator*(Time other) const {
    return Time(__t * other.__t);
}

Time Time::operator/(Time other) const {
    return Time(__t / other.__t);
}

bool Time::operator<(Time other) const {
    return __t < other.__t;
}

bool Time::operator>(Time other) const {
    return __t > other.__t;
}

bool Time::operator<=(Time other) const {
    return !(*this > other);
}

bool Time::operator>=(Time other) const {
    return !(*this < other);
}

Time Time::PosMod(Time mod) const {
    double t2 = fmod(__t, mod.__t);
    if (t2 < 0) t2 += mod.__t;
    return Time(t2);
}

bool Time::IsPos() const {
    return __t > 0;
}

double Time::Seconds() const {
    return __t;
}

double Time::Days() const {
    return __t / SECONDS_IN_DAY;
}

void Time::Serialize(DataNode* data, TableKey key) const {
    data->SetF(key, __t);
}

void Time::Deserialize(const DataNode* data, TableKey key) {
    __t = data->GetF(key, __t);
}

bool Time::IsInvalid() const {
    return isnan(__t);
}

double Time::SecDiff(Time lhs, Time rhs) {
    return lhs.__t - rhs.__t;
}

Time Time::Earliest(Time lhs, Time rhs) {
    return lhs < rhs ? lhs : rhs;
}

Time Time::Latest(Time lhs, Time rhs) {
    return lhs < rhs ? rhs : lhs;
}

Time Time::GetInvalid() {
    return Time(NAN);
}

#define TIMETEST_EQUAL(expr, seconds_res) if (fabs((expr).Seconds() - (seconds_res)) > 1e-10) { \
    ERROR("'%s' returned unexpected value: %f instead of %f", #expr, (expr).Seconds(), (seconds_res)) \
    return 1; \
}

int TimeTests() {
    const double v1 = 123.456, v2 = 456.789;
    Time t1 = Time(v1);
    Time t2 = Time(v2);

    TIMETEST_EQUAL(t1 + t2, v1 + v2)
    TIMETEST_EQUAL(t1 - t2, v1 - v2)
    TIMETEST_EQUAL(t2 - t1, v2 - v1)
    TIMETEST_EQUAL(Time(-340).PosMod(Time(100)), 60)
    if (fabs((t1 - t2).Seconds() - Time::SecDiff(t1, t2)) > 1e-10) {
        ERROR("TimeSub != TimeSecDiff")
        return 1;
    }
    if (fabs(Time::SecDiff((t1 + v2).Seconds(), (t1 + t2))) > 1e-10) {
        ERROR("TimeAdd != TimeAddSec")
        return 1;
    }
    DataNode dn = DataNode();
    t1.Serialize(&dn, "t");
    Time t1_prime;
    t1_prime.Deserialize(&dn, "t");
    if (fabs(Time::SecDiff(t1, t1_prime) > 1e-10)) {
        ERROR("Serialization is not the proper inverse of deserialization")
        return 1;
    }
    if (t1 > t2) return 1;

    return 0;
}