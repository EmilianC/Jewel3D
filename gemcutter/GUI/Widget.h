// Copyright (c) 2021 Emilian Cioca
#pragma once
#include "gemcutter/Entity/Entity.h"
#include "gemcutter/GUI/Rectangle.h"

namespace gem
{
	struct Edge
	{
		float anchor = 0.0f;
		float offset = 0.0f;
	};

	// A generic UI element sized with respect to its parent widget.
	class Widget : public Component<Widget>
	{
	public:
		Widget(Entity& owner);

		// Helper function which sets anchors to maintain the target size and position.
		// Position is in the space of the parent widget.
		void SetTransform(vec2 position, vec2 size);

		// Sets the anchors to the full space available in the parent widget.
		// This is the default behaviour.
		void FitToParent();

		Entity::Ptr CreateChildWidget();

		const Rectangle& GetAbsoluteBounds() const { return absoluteBounds; }

		static void UpdateAll();

		// Called when the bounds of the widget are updated.
		// This is where specific widgets can adjust their content.
		virtual void UpdateContent() {}

#ifdef _DEBUG
		void DrawOutline() const;
		vec3 debugColor;
#endif

		// Clips the rendering of child widgets within the bounds of this widget.
		bool clipChildren = true;

		Edge top;
		Edge bottom;
		Edge left;
		Edge right;

	private:
		void UpdateBounds(const Widget& parent);

		// World-space demensions of the widget
		Rectangle absoluteBounds;
	};
}
