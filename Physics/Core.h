#pragma once

#include <math.h>

#include "Precision.h"

namespace Ruby { namespace Physics {

    class Vector3
    {
    public:
        real x;
        real y;
        real z;
    private:
        real pad; // Padding to ensure word alignment.
    public:
        Vector3() : x(0), y(0), z(0) {}
        Vector3(const real x, const real y, const real z)
            : x(x), y(y), z(z) {}

        void Invert()
        {
            x = -x;
            y = -y;
            z = -z;
        }

        real Magnitude() const
        {
            return real_sqrt(x * x + y * y + z * z);
        }

        real SquareMagnitude() const
        {
            return x * x + y * y + z * z;
        }

        void Normalize()
        {
            real l = Magnitude();
            if (l > 0)
            {
                (*this) *= ((real)1) / l;
            }
        }

        void operator*=(const real value)
        {
            x *= value;
            y *= value;
            z *= value;
        }

        Vector3 operator*(const real value) const
        {
            return Vector3(x * value, y * value, z * value);
        }

        void operator+=(const Vector3& v)
        {
            x += v.x;
            y += v.y;
            z += v.z;
        }

        Vector3 operator+(const Vector3& v) const
        {
            return Vector3(x + v.x, y + v.y, z + v.z);
        }

        void operator-=(const Vector3& v)
        {
            x -= v.x;
            y -= v.y;
            z -= v.z;
        }

        Vector3 operator-(const Vector3& v) const
        {
            return Vector3(x - v.x, y - v.y, z - v.z);
        }

        void AddScaleVector(const Vector3& vector, real scale)
        {
            x += vector.x * scale;
            y += vector.y * scale;
            z += vector.z * scale;
        }

        Vector3 ComponentProduct(const Vector3& vector) const
        {
            return Vector3(x * vector.x, y * vector.y, z * vector.z);
        }

        void ComponentProductUpdate(const Vector3& vector)
        {
            x *= vector.x;
            y *= vector.y;
            z *= vector.z;
        }

        real ScalarProduct(const Vector3& vector) const
        {
            return x * vector.x + y * vector.y + z * vector.z;
        }

        real operator*(const Vector3& vector) const
        {
            return x * vector.x + y * vector.y + z * vector.z;
        }

        Vector3 VectorProduct(const Vector3& vector) const
        {
            return Vector3(
                y * vector.z - z * vector.y,
                z * vector.x - x * vector.z,
                x * vector.y - y * vector.x);
        }

        void operator %=(const Vector3& vector)
        {
            *this = VectorProduct(vector);
        }

        Vector3 operator%(const Vector3& vector)
        {
            return Vector3(
                y * vector.z - z * vector.y,
                z * vector.x - x * vector.z,
                x * vector.y - y * vector.x);
        }

        void MakeOrthonormalBasis(Vector3* a, Vector3* b, Vector3* c)
        {
            a->Normalize();
            (*c) = (*b) % (*a);
            if (c->SquareMagnitude() == 0.0) return;
            c->Normalize();
            (*b) = (*a) % (*c);
        }

        void Clear()
        {
            x = y = z = 0;
        }

    };

} }

