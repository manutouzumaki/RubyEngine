#include "Particle.h"
#include "../RubyDefines.h"

namespace Ruby { namespace Physics {


    void Particle::Integrate(real duration)
    {
        // We don't integrate things with infinite mass.
        if (inverseMass <= 0.0) return;
        Assert(duration > 0.0);
        
        // update linear position
        position.AddScaleVector(velocity, duration);

        // work out the acceleration from the force
        // (We'll add to this vector when we come to generate forces.)
        Vector3 resultingAcc = acceleration;
        resultingAcc.AddScaleVector(forceAccum, inverseMass);

        // update linear velocity from the acceleration
        velocity.AddScaleVector(resultingAcc, duration);

        // Impose drag
        velocity *= real_pow(damping, duration);

        // Clear the forces
        ClearAccumulator();
    }

    void Particle::SetMass(const real mass)
    {
        Assert(mass != 0);
        inverseMass = ((real)1.0) / mass;
    }

    real Particle::GetMass() const
    {
        if (inverseMass == 0)
        {
            return REAL_MAX;
        }
        else
        {
            return ((real)1.0) / inverseMass;
        }
    }

    void Particle::SetInverseMass(const real inverseMass)
    {
        Particle::inverseMass = inverseMass;
    }

    real Particle::GetInverseMass()
    {
        return inverseMass;
    }

    bool Particle::HasFiniteMass() const
    {
        return inverseMass >= 0.0;
    }

    void Particle::SetDamping(const real damping)
    {
        Particle::damping = damping;
    }

    real Particle::GetDamping() const
    {
        return damping;
    }

    void Particle::SetPosition(const Vector3& position)
    {
        Particle::position = position;
    }

    void Particle::SetPosition(const real x, const real y, const real z) 
    {
        position.x = x;
        position.y = y;
        position.z = z;
    }

    void Particle::GetPosition(Vector3* position) const
    {
        *position = Particle::position;
    }

    Vector3 Particle::GetPosition() const
    {
        return position;
    }

    void Particle::SetVelocity(const Vector3& velocity)
    {
        Particle::velocity = velocity;
    }

    void Particle::SetVelocity(const real x, const real y, const real z)
    {
        velocity.x = x;
        velocity.y = y;
        velocity.z = z;
    }

    void Particle::GetVelocity(Vector3* velocity) const
    {
        *velocity = Particle::velocity;
    }

    Vector3 Particle::GetVelocity() const
    {
        return velocity;
    }

    void Particle::SetAcceleration(const Vector3& acceleration)
    {
        Particle::acceleration = acceleration;
    }

    void Particle::SetAcceleration(const real x, const real y, const real z)
    {
        acceleration.x = x;
        acceleration.y = y;
        acceleration.z = z;
    }

    void Particle::GetAcceleration(Vector3* acceleration) const
    {
        *acceleration = Particle::acceleration;
    }

    Vector3 Particle::GetAcceleration() const
    {
        return acceleration;
    }

    void Particle::ClearAccumulator()
    {
        forceAccum.Clear();
    }

    void Particle::AddForce(const Vector3& force)
    {
        forceAccum += force;
    }

}}