namespace gem
{
	template<typename Functor>
	RayCastResult PhysicsWorldSingleton::RayCast(vec2 start, vec2 end, Functor&& func) const
	{
		RayCastResult result;

		struct CastHelper : public b2RayCastCallback
		{
			CastHelper(Functor& f, RayCastResult& r) : func(f), result(r) {}

			float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
			{
				if (!func(fixture, vec2(point.x, point.y), vec2(normal.x, normal.y)))
				{
					result.isValid = false;
					return -1.0f;
				}

				result.isValid = true;
				result.fixture = fixture;
				result.point   = {point.x, point.y};
				result.normal  = {normal.x, normal.y};

				return fraction;
			}

			Functor& func;
			RayCastResult& result;
		} castHelper { func, result };

		world.RayCast(&castHelper, {start.x, start.y}, {end.x, end.y});
		DrawRayDebug(start, end, castHelper.result);
		FinalizeResult(start, castHelper.result);

		return result;
	}

	template<typename Functor>
	unsigned PhysicsWorldSingleton::QueryAABB(vec2 pos, vec2 size, Functor&& func) const
	{
		struct QueryHelper : public b2QueryCallback
		{
			QueryHelper(Functor& f) : func(f) {}

			bool ReportFixture(b2Fixture* fixture) override
			{
				if (!func(fixture))
					return false;

				++count;
				return true;
			}

			Functor& func;
			unsigned count = 0;
		} queryHelper { func };

		b2AABB volume;
		volume.lowerBound = { pos.x, pos.y };
		volume.upperBound.x = volume.lowerBound.x + size.x;
		volume.upperBound.y = volume.lowerBound.y + size.y;

		world.QueryAABB(&queryHelper, volume);

		return queryHelper.count;
	}
}
