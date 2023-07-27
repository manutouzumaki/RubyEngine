#pragma once

#include "Particle.h"

namespace Ruby { namespace Physics {

	class ParticleContact
	{
		friend class ParticleContactResolver;

	public:
		// Hold the particle that are involved in the contact. the
		// second of these can be NULL, for contacts with the scenery
		Particle* particle[2];

		// Holds the normal restitution coefficient at the contact
		real restitution;

		// Holds the direction of the contact normal in world space
		Vector3 contactNormal;

		// Holds the depth of penetration at the contact
		real penetration;

		// Holds the amount each particle is moved by during interpenetration
		Vector3 particleMovement[2];

	protected:

		// resolve this contact, for both velocity and interpenetration
		void Resolve(real duration);
		// Calculates the separating velocity at this contact
		real CalculateSeparatingVelocity() const;

	private:

		void ResolveVelocity(real duration);
		void ResolveInterpenetration(real duration);
	};

	class ParticleContactResolver
	{
	protected:
		unsigned iterations;
		unsigned iterationsUsed;
	public:
		ParticleContactResolver(unsigned iterations);
		void SetIterations(unsigned iterations);
		void ResolveContacts(ParticleContact* contactArray, unsigned numContacts, real duration);

	};

}}



