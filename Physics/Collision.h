#pragma once

#include "Core.h"

namespace Ruby { namespace Physics {

	struct Plane
	{
		Vector3 n;
		float d;
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
		bool InTriangle(const Triangle& t);
		Point ClosestPointLine(const Line& line);
		Point ClosestPointPlane(const Plane& plane);
		Point ClosestPointTriangle(const Triangle& t);
	};

	struct Ray
	{
		Vector3 o;
		Vector3 d;
		
		real RaycastPlane(const Plane& plane);
		real RaycastTriangle(const Triangle& t);
	};

}}
