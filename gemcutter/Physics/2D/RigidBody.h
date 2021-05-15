#pragma once
#include "gemcutter/Application/Delegate.h"
#include "gemcutter/Entity/Entity.h"
#include "gemcutter/Math/Vector.h"
#include "gemcutter/Physics/Physics.h"

class b2Body;
class b2Fixture;
struct b2JointEdge;
struct b2ContactEdge;

namespace gem
{
	class RigidBody2d : public Component<RigidBody2d>
	{
		friend class PhysicsWorldSingleton;
	public:
		RigidBody2d(Entity& owner, RigidBodyType type);
		~RigidBody2d();

		void SetBox();
		void SetBox(float width, float height);

		void SetSphere();
		void SetSphere(float radius);

		void SetPolygon(const vec2* vertices, unsigned count);

		void SetVelocity(const vec2& v);
		vec2 GetVelocity() const;

		void SetAngularVelocity(float angularVelocity);
		float GetAngularVelocity() const;

		void ApplyForce(const vec2& force);
		void ApplyForce(const vec2& force, const vec2& worldPoint);

		void ApplyImpulse(const vec2& impulse);
		void ApplyImpulse(const vec2& impulse, const vec2& worldPoint);

		void ApplyTorque(float torque);
		void ApplyAngularImpulse(float impulse);

		void SetDamping(float linearDamping);
		float GetDamping() const;

		void SetAngularDamping(float angularDamping);
		float GetAngularDamping() const;

		void SetFriction(float friction);
		float GetFriction() const;

		void SetGravityScale(float scale);
		float GetGravityScale() const;

		void SetType(RigidBodyType type);
		RigidBodyType GetType() const;

		void SetBullet(bool bullet);
		bool IsBullet() const;

		void SetFixedRotation(bool fixedRotation);
		bool IsFixedRotation() const;

		float GetMass() const;
		bool IsAwake() const;

		const b2Fixture* GetFixtures() const;
		const b2JointEdge* GetJoints() const;
		const b2ContactEdge* GetContacts() const;

		Dispatcher<void(Entity&)> onCollisionStart;
		Dispatcher<void(Entity&)> onCollisionEnd;

	private:
		void OnDisable() override;
		void OnEnable() override;

		b2Body* body;
		RigidBodyType type;
		float friction = 0.2f;
	};
}
