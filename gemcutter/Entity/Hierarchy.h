// Copyright (c) 2020 Emilian Cioca
#pragma once
#include "gemcutter/Entity/Entity.h"
#include "gemcutter/Math/Matrix.h"
#include "gemcutter/Math/Quaternion.h"

#include <vector>

namespace gem
{
	// A Tag identifying an Entity with no parents.
	struct HierarchyRoot : public Tag<HierarchyRoot> {};

	// Allows Entities to be organized in a tree structure.
	// Can also propagate transformations from parent to child.
	class Hierarchy : public Component<Hierarchy>
	{
	public:
		Hierarchy(Entity& owner);
		~Hierarchy();

		// Attaches a child.
		void AddChild(Entity::Ptr entity);

		// Detaches the given child.
		void RemoveChild(Entity& entity);

		// Returns true if the specified Entity is a direct child of this one.
		bool IsChild(const Entity& entity) const;

		// Detaches all children.
		void ClearChildren();

		// Detaches from the parent, if we have one.
		void DetachFromParent();

		// Finds the root of the hierarchy, which could be this Entity.
		Entity::ConstPtr GetRoot() const;

		// Finds the root of the hierarchy, which could be this Entity.
		Entity::Ptr GetRoot();

		// Returns the parent we are attached to, if we have one.
		Entity::Ptr GetParent() const;

		// Returns the number of children currently held by this Entity.
		unsigned GetNumChildren() const;

		// Gets the list of children held by this Entity.
		const auto& GetChildren() const { return children; }
		auto& GetChildren() { return children; }

		// Gets the depth of this Entity in the hierarchy. The root is always depth 0.
		// Direct children of the root are at depth 1, and so on.
		unsigned GetDepth() const;

		// Returns true if this Entity doesn't have a parent.
		bool IsRoot() const;

		// Returns true if this Entity doesn't have any children.
		bool IsLeaf() const;

		// Creates and returns a new child Entity.
		Entity::Ptr CreateChild();

		// Returns the world-space transformation of the Entity, accumulated from the root of the hierarchy.
		mat4 GetWorldTransform() const;

		// Returns the world-space rotation of the Entity, accumulated from the root of the hierarchy.
		quat GetWorldRotation() const;

		// Whether or not this Entity propagates its transformations downwards through the hierarchy.
		bool propagateTransform = true;

	private:
		Hierarchy* parentHierarchy = nullptr;
		Entity::WeakPtr parent;
		std::vector<Entity::Ptr> children;
	};
}
