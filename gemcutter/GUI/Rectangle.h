// Copyright (c) 2021 Emilian Cioca
#pragma once

namespace gem
{
	struct Rectangle
	{
		Rectangle() = default;
		Rectangle(float x, float y, float width, float height);

		float GetAspectRatio() const;
		bool IsPointInside(float x, float y) const;

		float x = 0.0f;
		float y = 0.0f;
		float width = 1.0f;
		float height = 1.0f;
	};
}
