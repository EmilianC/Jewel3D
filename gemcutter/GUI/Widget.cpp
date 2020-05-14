// Copyright (c) 2021 Emilian Cioca
#include "Widget.h"
#include "gemcutter/Entity/Hierarchy.h"

#ifdef _DEBUG
#include "gemcutter/Rendering/Primitives.h"
#include "gemcutter/Rendering/Rendering.h"
#include "gemcutter/Utilities/Random.h"
#endif

namespace gem
{
	Widget::Widget(Entity& _owner)
		: Component(_owner)
	{
		owner.Require<Hierarchy>().propagateTransform = false;

#ifdef _DEBUG
		// Normalize to keep the intensity is reliable.
		debugColor = Normalize(RandomColor());
#endif
	}

	void Widget::SetTransform(vec2 position, vec2 size)
	{
		const float halfWidth  = size.x * 0.5f;
		const float halfHeight = size.y * 0.5f;

		top.offset    = halfHeight;
		bottom.offset = -halfHeight;

		left.offset  = -halfWidth;
		right.offset = halfWidth;

		//...

		UpdateContent();
	}

	void Widget::FitToParent()
	{
		top.anchor = 0.0f;
		top.offset = 0.0f;

		bottom.anchor = 0.0f;
		bottom.offset = 0.0f;

		left.anchor = 0.0f;
		left.offset = 0.0f;

		right.anchor = 0.0f;
		right.offset = 0.0f;
	}

	Entity::Ptr Widget::CreateChildWidget()
	{
		auto entity = owner.Get<Hierarchy>().CreateChild();
		entity->Add<Widget>();

		return entity;
	}

	void Widget::UpdateAll()
	{
		for (Entity& entity : With<Widget, HierarchyRoot>())
		{
			auto& widget = entity.Get<Widget>();
			widget.absoluteBounds.width = widget.right.offset;
			widget.absoluteBounds.height = widget.top.offset;

			for (Entity::Ptr& child : entity.Get<Hierarchy>().GetChildren())
			{
				if (auto* childWidget = child->Try<Widget>())
				{
					childWidget->UpdateBounds(widget);
				}
			}
		}
	}

#ifdef _DEBUG
	void Widget::DrawOutline() const
	{
		// Will have to make sure a camera is bound.

		vec3 bottomLeft{ absoluteBounds.x, absoluteBounds.y, 0.0f };
		vec3 topLeft{ absoluteBounds.x, absoluteBounds.y + absoluteBounds.height, 0.0f };
		vec3 topRight{ absoluteBounds.x + absoluteBounds.width, absoluteBounds.y + absoluteBounds.height, 0.0f };
		vec3 bottomRight{ absoluteBounds.x + absoluteBounds.width, absoluteBounds.y, 0.0f };

		Primitives.DrawRectangle(bottomLeft, topLeft, topRight, bottomRight, vec4(debugColor, 1.0f));

		// Draw anchors:
		SetBlendFunc(BlendFunc::Linear);
		//Primitives.DrawLine(, , vec4(debugColor, 0.5f));
		//Primitives.DrawLine(, , vec4(debugColor, 0.5f));
		//Primitives.DrawLine(, , vec4(debugColor, 0.5f));
		//Primitives.DrawLine( , vec4(debugColor, 0.5f));
	}
#endif

	void Widget::UpdateBounds(const Widget& parent)
	{
		const float offsetFromTop    = parent.absoluteBounds.height * top.anchor + top.offset;
		const float offsetFromBottom = parent.absoluteBounds.height * bottom.anchor + bottom.offset;
		const float offsetFromLeft   = parent.absoluteBounds.width * left.anchor + left.offset;
		const float offsetFromRight  = parent.absoluteBounds.width * right.anchor + right.offset;

		const float topEdge    = (parent.absoluteBounds.y + parent.absoluteBounds.height) - offsetFromTop;
		const float bottomEdge = parent.absoluteBounds.y + offsetFromBottom;
		const float leftEdge   = parent.absoluteBounds.x + offsetFromLeft;
		const float rightEdge  = (parent.absoluteBounds.x + parent.absoluteBounds.width) - offsetFromRight;

		absoluteBounds.x = leftEdge;
		absoluteBounds.y = bottomEdge;
		absoluteBounds.width = rightEdge - leftEdge;
		absoluteBounds.height = topEdge - bottomEdge;

		// The actual position of the entity is the pivot (center of the widget).
		owner.position.x = absoluteBounds.x + absoluteBounds.width * 0.5f;
		owner.position.y = absoluteBounds.y + absoluteBounds.height * 0.5f;

		UpdateContent();

		for (Entity::Ptr& child : owner.Get<Hierarchy>().GetChildren())
		{
			if (auto* childWidget = child->Try<Widget>())
			{
				childWidget->UpdateBounds(*this);
			}
		}
	}
}
