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

		ParticleBungee::ParticleBungee(Particle* other, real sc, real rl)
			: other(other), springConstant(sc), restLength(rl)
		{
		}
		void ParticleBungee::UpdateForces(Particle* particle, real duration)
		{
			// Calculate the vector of the spring
			Vector3 force;
			particle->GetPosition(&force);
			force -= other->GetPosition();
			// Check if the bungee is compressed
			real magnitude = force.Magnitude();
			if (magnitude <= restLength) return;
			// Calculate the magnitud if the force
			magnitude = springConstant * (restLength - magnitude);
			// Calculate the final force and apply it
			force.Normalize();
			force *= -magnitude;
			particle->AddForce(force);
		}

		ParticleFakeSpring::ParticleFakeSpring(Vector3* anchor, real sc, real d)
			: anchor(anchor), springConstant(sc), damping(d)
		{
		}
		void ParticleFakeSpring::UpdateForces(Particle* particle, real duration)
		{
			// Check that we dont have infinite mass
			if (!particle->HasFiniteMass()) return;

			// Calculate the relative position of the particle to the anchor
			Vector3 position;
			particle->GetPosition(&position);
			position -= *anchor;
			// Calculate the constants and check they are in bound
			real gamma = 0.5 * real_sqrt(4 * springConstant - damping * damping);
			if (gamma == 0.0) return;
			Vector3 c = position * (damping / (2.0 * gamma)) + particle->GetVelocity() * (1.0 / gamma);
			// Calculate the target position
			Vector3 target = position * real_cos(gamma * duration) + c * real_sin(gamma * duration);
			target *= real_exp(-0.5 * duration * damping);
			// Calculate the resulting acceleration and therefore the force
			Vector3 accel = (target - position) * ((real)1.0 / (duration * duration)) -
				particle->GetVelocity() * ((real)1.0 / duration);
			particle->AddForce(accel * particle->GetMass());
		}

		ParticleAnchoredSpring::ParticleAnchoredSpring()
		{
		}
		ParticleAnchoredSpring::ParticleAnchoredSpring(Vector3* anchor, real sc, real rl)
			: anchor(anchor), springConstant(sc), restLength(rl)
		{
		}
		void ParticleAnchoredSpring::Init(Vector3* anchor, real springConstant, real restLength)
		{
			ParticleAnchoredSpring::anchor = anchor;
			ParticleAnchoredSpring::springConstant = springConstant;
			ParticleAnchoredSpring::restLength = restLength;
		}
		void ParticleAnchoredSpring::UpdateForces(Particle* particle, real duration)
		{
			// Calculate the vector of the spring
			Vector3 force;
			particle->GetPosition(&force);
			force -= *anchor;
			// Calculate the magnitud of the force
			real magnitude = force.Magnitude();
			magnitude = (restLength - magnitude) * springConstant;
			// Calculate the final force and apply it
			force.Normalize();
			force *= magnitude;
			particle->AddForce(force);
		}

		void ParticleAnchoredBungee::UpdateForces(Particle* particle, real duration)
		{
			// Calculate the vector of the spring
			Vector3 force;
			particle->GetPosition(&force);
			force -= *anchor;

			// Calculate the magnitude of the force
			real magnitude = force.Magnitude();
			if (magnitude < restLength) return;

			magnitude = magnitude - restLength;
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