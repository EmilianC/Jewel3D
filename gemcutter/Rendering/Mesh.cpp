// Copyright (c) 2017 Emilian Cioca
#include "Mesh.h"

namespace gem
{
	Mesh::Mesh(Entity& _owner)
		: Renderable(_owner)
	{
	}

	Mesh::Mesh(Entity& _owner, VertexArray::Ptr _array)
		: Renderable(_owner, std::move(_array))
	{
	}

	Mesh::Mesh(Entity& _owner, Material::Ptr material)
		: Renderable(_owner, std::move(material))
	{
	}

	Mesh::Mesh(Entity& _owner, VertexArray::Ptr _array, Material::Ptr material)
		: Renderable(_owner, std::move(_array), std::move(material))
	{
	}
}
