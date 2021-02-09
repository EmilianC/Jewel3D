// Copyright (c) 2017 Emilian Cioca
#include "RenderPass.h"
#include "gemcutter/Application/Application.h"
#include "gemcutter/Application/Logging.h"
#include "gemcutter/Entity/Entity.h"
#include "gemcutter/Entity/Hierarchy.h"
#include "gemcutter/Math/Transform.h"
#include "gemcutter/Rendering/Camera.h"
#include "gemcutter/Rendering/Primitives.h"
#include "gemcutter/Rendering/Rendering.h"
#include "gemcutter/Rendering/RenderTarget.h"
#include "gemcutter/Rendering/Viewport.h"
#include "gemcutter/Resource/Font.h"
#include "gemcutter/Resource/Material.h"
#include "gemcutter/Resource/Shader.h"
#include "gemcutter/Resource/Texture.h"
#include "gemcutter/Resource/UniformBuffer.h"
#include "gemcutter/Resource/VertexArray.h"
// Renderables
#include "gemcutter/Rendering/ParticleEmitter.h"
#include "gemcutter/Rendering/Text.h"

#include <glew/glew.h>

namespace
{
	void BindRenderable(const gem::Renderable& renderable, gem::Shader* overrideShader)
	{
		const gem::Material* material = renderable.GetMaterial().get();
		ASSERT(material, "Renderable Entity does not have a valid Material to render with.");

		if (overrideShader)
		{
			overrideShader->Bind();
		}
		else
		{
			ASSERT(material->shader, "Renderable Entity does not have a Shader and the RenderPass does not have an override attached.");

			material->shader->Bind(renderable.variants);
		}

		if (renderable.array)
		{
			renderable.array->Bind();
		}

		renderable.buffers.Bind();
		material->textures.Bind();
		gem::SetBlendFunc(material->blendMode);
		gem::SetDepthFunc(material->depthMode);
		gem::SetCullFunc(material->cullMode);
	}

	void UnBindRenderable(const gem::Renderable& renderable, gem::Shader* overrideShader)
	{
		const gem::Material& material = *renderable.GetMaterial();

		if (overrideShader)
		{
			overrideShader->UnBind();
		}
		else
		{
			material.shader->UnBind();
		}

		if (renderable.array)
		{
			renderable.array->UnBind();
		}

		renderable.buffers.UnBind();
		material.textures.UnBind();
	}
}

namespace gem
{
	RenderPass::RenderPass()
	{
		CreateUniformBuffer();
	}

	RenderPass::RenderPass(const RenderPass& other)
	{
		CreateUniformBuffer();
		*this = other;
	}

	RenderPass& RenderPass::operator=(const RenderPass& other)
	{
		viewport = other.viewport;
		camera = other.camera;
		target = other.target;
		shader = other.shader;
		skybox = other.skybox;

		return *this;
	}

	void RenderPass::SetCamera(Entity::Ptr _camera)
	{
		ASSERT(!_camera || _camera->Has<Camera>(), "'camera' must have a camera component.");

		camera = std::move(_camera);
	}

	void RenderPass::SetShader(Shader::Ptr _shader)
	{
		shader = std::move(_shader);
	}

	void RenderPass::SetTarget(RenderTarget::Ptr _target)
	{
		target = std::move(_target);
	}

	void RenderPass::SetViewport(std::optional<Viewport> vp)
	{
		viewport = vp;
	}

	void RenderPass::SetSkybox(Texture::Ptr sky)
	{
		ASSERT(!sky || sky->IsCubeMap(), "'sky' must be a cubemap texture.");

		skybox = std::move(sky);
	}

	void RenderPass::Bind()
	{
		if (target)
		{
			target->Bind();
		}

		if (viewport)
		{
			viewport->bind();
		}
		else if (target)
		{
			target->GetViewport().bind();
		}
		else
		{
			Application.GetScreenViewport().bind();
		}

		if (camera)
		{
			camera->Get<Camera>().Bind();
		}

		textures.Bind();
		buffers.Bind();
	}

	void RenderPass::UnBind()
	{
		glBindVertexArray(GL_NONE);

		// UnBind override shader.
		if (shader)
		{
			shader->UnBind();
		}

		textures.UnBind();
		buffers.UnBind();

		if (target)
		{
			RenderTarget::UnBind();
		}

		if (camera != nullptr)
		{
			Camera::UnBind();
		}
	}

	void RenderPass::PostProcess()
	{
		ASSERT(shader != nullptr, "RenderPass must have a Shader attached if it is a post-processing pass.");
		ASSERT(Primitives.IsLoaded(), "Primitives system must be initialized in order to render a post-processing pass.");

		Bind();

		SetBlendFunc(BlendFunc::None);
		SetDepthFunc(DepthFunc::None);

		MVP.Set(mat4::Identity);
		modelView.Set(mat4::Identity);
		model.Set(mat4::Identity);
		invModel.Set(mat4::Identity);
		normalMatrix.Set(mat3::Identity);
		transformBuffer.Bind(static_cast<unsigned>(UniformBufferSlot::Model));

		Primitives.DrawFullScreenQuad(*shader);

		if (skybox)
		{
			Primitives.DrawSkyBox(*skybox);
		}

		UnBind();
	}

	void RenderPass::Render(const Entity& root)
	{
		Bind();

		RenderEntityRecursive(root);

		if (skybox)
		{
			Primitives.DrawSkyBox(*skybox);
		}

		UnBind();
	}

	void RenderPass::Render(const std::vector<Entity::Ptr>& entities)
	{
		Bind();

		for (auto& entity : entities)
		{
			ASSERT(entity, "Entity pointer cannot be null.");
			RenderEntity(*entity);
		}

		if (skybox)
		{
			Primitives.DrawSkyBox(*skybox);
		}

		UnBind();
	}

	void RenderPass::RenderInstanced(const Entity& instance, unsigned count)
	{
		if (!instance.IsEnabled() || count == 0)
		{
			return;
		}

		auto* renderable = instance.Try<Renderable>();
		if (!renderable || !renderable->IsEnabled())
		{
			return;
		}

		Bind();
		BindRenderable(*renderable, shader.get());

		// Update transform uniforms.
		MVP.Set(mat4::Identity);
		modelView.Set(mat4::Identity);
		model.Set(mat4::Identity);
		invModel.Set(mat4::Identity);
		normalMatrix.Set(mat3::Identity);
		transformBuffer.Bind(static_cast<unsigned>(UniformBufferSlot::Model));

		glDrawArraysInstanced(GL_TRIANGLES, 0, renderable->array->GetVertexCount(), count);

		UnBindRenderable(*renderable, shader.get());

		if (skybox)
		{
			Primitives.DrawSkyBox(*skybox);
		}

		UnBind();
	}

	void RenderPass::RenderEntity(const Entity& ent)
	{
		if (!ent.IsEnabled())
		{
			return;
		}

		auto* renderable = ent.Try<Renderable>();
		if (!renderable || !renderable->IsEnabled())
		{
			return;
		}

		BindRenderable(*renderable, shader.get());

		// Update transform uniforms.
		const mat4 worldTransform = ent.GetWorldTransform();

		if (camera)
		{
			auto& cameraComponent = camera->Get<Camera>();

			const mat4 mv = cameraComponent.GetViewMatrix() * worldTransform;
			const mat4 mvp = cameraComponent.GetProjMatrix() * mv;

			MVP.Set(mvp);
			modelView.Set(mv);
		}
		else
		{
			MVP.Set(mat4::Identity);
			modelView.Set(mat4::Identity);
		}

		model.Set(worldTransform);
		invModel.Set(worldTransform.GetFastInverse());
		normalMatrix.Set(mat3(worldTransform).GetInverse().GetTranspose());

		transformBuffer.Bind(static_cast<unsigned>(UniformBufferSlot::Model));

		if (auto* text = dynamic_cast<const Text*>(renderable))
		{
			auto& font = text->font;
			ASSERT(font != nullptr, "Entity has a Text component but does not have a Font to render with.");

			auto* dimensions = font->GetDimensions();
			auto* positions = font->GetPositions();
			auto* advances = font->GetAdvances();
			auto* masks = font->GetMasks();

			// We have to send the vertices of each character we render. We'll store them here.
			float points[18] =
			{
				0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f,

				0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f,
			};

			const vec3 advanceDirection = worldTransform.GetRight();
			const vec3 upDirection = worldTransform.GetUp();
			const vec3 initialPosition = ent.position;
			vec3 linePosition = initialPosition;
			unsigned currentLine = 1;

			if (text->centeredX)
			{
				text->owner.position -= advanceDirection * (((text->GetLineWidth(currentLine) + text->kernel * text->text.size())) / 2.0f);
			}

			if (text->centeredY)
			{
				text->owner.position -= upDirection * ((font->GetStringHeight() * static_cast<float>(text->GetNumLines())) / 2.0f);
			}

			glBindVertexArray(Font::GetVAO());
			glBindBuffer(GL_ARRAY_BUFFER, Font::GetVBO());
			font->GetTexture()->Bind(0);

			for (unsigned i = 0; i < text->text.size(); ++i)
			{
				char character = text->text[i];
				unsigned charIndex = static_cast<unsigned>(character) - '!';

				// Handle whitespace.
				if (character == ' ')
				{
					text->owner.position += advanceDirection * (font->GetStringWidth("Z") + text->kernel);
					continue;
				}
				else if (character == '\n')
				{
					linePosition += -upDirection * static_cast<float>(font->GetStringHeight()) * 1.33f;
					text->owner.position = linePosition;
					currentLine++;

					if (text->centeredX)
					{
						text->owner.position -= advanceDirection * (((text->GetLineWidth(currentLine) + text->kernel * text->text.size())) / 2.0f);
					}

					continue;
				}
				else if (character == '\t')
				{
					text->owner.position += advanceDirection * (font->GetStringWidth("Z") + text->kernel) * 4;
					continue;
				}

				if (!masks[charIndex])
				{
					// Character does not exist in this font. Advance to next character.
					text->owner.position += advanceDirection * ((advances[charIndex].x + text->kernel));
					continue;
				}

				/* Adjusts the node's position based on the character. */
				vec3 characterPosition;
				characterPosition += advanceDirection * static_cast<float>(positions[charIndex].x);
				characterPosition += upDirection * static_cast<float>(positions[charIndex].y);
				text->owner.position += characterPosition;

				const mat4 newTransform = ent.GetWorldTransform();

				/* Construct a polygon based on the current character's dimensions. */
				points[3] = static_cast<float>(dimensions[charIndex].x);
				points[7] = static_cast<float>(dimensions[charIndex].y);
				points[9] = static_cast<float>(dimensions[charIndex].x);
				points[12] = static_cast<float>(dimensions[charIndex].x);
				points[13] = static_cast<float>(dimensions[charIndex].y);
				points[16] = static_cast<float>(dimensions[charIndex].y);

				/* Update buffers with the new polygon. */
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 18, points);

				/* Render */
				if (camera)
				{
					auto& cameraComponent = camera->Get<Camera>();

					MVP.Set(cameraComponent.GetViewProjMatrix() * newTransform);
					modelView.Set(cameraComponent.GetViewMatrix() * newTransform);
				}
				else
				{
					MVP.Set(mat4::Identity);
					modelView.Set(mat4::Identity);
				}
				model.Set(newTransform);
				invModel.Set(newTransform.GetFastInverse());
				transformBuffer.Bind(static_cast<unsigned>(UniformBufferSlot::Model));

				glDrawArrays(GL_TRIANGLES, 0, 6);

				/* Adjust position for the next node. */
				// Undo character translate.
				text->owner.position -= characterPosition;
				// Advance to next character.
				text->owner.position += advanceDirection * ((advances[charIndex].x + text->kernel));
			}

			glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

			text->owner.position = initialPosition;
		}
		else if (auto* emitter = dynamic_cast<const ParticleEmitter*>(renderable))
		{
			if (emitter->GetNumAliveParticles() > 0)
			{
				emitter->GetBuffer().Bind(static_cast<unsigned>(UniformBufferSlot::Particle));

				glBindVertexArray(emitter->GetVAO());
				glDrawArrays(GL_POINTS, 0, emitter->GetNumAliveParticles());
			}
		}
		else
		{
			auto* vertexArray = renderable->array.get();
			ASSERT(vertexArray, "Renderable Entity does not have a valid VertexArray to render.");

			vertexArray->Draw();
		}

		UnBindRenderable(*renderable, shader.get());
	}

	void RenderPass::RenderEntityRecursive(const Entity& ent)
	{
		RenderEntity(ent);

		if (auto* hierarchy = ent.Try<Hierarchy>())
		{
			for (auto& child : hierarchy->GetChildren())
			{
				RenderEntityRecursive(*child);
			}
		}
	}

	void RenderPass::CreateUniformBuffer()
	{
		MVP = transformBuffer.AddUniform<mat4>("MVP");
		modelView = transformBuffer.AddUniform<mat4>("ModelView");
		model = transformBuffer.AddUniform<mat4>("Model");
		invModel = transformBuffer.AddUniform<mat4>("InvModel");
		normalMatrix = transformBuffer.AddUniform<mat3>("Normal");

		transformBuffer.InitBuffer(VertexBufferUsage::Dynamic);
	}
}
