#pragma once

#include "Core.h"
#include "Particle.h"

#include <vector>

namespace Ruby {
	namespace Physics {

		class ParticleForceGenerator
		{
		public:
			virtual void UpdateForces(Particle* particle, real duration) = 0;
		};

		class ParticleGravity : public ParticleForceGenerator
		{
			Vector3 gravity;
		public:
			ParticleGravity(const Vector3& gravity);
			virtual void UpdateForces(Particle* particle, real duration);
		};

		class ParticleDrag : public ParticleForceGenerator
		{
			// Holds the velocity drag coeffificent.
			real k1;
			// holds the velocity squared drag coeffificent.
			real k2;
		public:
			ParticleDrag(real k1, real k2);
			virtual void UpdateForces(Particle* particle, real duration);
		};

		class ParticleForceRegistry
		{
		protected:

			struct ParticleForceRegistration
			{
				Particle* particle;
				ParticleForceGenerator* fg;
			};

			typedef std::vector<ParticleForceRegistration>  Registry;
			Registry registrations;

		public:

			// Registry the given force to apply to the given particle
			void Add(Particle* particle, ParticleForceGenerator* fg);

			// Clears all registrations from the registry, this will not delete the
			// particles or the force generatos themselves
			void Clear();

			// Calls all the force generators to update the forces of their corresponding
			// particles
			void UpdateForces(real duration);

		};

		class ParticleAnchoredSpring : public ParticleForceGenerator
		{
		protected:
			Vector3* anchor;
			real springConstant;
			real restLength;
		public:
			ParticleAnchoredSpring();
			ParticleAnchoredSpring(Vector3* anchor, real springConstant, real restLength);

			const Vector3* GetAnchor() const { return anchor; }

			void Init(Vector3* anchor, real springConstant, real restLength);
			virtual void UpdateForces(Particle* particle, real duration);
		};

		class ParticleAnchoredBungee : public ParticleAnchoredSpring 
		{
		public:
			virtual void UpdateForces(Particle* particle, real duration);
		};

		class ParticleFakeSpring : public ParticleForceGenerator
		{
			Vector3* anchor;
			real springConstant;
			real damping;
		public:
			ParticleFakeSpring(Vector3* anchor, real springConstant, real damping);
			virtual void UpdateForces(Particle* particle, real duration);
		};

		class ParticleSpring : public ParticleForceGenerator
		{
			Particle* other;
			real springConstant;
			real restLength;
		public:
			ParticleSpring(Particle* other, real springConstant, real restLength);
			virtual void UpdateForces(Particle* particle, real duration);
		};

		class ParticleBungee : public ParticleForceGenerator
		{
			Particle* other;
			real springConstant;
			real restLength;
		public:
			ParticleBungee(Particle* other, real springConstant, real restLength);
			virtual void UpdateForces(Particle* particle, real duration);
		};

	}
}

