#include "Collision.h"

namespace Ruby { namespace Physics {

    float Clamp(float a, float min, float max)
    {
        return fminf(fmaxf(a, min), max);
    }

    float ClosestPtSegmentSegment(Vector3 p1, Vector3 q1, Vector3 p2, Vector3 q2, float& s, float& t, Vector3& c1, Vector3& c2) {
        Vector3 d1 = q1 - p1; // direction vector of segment s1
        Vector3 d2 = q2 - p2; // direction vector of segment s2
        Vector3 r = p1 - p2;
        float a = d1.ScalarProduct(d1); // squared length of segment S1
        float e = d2.ScalarProduct(d2); // squared length of segment S2
        float f = d2.ScalarProduct(r);
        // check if either or both segments degenerate into points
        if (a <= FLT_EPSILON && e <= FLT_EPSILON) {
            // Both segments degenerate into points
            s = t = 0.0f;
            c1 = p1;
            c2 = p2;
            return (c1 - c2).ScalarProduct(c1 - c2);
        }
        if (a <= FLT_EPSILON) {
            // first segment degenerate into a point
            s = 0.0f;
            t = f / e;
            t = Clamp(t, 0.0f, 1.0f);
        }
        else {
            float c = d1.ScalarProduct(r);
            if (e <= FLT_EPSILON) {
                // second segment degenerate into a point
                t = 0.0f;
                s = Clamp(-c / a, 0.0f, 1.0f);
            }
            else {
                // the general nondegenerate case start here
                float b = d1.ScalarProduct(d2);
                float denom = a * e - b * b;
                // if segments not parallel, compute closest point on L1 to L2 and
                // clamp to segment S1. Else pick arbitrary s (here 0)
                if (denom != 0.0f) {
                    s = Clamp((b * f - c * e) / denom, 0.0f, 1.0f);
                }
                else s = 0.0f;
                // compute point on L2 closest to S1(s) using
                // t = dot((P1 + D1*s) - P2, D2) / dot(D2, D2) = (b*s + f) / e
                t = (b * s + f) / e;

                if (t < 0.0f) {
                    t = 0.0f;
                    s = Clamp(-c / a, 0.0f, 1.0f);
                }
                else if (t > 1.0f) {
                    t = 1.0f;
                    s = Clamp((b - c) / a, 0.0f, 1.0f);
                }
            }
        }
        c1 = p1 + d1 * s;
        c2 = p2 + d2 * t;
        return (c1 - c2).ScalarProduct(c1 - c2);
    }

    int Sphere::MovingSpherePlane(Vector3 v, Plane p, float& t, Vector3& q)
    {
        float dist = p.n.ScalarProduct(c) - p.d;
        if (fabsf(dist) <= this->r)
        {
            t = 0.0f;
            q = c;
            return 1;
        }
        else
        {
            float denom = p.n.ScalarProduct(v);
            if (denom * dist >= 0.0f)
            {
                return 0;
            }
            else
            {
                float r = dist > 0.0f ? this->r : -this->r;
                t = (r - dist) / denom;
                q = c + v * t - p.n * r;
                return 1;
            }
        }
    }

    int Sphere::IntersectSegment(Segment segment, float& t, Vector3& q) {
        Vector3 d = segment.b - segment.a;
        Vector3 m = segment.a - c;

        float b = m.ScalarProduct(d);
        float c = m.ScalarProduct(m) - r * r;
        if (c > 0.0f && b > 0.0f) return 0;
        float discr = b * b - c;
        if (discr < 0.0f) return 0;
        t = -b - sqrtf(discr);
        if (t < 0.0f) t = 0.0f;

        q = segment.a + d * t;
        return 1;
    }

    int Segment::IntersectCapsule(const Capsule& capsule, float& tout, Vector3& q)
    {
        Vector3 c1;
        Vector3 c2;
        float s = 0;
        float t = 0;
        float sqDist = ClosestPtSegmentSegment(a, b, capsule.a, capsule.b, s, t, c1, c2);
        bool result = sqDist <= capsule.r * capsule.r;
        if (result)
        {
            Sphere sphere;
            sphere.c = c2;
            sphere.r = capsule.r;
            sphere.IntersectSegment(*this, t, q);
            tout = t;
            return 1;
        }

        return 0;
    }

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

    Point Point::ClosestPointSegement(const Segment&segment) {
        Vector3 q;
        Vector3 ab = segment.b - segment.a;
        float t = (*this - segment.a).ScalarProduct(ab);
        if (t <= 0.0f) {
            t = 0.0f;
            q = segment.a;
            return Point(q.x, q.y, q.z);
        }
        else {
            float denom = ab.ScalarProduct(ab);
            if (t >= denom) {
                t = 1.0f;
                q = segment.b;
                return Point(q.x, q.y, q.z);
            }
            else {
                t = t / denom;
                q = segment.a + ab * t;
                return Point(q.x, q.y, q.z);
            }
        }
    }

    real Ray::RaycastSphere(const Sphere& s)
    {
        // construct a vector from the origin of the ray to the center of the sphere
        Vector3 e = s.c - o;
        // store the squared magnitude of this new vector, as well as the squared radius of the spherer
        real rSq = s.r * s.r;
        real eSq = e.SquareMagnitude();
        // Project the vector pointing from the origin of the ray to the sphere onto the direction of the ray

        Vector3 normDir = d;
        normDir.Normalize();

        real a = e.ScalarProduct(normDir);

        real bSq = eSq - (a * a);
        real f = real_sqrt(rSq - bSq);

        if (rSq - (eSq - (a * a)) < 0.0f) 
        {
            return -1.0;
        }
        else if (eSq < rSq)
        {
            return a + f;
        }
        return a - f;
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
