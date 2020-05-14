// Copyright (c) 2021 Emilian Cioca
#include "Screen.h"
#include "gemcutter/Application/HierarchicalEvent.h"
#include "gemcutter/Entity/Hierarchy.h"
#include "gemcutter/Input/Input.h"

namespace gem
{
	Screen::Screen(Entity& _owner)
		: Widget(_owner)
	{
		owner.Require<Hierarchy>();
		ASSERT(owner.Get<Hierarchy>().IsRoot(), "A Screen widget must be a root.");

		owner.Require<HierarchicalDispatcher<MouseMoved>>();
		owner.Require<HierarchicalDispatcher<MouseScrolled>>();
		owner.Require<HierarchicalDispatcher<KeyPressed>>();
		owner.Require<HierarchicalDispatcher<KeyReleased>>();

		top.offset   = static_cast<float>(Application.GetScreenHeight());
		right.offset = static_cast<float>(Application.GetScreenWidth());

		onResize = [this](const Resize& e) {
			top.offset   = static_cast<float>(e.height);
			right.offset = static_cast<float>(e.width);
		};
	}
}
