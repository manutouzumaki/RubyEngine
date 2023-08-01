#pragma once

#include "Core.h"

namespace Ruby { namespace Physics {

    struct Point;
    struct Ray;
    struct Segment;

    struct Plane
    {
        Vector3 n;
        float d;
    };

    struct Sphere
    {
        Vector3 c;
        real r;

        int MovingSpherePlane(Vector3 v, Physics::Plane p, float& t, Vector3& q);
        int IntersectSegment(Segment segment, float& t, Vector3& q);
    };

    struct Capsule
    {
        Vector3 a;
        Vector3 b;
        real r;
    };

    struct Segment
    {
        Vector3 a;
        Vector3 b;

        int IntersectCapsule(const Capsule& capsule, float& t, Vector3& q);
    };

	struct Line
	{
		Line() {}
		Line(Vector3 start, Vector3 end);
		Vector3 start;
		Vector3 end;
	};

	struct Triangle
	{
		Vector3 a;
		Vector3 b;
		Vector3 c;

		Plane GetPlane() const;
	};

	struct Point : public Vector3
	{
	public:
        Point() {}
        Point(real x, real y, real z) : Vector3(x, y, z) {}
		bool InTriangle(const Triangle& t);
		Point ClosestPointLine(const Line& line);
		Point ClosestPointPlane(const Plane& plane);
		Point ClosestPointTriangle(const Triangle& t);
        Point ClosestPointSegement(const Segment& segment);
	};

	struct Ray
	{
		Vector3 o;
		Vector3 d;

        real SegmentCapsule(const Capsule& capsule);

        real RaycastSphere(const Sphere& s);
		real RaycastPlane(const Plane& plane);
		real RaycastTriangle(const Triangle& t);
	};

}}
