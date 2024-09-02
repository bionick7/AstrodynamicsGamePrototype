#include <math.h>

#include "dvector3.hpp"
#include "logging.hpp"

DVector3 DVector3::Zero()    { return DVector3(0, 0, 0); }
DVector3 DVector3::One()     { return DVector3(1, 1, 1); }
DVector3 DVector3::Right()   { return DVector3(1, 0, 0); }
DVector3 DVector3::Left()    { return DVector3(-1, 0, 0); }
DVector3 DVector3::Up()      { return DVector3(0, 1, 0); }
DVector3 DVector3::Down()    { return DVector3(0, -1, 0); }
DVector3 DVector3::Back()    { return DVector3(0, 0, 1); }
DVector3 DVector3::Forward() { return DVector3(0, 0, -1); }

DVector3::DVector3() {
    x = 0; y = 0; z = 0;
}

DVector3::DVector3(double px, double py, double pz)
{
    x = px;
    y = py;
    z = pz;
}

double DVector3::Length() const {
    return sqrt(LengthSquared());
}

double DVector3::LengthSquared() const {
    return Dot(*this);
}

DVector3 DVector3::Normalized() const {
    double l_sqr = LengthSquared();
    if (abs(l_sqr - 1) < 1e-12) return *this;
    if (abs(l_sqr) < 1e-12) return Zero();
    return *this / sqrt(l_sqr);
}

double DVector3::Dot(DVector3 v) const {
    return x * v.x + y * v.y + z * v.z;
}

DVector3 DVector3::Cross(DVector3 v) const {
    return DVector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
}

DVector3 DVector3::Project(DVector3 v) const {
    return v.LengthSquared() == 0 ? v : v * Dot(v) / v.LengthSquared();
}

double DVector3::AngleTo(DVector3 v) const {
    return atan2(Cross(v).Length(), Dot(v));
}

double DVector3::SignedAngleTo(DVector3 v, DVector3 n) const {
    return AngleTo(v) * (Cross(v).Dot(n) > 0 ? 1 : -1);
}

DVector3 DVector3::Reflect(DVector3 n) const {
    if (abs(n.LengthSquared() - 1.0) > 1e-14) ERROR("normal Vector not normalized");
    return 2.0 * n * Dot(n) - *this;
}

DVector3 DVector3::Bounce(DVector3 n) const {
    return -Reflect(n);
}

DVector3 DVector3::Lerp(DVector3 v, double s) const {
    return *this + s * (v - *this);
}

DVector3 DVector3::Rotated(DVector3 axis, double angle) const {
    // Using Euler-Rodrigues Formula
    // Ref.: https://en.wikipedia.org/w/index.php?title=Euler%E2%80%93Rodrigues_formula

    axis = axis.Normalized();
    DVector3 w = axis * sin(angle / 2.0);
    double a = cos(angle / 2.0);

    DVector3 wv = -Cross(w);
    return *this + 2*a * wv + 2*w.Cross(wv);
}

bool DVector3::IsEqualApprox(DVector3 v) const {
    return (*this - v).Length() < 1e-6;
}

Quaternion DVector3::RotationTo(DVector3 v) const {
    NOT_IMPLEMENTED
}

DVector3 DVector3::CubicInterpolate(DVector3 pre, DVector3 from, 
                                    DVector3 to, DVector3 post, double weight) const {
    return 0.5 * (
        (from * 2.0) +
        (-pre + to) * weight +
        (2.0 * pre - 5.0 * from + 4.0 * to - post) * (weight * weight) +
        (-pre + 3.0 * from - 3.0 * to + post) * (weight * weight * weight)
    );
}

DVector3 DVector3::AnyOrthogonalDirection() const {
    DVector3 v = this->Dot(DVector3(1, 0, 1)) == 0 ? Right() : Up();
    v = v - v.Project(*this);
    return v.Normalized();
}

DVector3 DVector3::Vector3Transform(DVector3 col1, DVector3 col2, DVector3 col3, DVector3 offs) const {
    DVector3 result;

    result.x = col1.x*x + col2.x*y + col3.x*z + offs.x;
    result.y = col1.y*x + col2.y*y + col3.y*z + offs.y;
    result.z = col1.z*x + col2.z*y + col3.z*z + offs.z;

    return result;
}

DVector3 DVector3::Vector3RotateByQuaternion(Quaternion q) const {
    DVector3 result;

    result.x = x*(q.x*q.x + q.w*q.w - q.y*q.y - q.z*q.z) + y*(2*q.x*q.y - 2*q.w*q.z) + z*(2*q.x*q.z + 2*q.w*q.y);
    result.y = x*(2*q.w*q.z + 2*q.x*q.y) + y*(q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z) + z*(-2*q.w*q.x + 2*q.y*q.z);
    result.z = x*(-2*q.w*q.y + 2*q.x*q.z) + y*(2*q.w*q.x + 2*q.y*q.z)+ z*(q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z);

    return result;
}

DVector3::operator Vector3() const {
    return (Vector3) {(float)x, (float)y, (float)z};
}

DVector3 operator -(DVector3 v) {
    return DVector3(-v.x, -v.y, -v.z);
}

DVector3 operator +(DVector3 v, DVector3 w) {
    return DVector3(v.x + w.x, v.y + w.y, v.z + w.z);
}

DVector3 operator -(DVector3 v, DVector3 w) {
    return DVector3(v.x - w.x, v.y - w.y, v.z - w.z);
}

DVector3 operator *(DVector3 v, double s) {
    return DVector3(v.x * s, v.y * s, v.z * s);
}

DVector3 operator *(double s, DVector3 v) {
    return DVector3(v.x * s, v.y * s, v.z * s);
}

DVector3 operator /(DVector3 v, double s) {
    return DVector3(v.x / s, v.y / s, v.z / s);
}

DVector3 operator *(Quaternion q, DVector3 v) {
    DVector3 u = DVector3(q.x, q.y, q.z);
    DVector3 uv = u.Cross(v);
    return v + ((uv * q.w) + u.Cross(uv)) * 2;
}

bool operator ==(DVector3 v, DVector3 w) {
    return v.x == w.x && v.y == w.y && v.z == w.z;
}

bool operator !=(DVector3 v, DVector3 w) {
    return !(v == w);
}
