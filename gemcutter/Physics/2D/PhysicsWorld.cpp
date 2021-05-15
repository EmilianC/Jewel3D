#include "PhysicsWorld.h"

#include "gemcutter/Application/Logging.h"
#include "gemcutter/Math/Math.h"
#include "gemcutter/Physics/2D/RigidBody.h"

#ifdef _DEBUG
#include "gemcutter/Physics/2D/PhysicsDebug.h"
#endif

namespace gem
{
	PhysicsWorldSingleton PhysicsWorld({});

	PhysicsWorldSingleton::PhysicsWorldSingleton(const PhysicsParams& worldParams)
		: world({worldParams.gravity.x, worldParams.gravity.y})
		, params(worldParams)
	{
	}

	PhysicsWorldSingleton::~PhysicsWorldSingleton()
	{
		SetDebugEnabled(false);
	}

	void PhysicsWorldSingleton::SetGravity(vec2 gravity)
	{
		params.gravity = gravity;
		world.SetGravity({gravity.x, gravity.y});
	}

	void PhysicsWorldSingleton::SetVelocityIterations(int iterations)
	{
		ASSERT(iterations > 0, "Iterations must be greater than zero.");
		params.m_velocityIterations = iterations;
	}

	void PhysicsWorldSingleton::SetPositionIterations(int iterations)
	{
		ASSERT(iterations > 0, "Iterations must be greater than zero.");
		params.m_positionIterations = iterations;
	}

	vec2 PhysicsWorldSingleton::GetGravity() const
	{
		return params.gravity;
	}

	int PhysicsWorldSingleton::GetVelocityIterations() const
	{
		return params.m_velocityIterations;
	}

	int PhysicsWorldSingleton::GetPositionIterations() const
	{
		return params.m_positionIterations;
	}

	void PhysicsWorldSingleton::Update(float deltaTime)
	{
		for (Entity& entity : With<KinematicRigidBodyTag>())
		{
			auto& rb = entity.Get<RigidBody2d>();

			b2Vec2 pos{ entity.position.x, entity.position.y };
			float angle = ToRadian(entity.rotation.GetEuler().z);

			rb.body->SetTransform(pos, angle);
		}

		world.Step(deltaTime, params.m_velocityIterations, params.m_positionIterations);

		for (Entity& entity : With<DynamicRigidBodyTag>())
		{
			auto& rb = entity.Get<RigidBody2d>();

			b2Vec2 pos = rb.body->GetPosition();
			float angle = ToDegree(rb.body->GetAngle());

			entity.position.x = pos.x;
			entity.position.y = pos.y;
			entity.rotation.FromEuler({0.0f, 0.0f, angle});
		}

	#ifdef _DEBUG
		if (debugRenderer)
		{
			debugRenderer->Clear();
			world.DebugDraw();

			for (b2Contact* contact = world.GetContactList(); contact; contact = contact->GetNext())
			{
				if (!contact->IsEnabled() || !contact->IsTouching())
				{
					continue;
				}

				b2WorldManifold manifold;
				contact->GetWorldManifold(&manifold);

				for (int i = 0; i < contact->GetManifold()->pointCount; ++i)
				{
					debugRenderer->DrawPoint(manifold.points[i], 0.1f, b2Color(1.0f, 0.0f, 0.0f));
					debugRenderer->DrawSegment(
						manifold.points[i],
						{manifold.points[i].x + manifold.normal.x * 0.25f, manifold.points[i].y + manifold.normal.y * 0.25f},
						b2Color(0.0f, 0.0f, 1.0f)
					);
				}
			}
		}
	#endif
	}

	RayCastResult PhysicsWorldSingleton::RayCast(vec2 start, vec2 end, EnumFlags<RigidBodyType> mask) const
	{
		RayCastResult result;

		struct CastHelper : public b2RayCastCallback
		{
			CastHelper(RayCastResult& r, EnumFlags<RigidBodyType> m) : result(r), mask(m) {}

			float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
			{
				if (fixture->IsSensor() && !mask.Has(RigidBodyType::Trigger))
				{
					return -1.0f;
				}

				const b2BodyType type = fixture->GetBody()->GetType();
				if (type == b2BodyType::b2_dynamicBody   && !mask.Has(RigidBodyType::Dynamic) ||
					type == b2BodyType::b2_kinematicBody && !mask.Has(RigidBodyType::Kinematic) ||
					type == b2BodyType::b2_staticBody    && !mask.Has(RigidBodyType::Static))
				{
					result.isValid = false;
					return fraction;
				}

				result.isValid = true;
				result.fixture = fixture;
				result.point   = {point.x, point.y};
				result.normal  = {normal.x, normal.y};

				return fraction;
			}

			RayCastResult& result;
			EnumFlags<RigidBodyType> mask;
		} castHelper { result, mask };

		world.RayCast(&castHelper, {start.x, start.y}, {end.x, end.y});
		DrawRayDebug(start, end, castHelper.result);
		FinalizeResult(start, castHelper.result);

		return result;
	}

	void PhysicsWorldSingleton::SetDebugEnabled(bool enabled)
	{
	#ifdef _DEBUG
		if (enabled)
		{
			if (!debugRenderer)
			{
				debugRenderer = new PhysicsDebug();
				if (!debugRenderer->Init())
				{
					ASSERT(false, "Failed to initialize physics debug renderer.");

					delete debugRenderer;
					debugRenderer = nullptr;
				}
			}
		}
		else if (debugRenderer)
		{
			delete debugRenderer;
			debugRenderer = nullptr;
		}

		world.SetDebugDraw(debugRenderer);
	#endif
	}

	bool PhysicsWorldSingleton::GetDebugEnabled() const
	{
	#ifdef _DEBUG
		return !!debugRenderer;
	#else
		return false;
	#endif
	}

	void PhysicsWorldSingleton::DrawDebug()
	{
	#ifdef _DEBUG
		if (debugRenderer)
		{
			debugRenderer->Render();
		}
	#endif
	}

	void PhysicsWorldSingleton::FinalizeResult(vec2 rayStart, RayCastResult& hitResult) const
	{
		if (hitResult.isValid)
		{
			b2Body* body = hitResult.fixture->GetBody();
			hitResult.component = reinterpret_cast<RigidBody2d*>(body->GetUserData().pointer);
			hitResult.distance  = Distance(rayStart, hitResult.point);
		}
	}

	void PhysicsWorldSingleton::DrawRayDebug(vec2 start, vec2 end, const RayCastResult& hitResult) const
	{
	#ifdef _DEBUG
		if (!debugRenderer)
		{
			return;
		}

		if (hitResult.isValid)
		{
			const b2Vec2 hit    = {hitResult.point.x,  hitResult.point.y};
			const b2Vec2 normal = {hitResult.normal.x, hitResult.normal.y};
			const float normalSize = 0.4f;

			debugRenderer->DrawPoint(hit, 0.1f, b2Color(1.0f, 0.0f, 0.0f));
			debugRenderer->DrawSegment({start.x, start.y}, hit, b2Color(1.0f, 0.0f, 0.0f));
			debugRenderer->DrawSegment(hit, {hit.x + normal.x * normalSize, hit.y + normal.y * normalSize}, b2Color(0.0f, 0.0f, 1.0f));
		}
		else
		{
			debugRenderer->DrawSegment({start.x, start.y}, {end.x, end.y}, b2Color(0.0f, 1.0f, 0.0f));
		}
	#endif
	}
}
