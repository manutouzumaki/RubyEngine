#pragma once

#include "Core.h"

namespace Ruby { namespace Physics {

    class Particle
    {
    protected:
        Vector3 position; // linear position of the particle in world space
        Vector3 velocity; // linear velocity of the particle in world space
        Vector3 acceleration;
        Vector3 forceAccum;
        real damping; // holds the amount of dampling applied to linear motion
        real inverseMass; // holds the inverse of the mass of the particle 
    public:

        void Integrate(real duration);

        void ClearAccumulator();
        void AddForce(const Vector3& force);
        
        void SetMass(const real mass);
        real GetMass() const;
        void SetInverseMass(const real inverseMass);
        real GetInverseMass();
        bool HasFiniteMass() const;
        
        void SetDamping(const real damping);
        real GetDamping() const;

        void SetPosition(const Vector3& position);
        void SetPosition(const real x, const real y, const real z);
        void GetPosition(Vector3* position) const;
        Vector3 GetPosition() const;

        void SetVelocity(const Vector3& velocity);
        void SetVelocity(const real x, const real y, const real z);
        void GetVelocity(Vector3* velocity) const;
        Vector3 GetVelocity() const;

        void SetAcceleration(const Vector3& acceleration);
        void SetAcceleration(const real x, const real y, const real z);
        void GetAcceleration(Vector3* acceleration) const;
        Vector3 GetAcceleration() const;


    };


} }

