#include "Collision.h"

namespace Ruby { namespace Physics {

	Line::Line(Vector3 start, Vector3 end)
		: start(start), end(end)
	{
	}
	Plane Triangle::GetPlane() const
	{
		Plane result;
		result.n = (b - a).VectorProduct(c - a);
		result.n.Normalize();
		result.d = result.n.ScalarProduct(a);
		return result;
	}

	bool Point::InTriangle(const Triangle& t)
	{
		// Create a tmp triangle with the size of the original triangle
		// in the local coordinate system of the point
		Vector3 p = *this;

		Vector3 a = t.a - p;
		Vector3 b = t.b - p;
		Vector3 c = t.c - p;

		// given the point p and triangle ABC, create the sides of a pyramid.
		// The sides of the pyramid will be triangles created from the point PBC, PCA, PAB.
		// then find and store the normal of each side if this pyramid
		Vector3 normPBC = b.VectorProduct(c);
		Vector3 normPCA = c.VectorProduct(a);
		Vector3 normPAB = a.VectorProduct(b);
		
		// if the face of the pyramid do not have the same normal, the point
		// is not contained within the triangle
		if(normPBC.ScalarProduct(normPCA) < 0.0)
		{
			return false;
		}
		if (normPBC.ScalarProduct(normPAB) < 0.0)
		{
			return false;
		}
		// if all faces of the pyramid have the same normal. the pyramid is flat.
		// this means the point is in the triangle and we have an intersection
		return true;		
	}
	Point Point::ClosestPointLine(const Line& line)
	{
		Point result;
		Vector3 point = *this;
		Vector3 lVec = line.end - line.start; // Line Vector

		real numerator = (point - line.start).ScalarProduct(lVec);
		real denominator = lVec.ScalarProduct(lVec);
		real t = numerator / denominator;
		t = fmax(t, 0.0);
		t = fmin(t, 1.0);
		point = line.start + lVec * t;
		result.x = point.x;
		result.y = point.y;
		result.z = point.z;
		return result;
	}
	Point Point::ClosestPointPlane(const Plane& plane)
	{
		Point result;
		Vector3 p = *this;
		real dot = plane.n.ScalarProduct(p);
		real distance = dot - plane.d;
		p = p - plane.n * distance;
		result.x = p.x;
		result.y = p.y;
		result.z = p.z;
		return result;
	}
	Point Point::ClosestPointTriangle(const Triangle& t)
	{
		Point p = *this;
		Plane plane = t.GetPlane();
		Point closest = ClosestPointPlane(plane);
		if (closest.InTriangle(t))
		{
			return closest;
		}
		// Construct one line for each side of the triangle. Find the closest
		// point on the side of the triangle to the test point
		Point c1 = ClosestPointLine(Line(t.a, t.b));
		Point c2 = ClosestPointLine(Line(t.b, t.c));
		Point c3 = ClosestPointLine(Line(t.c, t.a));

		// Measure how far each of the closest points from the previus step are from
		// the test point
		real magSq1 = (p - c1).SquareMagnitude();
		real magSq2 = (p - c2).SquareMagnitude();
		real magSq3 = (p - c3).SquareMagnitude();
		
		// return the closest one to the test point
		if(magSq1 <= magSq2 && magSq1 <= magSq3)
		{
			return c1;
		}
		else if (magSq2 <= magSq1 && magSq2 <= magSq3)
		{
			return c2;
		}
		return c3;
	}


	real Ray::RaycastPlane(const Plane& plane)
	{
		real nd = d.ScalarProduct(plane.n);
		real pn = o.ScalarProduct(plane.n);

		if (nd >= 0.0)
		{
			return -1.0;
		}
		real t = (plane.d - pn) / nd;

		if (t >= 0.0)
		{
			return t;
		}

		return -1.0;
	}

	real Ray::RaycastTriangle(const Triangle& tri)
	{
		Plane plane = tri.GetPlane();
		real t = RaycastPlane(plane);
		if (t < 0.0)
		{
			return t;
		}
		Vector3 p = o + d * t;
		Point point;
		point.x = p.x; point.y = p.y; point.z = p.z;
		if (point.InTriangle(tri))
		{
			return t;
		}
		return -1.0;
	}
} }
