#include "ParticleForceGenerator.h"
namespace Ruby {
	namespace Physics {

		ParticleGravity::ParticleGravity(const Vector3& gravity)
			: gravity(gravity)
		{
		}
		void ParticleGravity::UpdateForces(Particle* particle, real duration)
		{
			// check that we do not have infinite mass
			if (!particle->HasFiniteMass()) return;

			// Apply the mass-scaled force to the particle
			particle->AddForce(gravity * particle->GetMass());
		}

		ParticleDrag::ParticleDrag(real k1, real k2)
			: k1(k1), k2(k2)
		{
		}
		void ParticleDrag::UpdateForces(Particle* particle, real duration)
		{
			Vector3 force;
			particle->GetVelocity(&force);

			// calculate the total drag coefficient
			real dragCoeff = force.Magnitude();
			dragCoeff = k1 * dragCoeff + k2 * dragCoeff * dragCoeff;

			// calculate the final force and apply it
			force.Normalize();
			force *= -dragCoeff;
			particle->AddForce(force);
		}

		ParticleSpring::ParticleSpring(Particle* other, real sc, real rl)
			: other(other), springConstant(sc), restLength(rl)
		{
		}

		void ParticleSpring::UpdateForces(Particle* particle, real duration)
		{
			// Calculate the vector of the spring
			Vector3 force;
			particle->GetPosition(&force);
			force -= other->GetPosition();
			// Calculate the magnitude of the force
			real magnitude = force.Magnitude();
			magnitude = real_abs(magnitude - restLength);
			magnitude *= springConstant;
			// Calculate the final force and apply it
			force.Normalize();
			force *= -magnitude;
			particle->AddForce(force);
		}




		// Registry the given force to apply to the given particle
		void ParticleForceRegistry::Add(Particle* particle, ParticleForceGenerator* fg)
		{
			ParticleForceRegistration registration;
			registration.particle = particle;
			registration.fg = fg;
			registrations.push_back(registration);
		}

		// Clears all registrations from the registry, this will not delete the
		// particles or the force generatos themselves
		void ParticleForceRegistry::Clear()
		{
			registrations.clear();
		}

		// Calls all the force generators to update the forces of their corresponding
		// particles
		void ParticleForceRegistry::UpdateForces(real duration)
		{
			Registry::iterator i = registrations.begin();
			for (; i != registrations.end(); i++)
			{
				i->fg->UpdateForces(i->particle, duration);
			}
		}

	}
}