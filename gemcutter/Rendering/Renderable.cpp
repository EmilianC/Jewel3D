// Copyright (c) 2020 Emilian Cioca
#include "Renderable.h"

namespace gem
{
	Renderable::Renderable(Entity& _owner)
		: Component(_owner)
	{
		material = Material::MakeNew();
		material->shader = Shader::MakeNewPassThrough();
	}

	Renderable::Renderable(Entity& _owner, VertexArray::Ptr _array)
		: Component(_owner)
		, array(std::move(_array))
	{
	}

	Renderable::Renderable(Entity& _owner, Material::Ptr _material)
		: Component(_owner)
	{
		SetMaterial(std::move(_material));
	}

	Renderable::Renderable(Entity& _owner, VertexArray::Ptr _array, Material::Ptr _material)
		: Component(_owner)
		, array(std::move(_array))
	{
		SetMaterial(std::move(_material));
	}

	void Renderable::SetMaterial(Material::Ptr newMaterial)
	{
		ASSERT(newMaterial, "'newMaterial' cannot be null.");

		// Create any instance buffer copies needed per-entity.
		for (const BufferBinding& binding : newMaterial->shader->GetBufferBindings())
		{
			if (binding.templateBuff)
			{
				auto newBuffer = UniformBuffer::MakeNew();
				newBuffer->Copy(*binding.templateBuff);

				buffers.Add(std::move(newBuffer), binding.unit);
			}
		}

		material = std::move(newMaterial);
	}

	const Material::Ptr& Renderable::GetMaterial() const
	{
		return material;
	}

	Material::Ptr& Renderable::GetMaterial()
	{
		return material;
	}
}
