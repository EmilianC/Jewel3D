#include "RigidBody.h"

#include "gemcutter/Math/Math.h"
#include "gemcutter/Physics/2D/PhysicsWorld.h"

namespace gem
{
	RigidBody2d::RigidBody2d(Entity& _owner, RigidBodyType _type)
		: Component<RigidBody2d>(_owner)
		, type(_type)
	{
		b2BodyDef bodyDefinition;
		bodyDefinition.position.x = owner.position.x;
		bodyDefinition.position.y = owner.position.y;
		bodyDefinition.angle = ToRadian(owner.rotation.GetEuler().z);
		bodyDefinition.userData.pointer = reinterpret_cast<uintptr_t>(this);

		switch (type)
		{
		case RigidBodyType::Dynamic:
			bodyDefinition.type = b2BodyType::b2_dynamicBody;
			owner.Tag<DynamicRigidBodyTag>();
			break;

		case RigidBodyType::Kinematic:
			bodyDefinition.type = b2BodyType::b2_kinematicBody;
			owner.Tag<KinematicRigidBodyTag>();
			break;

		case RigidBodyType::Static:
			bodyDefinition.type = b2BodyType::b2_staticBody;
			break;

		case RigidBodyType::Trigger:
			bodyDefinition.type = b2BodyType::b2_kinematicBody;
			owner.Tag<KinematicRigidBodyTag>();
			break;
		}

		body = PhysicsWorld.world.CreateBody(&bodyDefinition);
	}

	RigidBody2d::~RigidBody2d()
	{
		switch (type)
		{
		case RigidBodyType::Dynamic:
			owner.RemoveTag<DynamicRigidBodyTag>();
			break;

		case RigidBodyType::Kinematic:
		case RigidBodyType::Trigger:
			owner.RemoveTag<KinematicRigidBodyTag>();
			break;
		}

		PhysicsWorld.world.DestroyBody(body);
	}

	void RigidBody2d::SetBox()
	{
		SetBox(owner.scale.x, owner.scale.y);
	}

	void RigidBody2d::SetBox(float width, float height)
	{
		b2PolygonShape box;
		box.SetAsBox(width * 0.5f, height * 0.5f);

		b2Fixture* fixture = body->CreateFixture(&box, 1.0f);
		fixture->SetSensor(type == RigidBodyType::Trigger);
		fixture->SetFriction(friction);
		fixture->GetUserData().pointer = reinterpret_cast<uintptr_t>(this);
	}

	void RigidBody2d::SetSphere()
	{
		SetSphere(Length(vec2(owner.scale)) * 0.5f);
	}

	void RigidBody2d::SetSphere(float radius)
	{
		ASSERT(radius > 0.0f, "'radius' must be a positive non-zero number.");

		b2CircleShape sphere;
		sphere.m_radius = radius;

		b2Fixture* fixture = body->CreateFixture(&sphere, 1.0f);
		fixture->SetSensor(type == RigidBodyType::Trigger);
		fixture->SetFriction(friction);
		fixture->GetUserData().pointer = reinterpret_cast<uintptr_t>(this);
	}

	void RigidBody2d::SetPolygon(const vec2* vertices, unsigned count)
	{
		ASSERT(count >= 3, "Must provide at least three vertices.");
		ASSERT(count <= b2_maxPolygonVertices, "Must not execeed %d vertices.", b2_maxPolygonVertices);

		b2Vec2 polygon[b2_maxPolygonVertices];
		std::memcpy(polygon, vertices, count * sizeof(b2Vec2));

		b2PolygonShape shape;
		shape.Set(polygon, count);

		b2Fixture* fixture = body->CreateFixture(&shape, 1.0f);
		fixture->SetSensor(type == RigidBodyType::Trigger);
		fixture->SetFriction(friction);
		fixture->GetUserData().pointer = reinterpret_cast<uintptr_t>(this);
	}

	void RigidBody2d::SetVelocity(const vec2& v)
	{
		body->SetLinearVelocity({v.x, v.y});
	}

	vec2 RigidBody2d::GetVelocity() const
	{
		b2Vec2 v = body->GetLinearVelocity();
		return {v.x, v.y};
	}

	void RigidBody2d::SetAngularVelocity(float angularVelocity)
	{
		body->SetAngularVelocity(ToRadian(angularVelocity));
	}

	float RigidBody2d::GetAngularVelocity() const
	{
		return ToDegree(body->GetAngularVelocity());
	}

	void RigidBody2d::ApplyForce(const vec2& force)
	{
		body->ApplyForceToCenter({force.x, force.y}, true);
	}

	void RigidBody2d::ApplyForce(const vec2& force, const vec2& worldPoint)
	{
		body->ApplyForce({force.x, force.y}, {worldPoint.x, worldPoint.y}, true);
	}

	void RigidBody2d::ApplyImpulse(const vec2& impulse)
	{
		body->ApplyLinearImpulseToCenter({impulse.x, impulse.y}, true);
	}

	void RigidBody2d::ApplyImpulse(const vec2& impulse, const vec2& worldPoint)
	{
		body->ApplyLinearImpulse({impulse.x, impulse.y}, {worldPoint.x, worldPoint.y}, true);
	}

	void RigidBody2d::ApplyTorque(float torque)
	{
		body->ApplyTorque(torque, true);
	}

	void RigidBody2d::ApplyAngularImpulse(float impulse)
	{
		body->ApplyAngularImpulse(impulse, true);
	}

	void RigidBody2d::SetDamping(float linearDamping)
	{
		body->SetLinearDamping(linearDamping);
	}

	float RigidBody2d::GetDamping() const
	{
		return body->GetLinearDamping();
	}

	void RigidBody2d::SetAngularDamping(float angularDamping)
	{
		body->SetAngularDamping(angularDamping);
	}

	float RigidBody2d::GetAngularDamping() const
	{
		return body->GetAngularDamping();
	}

	void RigidBody2d::SetFriction(float _friction)
	{
		if (friction == _friction)
			return;

		friction = _friction;
		for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
		{
			fixture->SetFriction(friction);
		}
	}

	float RigidBody2d::GetFriction() const
	{
		return friction;
	}

	void RigidBody2d::SetGravityScale(float scale)
	{
		body->SetGravityScale(scale);
	}

	float RigidBody2d::GetGravityScale() const
	{
		return body->GetGravityScale();
	}

	void RigidBody2d::SetType(RigidBodyType _type)
	{
		if (type == _type)
		{
			return;
		}

		switch (type)
		{
		case RigidBodyType::Dynamic:
			owner.RemoveTag<DynamicRigidBodyTag>();
			break;

		case RigidBodyType::Kinematic:
		case RigidBodyType::Trigger:
			owner.RemoveTag<KinematicRigidBodyTag>();
			break;
		}

		switch (_type)
		{
		case RigidBodyType::Dynamic:
			body->SetType(b2BodyType::b2_dynamicBody);
			owner.Tag<DynamicRigidBodyTag>();
			break;

		case RigidBodyType::Kinematic:
			body->SetType(b2BodyType::b2_kinematicBody);
			owner.Tag<KinematicRigidBodyTag>();
			break;

		case RigidBodyType::Static:
			body->SetType(b2BodyType::b2_staticBody);
			break;

		case RigidBodyType::Trigger:
			body->SetType(b2BodyType::b2_staticBody);
			owner.Tag<KinematicRigidBodyTag>();
			break;
		}

		type = _type;
	}

	RigidBodyType RigidBody2d::GetType() const
	{
		return type;
	}

	void RigidBody2d::SetBullet(bool bullet)
	{
		body->SetBullet(bullet);
	}

	bool RigidBody2d::IsBullet() const
	{
		return body->IsBullet();
	}

	void RigidBody2d::SetFixedRotation(bool fixedRotation)
	{
		body->SetFixedRotation(fixedRotation);
	}

	bool RigidBody2d::IsFixedRotation() const
	{
		return body->IsFixedRotation();
	}

	float RigidBody2d::GetMass() const
	{
		return body->GetMass();
	}

	bool RigidBody2d::IsAwake() const
	{
		return body->IsAwake();
	}

	const b2Fixture* RigidBody2d::GetFixtures() const
	{
		return body->GetFixtureList();
	}

	const b2JointEdge* RigidBody2d::GetJoints() const
	{
		return body->GetJointList();
	}

	const b2ContactEdge* RigidBody2d::GetContacts() const
	{
		return body->GetContactList();
	}

	void RigidBody2d::OnDisable()
	{
		body->SetEnabled(false);
	}

	void RigidBody2d::OnEnable()
	{
		body->SetEnabled(true);
	}
}
