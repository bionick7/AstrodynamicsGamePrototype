#ifndef DVECTOR3_H
#define DVECTOR3_H

#include <raylib.h>

struct DVector3 {
	// 24 bytes
	// Vector3, but with double precision
	static DVector3 Zero();
	static DVector3 One();
	static DVector3 Right();
	static DVector3 Left();
	static DVector3 Up();
	static DVector3 Down();
	static DVector3 Back();
	static DVector3 Forward();

	double x;
	double y;
	double z;

	DVector3();
	DVector3(Vector3 v);
	DVector3(double px, double py, double pz);

	double Length() const;
	double LengthSquared() const;
	DVector3 Normalized() const;
	double Dot(DVector3 v) const;
	DVector3 Cross(DVector3 v) const;
	DVector3 Project(DVector3 v) const;
	double AngleTo(DVector3 v) const;
	double SignedAngleTo(DVector3 v, DVector3 n) const;
	DVector3 Reflect(DVector3 n)const;
	DVector3 Bounce(DVector3 n)const;
	DVector3 Lerp(DVector3 v, double s) const;
	DVector3 Rotated(DVector3 axis, double angle) const;
	bool IsEqualApprox(DVector3 v) const;
	Quaternion RotationTo(DVector3 v) const;
	DVector3 CubicInterpolate(DVector3 pre, DVector3 from, 
                              DVector3 to, DVector3 post, double weight) const;
	DVector3 AnyOrthogonalDirection() const;
	DVector3 Vector3Transform(DVector3 col1, DVector3 col2, DVector3 col3, DVector3 offs) const;
	DVector3 Vector3RotateByQuaternion(Quaternion q) const;

    explicit operator Vector3() const;
};

DVector3 operator -(DVector3 v);
DVector3 operator +(DVector3 v, DVector3 w);
DVector3 operator -(DVector3 v, DVector3 w);
DVector3 operator *(DVector3 v, double s);
DVector3 operator *(double s, DVector3 v);
DVector3 operator /(DVector3 v, double s);
DVector3 operator *(Quaternion q, DVector3 v);
bool operator ==(DVector3 v, DVector3 w);
bool operator !=(DVector3 v, DVector3 w);



#endif  // DVECTOR3_H