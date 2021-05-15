#pragma once
#include "gemcutter/Math/Vector.h"
#include "gemcutter/Physics/Physics.h"
#include "gemcutter/Utilities/EnumFlags.h"

#include <box2d/box2d.h>

namespace gem
{
	class RigidBody2d;

	struct PhysicsParams
	{
		vec2 gravity = {0.0f, -9.81f};
		int m_velocityIterations = 8;
		int m_positionIterations = 3;
	};

	struct RayCastResult
	{
		bool isValid = false;
		b2Fixture* fixture = nullptr;
		RigidBody2d* component = nullptr;
		vec2 point;
		vec2 normal;
		float distance = 0.0f;
	};

	extern class PhysicsWorldSingleton PhysicsWorld;
	class PhysicsWorldSingleton
	{
		friend RigidBody2d;
	public:
		PhysicsWorldSingleton(const PhysicsParams& worldParams);
		~PhysicsWorldSingleton();

		void SetGravity(vec2 gravity);
		void SetVelocityIterations(int iterations);
		void SetPositionIterations(int iterations);

		vec2 GetGravity() const;
		int  GetVelocityIterations() const;
		int  GetPositionIterations() const;

		void Update(float deltaTime);

		template<typename Functor>
		RayCastResult RayCast(vec2 start, vec2 end, Functor&& func) const;
		RayCastResult RayCast(vec2 start, vec2 end, EnumFlags<RigidBodyType> mask) const;

		template<typename Functor>
		unsigned QueryAABB(vec2 pos, vec2 size, Functor&& func) const;

		void SetDebugEnabled(bool enabled);
		bool GetDebugEnabled() const;
		void DrawDebug();

	private:
		b2World world;
		PhysicsParams params;

		void FinalizeResult(vec2 rayStart, RayCastResult& hitResult) const;
		void DrawRayDebug(vec2 start, vec2 end, const RayCastResult& hitResult) const;

	#ifdef _DEBUG
		class PhysicsDebug* debugRenderer = nullptr;
	#endif
	};
}

#include "PhysicsWorld.inl"
