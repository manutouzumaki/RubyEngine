#include "ParticleContact.h"

namespace Ruby { namespace Physics {

	void ParticleContact::Resolve(real duration)
	{
		ResolveVelocity(duration);
		ResolveInterpenetration(duration);
	}

	real ParticleContact::CalculateSeparatingVelocity() const
	{
		Vector3 relativeVelocity = particle[0]->GetVelocity();
		if (particle[1]) relativeVelocity -= particle[1]->GetVelocity();
		return relativeVelocity * contactNormal;
	}

	void ParticleContact::ResolveVelocity(real duration)
	{
		// Find the velocity in the direction of the contact
		real separatingVelocity = CalculateSeparatingVelocity();
		// Check is needs to be resolve
		if (separatingVelocity > 0)
		{
			return;
		}

		// Calculate the new separating velocity
		real newSepVelocity = -separatingVelocity * restitution;
		// Check the velocity build-up due to acceleration only
		Vector3 accCausedVelocity = particle[0]->GetAcceleration();
		if (particle[1]) accCausedVelocity -= particle[1]->GetAcceleration();
		real accCausedSepVelocity = accCausedVelocity * contactNormal * duration;

		// If we've got a closing velocity due to acceleration build-up,
		// remove it from the new separating velocity
		if (accCausedSepVelocity < 0)
		{
			newSepVelocity += restitution * accCausedSepVelocity;
			// make sure we havent removed more that was there to remove
			if (newSepVelocity < 0) newSepVelocity = 0;
		}

		real deltaVelocity = newSepVelocity - separatingVelocity;

		// We apply the change in velocity to each object in proportion to
		// their inverse mass (i.e. those with lower inverse mass [higher
		// actual mass] get less change in velocity)..
		real totalInverseMass = particle[0]->GetInverseMass();
		if (particle[1]) totalInverseMass += particle[1]->GetInverseMass();

		// If all particles have infinite mass, then impulses have no effect
		if (totalInverseMass <= 0) return;

		// Calculate the impulse to apply
		real impulse = deltaVelocity / totalInverseMass;

		// Find the amount of impulse per unit of inverse mass
		Vector3 impulsePerIMass = contactNormal * impulse;

		// Apply impulses: they are applied in the direction of the contact,
		// and are proportional to the inverse mass.
		particle[0]->SetVelocity(particle[0]->GetVelocity() +
			impulsePerIMass * particle[0]->GetInverseMass()
		);
		if (particle[1])
		{
			// Particle 1 goes in the opposite direction
			particle[1]->SetVelocity(particle[1]->GetVelocity() +
				impulsePerIMass * -particle[1]->GetInverseMass()
			);
		}

	}

	void ParticleContact::ResolveInterpenetration(real duration)
	{
		// If we don't have any penetration, skip this step.
		if (penetration <= 0) return;

		// The movement of each object is based on their inverse mass, so
		// total that.
		real totalInverseMass = particle[0]->GetInverseMass();
		if (particle[1]) totalInverseMass += particle[1]->GetInverseMass();

		// If all particles have infinite mass, then we do nothing
		if (totalInverseMass <= 0) return;

		// Find the amount of penetration resolution per unit of inverse mass
		Vector3 movePerIMass = contactNormal * (penetration / totalInverseMass);

		// Calculate the the movement amounts
		particleMovement[0] = movePerIMass * particle[0]->GetInverseMass();
		if (particle[1]) {
			particleMovement[1] = movePerIMass * -particle[1]->GetInverseMass();
		}
		else {
			particleMovement[1].Clear();
		}

		// Apply the penetration resolution
		particle[0]->SetPosition(particle[0]->GetPosition() + particleMovement[0]);
		if (particle[1]) {
			particle[1]->SetPosition(particle[1]->GetPosition() + particleMovement[1]);
		}
	}



	ParticleContactResolver::ParticleContactResolver(unsigned iterations)
		:iterations(iterations), iterationsUsed(0)
	{
	}

	void ParticleContactResolver::SetIterations(unsigned iterations)
	{
		ParticleContactResolver::iterations = iterations;
	}

	void ParticleContactResolver::ResolveContacts(ParticleContact* contactArray,
		unsigned numContacts, real duration)
	{
		unsigned i;

		iterationsUsed = 0;
		while (iterationsUsed < iterations)
		{
			// Find the contact with the largest closing velocity;
			real max = REAL_MAX;
			unsigned maxIndex = numContacts;
			for (i = 0; i < numContacts; i++)
			{
				real sepVel = contactArray[i].CalculateSeparatingVelocity();
				if (sepVel < max &&
					(sepVel < 0 || contactArray[i].penetration > 0))
				{
					max = sepVel;
					maxIndex = i;
				}
			}

			// Do we have anything worth resolving?
			if (maxIndex == numContacts) break;

			// Resolve this contact
			contactArray[maxIndex].Resolve(duration);

			// Update the interpenetrations for all particles
			Vector3* move = contactArray[maxIndex].particleMovement;
			for (i = 0; i < numContacts; i++)
			{
				if (contactArray[i].particle[0] == contactArray[maxIndex].particle[0])
				{
					contactArray[i].penetration -= move[0] * contactArray[i].contactNormal;
				}
				else if (contactArray[i].particle[0] == contactArray[maxIndex].particle[1])
				{
					contactArray[i].penetration -= move[1] * contactArray[i].contactNormal;
				}
				if (contactArray[i].particle[1])
				{
					if (contactArray[i].particle[1] == contactArray[maxIndex].particle[0])
					{
						contactArray[i].penetration += move[0] * contactArray[i].contactNormal;
					}
					else if (contactArray[i].particle[1] == contactArray[maxIndex].particle[1])
					{
						contactArray[i].penetration += move[1] * contactArray[i].contactNormal;
					}
				}
			}

			iterationsUsed++;
		}
	}

} }