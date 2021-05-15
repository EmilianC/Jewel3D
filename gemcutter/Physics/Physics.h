#pragma once
#include "gemcutter/Entity/Entity.h"

namespace gem
{
	// A tag identifying entities which must be updated to match their non-static rigid bodies.
	class DynamicRigidBodyTag : public Tag<DynamicRigidBodyTag> {};
	// A tag identifying non-static rigid bodies which update to follow their entities.
	class KinematicRigidBodyTag : public Tag<KinematicRigidBodyTag> {};

	enum class RigidBodyType
	{
		// Physics body sets Entity position.
		Dynamic   = 0x0001,
		// Entity sets physics body position.
		Kinematic = 0x0010,
		// Physics body never moves.
		Static    = 0x0100,
		// Entity Sets physics body position.
		Trigger   = 0x1000
	};
}
