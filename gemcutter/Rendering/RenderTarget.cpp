// Copyright (c) 2017 Emilian Cioca
#include "RenderTarget.h"
#include "gemcutter/Application/Application.h"
#include "gemcutter/Application/Logging.h"
#include "gemcutter/Math/Vector.h"
#include "gemcutter/Resource/Texture.h"
#include "gemcutter/Utilities/ScopeGuard.h"

#include <glew/glew.h>

namespace
{
	GLenum* drawBuffers = nullptr;
}

namespace gem
{
	RenderTarget::RenderTarget(unsigned _width, unsigned _height, unsigned _numColorTextures, bool hasDepth, unsigned _numSamples)
		: FBO(GL_NONE)
		, numColorTextures(_numColorTextures)
		, numSamples(_numSamples)
		, width(_width)
		, height(_height)
	{
		ASSERT(width != 0, "'width' must be greater than 0.");
		ASSERT(height != 0, "'height' must be greater than 0.");
		ASSERT(numSamples == 1 || numSamples == 2 || numSamples == 4 || numSamples == 8 || numSamples == 16,
			"'numSamples' must be 1, 2, 4, 8, or 16.");
		ASSERT(_numColorTextures > 0 || hasDepth, "A RenderTarget cannot be made without any textures.");

		// First time setup.
		if (!drawBuffers)
		{
			unsigned maxTextureAttachments = GPUInfo.GetMaxRenderTargetTextures();
			drawBuffers = static_cast<GLenum*>(malloc(sizeof(GLenum) * maxTextureAttachments));

			for (unsigned i = 0; i < maxTextureAttachments; ++i)
			{
				drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
			}
		}

		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		defer { glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE); };

		if (hasDepth)
		{
			depth = Texture::MakeNew();
			depth->CreateTexture(
				width, height,
				TextureFormat::DEPTH_24, TextureFilter::Point,
				TextureWrap::Clamp, 1.0f, numSamples);

			glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				GL_DEPTH_ATTACHMENT,
				IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
				depth->GetHandle(),
				0);
		}

		if (numColorTextures > 0)
		{
			if (numColorTextures > GPUInfo.GetMaxRenderTargetTextures() ||
				numColorTextures > GPUInfo.GetMaxDrawBuffers())
			{
				Error("RenderTarget: Number of color textures exceeds amount supported by the OpenGL drivers.");
				return;
			}

			colors = new Texture::Ptr[numColorTextures];
		}

		// Set any color textures as write-able targets.
		glDrawBuffers(numColorTextures, drawBuffers);
	}

	RenderTarget::~RenderTarget()
	{
		delete[] colors;

		glDeleteFramebuffers(1, &FBO);
	}

	RenderTarget::Ptr RenderTarget::MakeResolve() const
	{
		ASSERT(IsMultisampled(), "Must be multisampled to create a resolve RenderTarget.");

		// Mirror all textures, but with just one sample.
		RenderTarget::Ptr resolve = RenderTarget::MakeNew(width, height, numColorTextures, HasDepth(), 1);

		for (unsigned i = 0; i < numColorTextures; ++i)
		{
			resolve->InitTexture(i, colors[i]->GetFormat(), colors[i]->GetFilter());
		}

		if (!resolve->Validate())
		{
			resolve.reset();
		}

		return resolve;
	}

	void RenderTarget::InitTexture(unsigned index, TextureFormat format, TextureFilter filter)
	{
		ASSERT(numColorTextures > 0, "RenderTarget does not have any color textures to initialize.");
		ASSERT(index < numColorTextures, "'index' must specify a valid color texture.");
		ASSERT(format != TextureFormat::DEPTH_24, "'format' cannot be DEPTH_24 for a color texture.");
		ASSERT(!colors[index], "'index' is already initialized.");

		colors[index] = Texture::MakeNew();
		colors[index]->CreateTexture(width, height, format, filter, TextureWrap::Clamp, 1.0f, numSamples);

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0 + index,
			IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
			colors[index]->GetHandle(),
			0);

		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	bool RenderTarget::Validate() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		defer { glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE); };

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			Error("RenderTarget: Failed to validate with the provided parameters.");
			return false;
		}

		return true;
	}

	void RenderTarget::AttachDepthTexture(Texture::Ptr texture)
	{
		ASSERT(texture, "'texture' cannot be null.");
		ASSERT(texture->GetHandle() != GL_NONE, "'texture' must be initialized.");
		ASSERT(texture->GetNumSamples() == numSamples, "'texture' must have the same per-texel sample count as the RenderTarget.");
		ASSERT(texture->GetFormat() == TextureFormat::DEPTH_24, "Format of 'texture' must be a 24 bit depth texture.");
		ASSERT(texture->GetWidth() == width, "Dimensions of 'texture' must match the dimensions of the RenderTarget.");
		ASSERT(texture->GetHeight() == height, "Dimensions of 'texture' must match the dimensions of the RenderTarget.");
		ASSERT(!texture->IsCubeMap(), "'texture' cannot be a cubemap.");

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, texture->GetHandle(), 0);
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

		depth = std::move(texture);
	}

	void RenderTarget::DetachDepthTexture()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, GL_NONE, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

		depth.reset();
	}

	void RenderTarget::Clear() const
	{
		GLbitfield clearFlags = 0x0;
		if (HasDepth())
		{
			clearFlags |= GL_DEPTH_BUFFER_BIT;
		}

		if (colors)
		{
			clearFlags |= GL_COLOR_BUFFER_BIT;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glClear(clearFlags);
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	void RenderTarget::ClearDepth() const
	{
		ASSERT(HasDepth(), "RenderTarget does not have a depth texture to clear.");

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	void RenderTarget::ClearDepth(float _depth) const
	{
		ASSERT(HasDepth(), "RenderTarget does not have a depth texture to clear.");
		ASSERT(_depth >= 0.0f && _depth <= 1.0f, "'depth' must be in the range of [0, 1].");

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glClearBufferfv(GL_DEPTH, 0, &_depth);
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	void RenderTarget::ClearColor() const
	{
		ASSERT(numColorTextures > 0, "RenderTarget does not have a color texture to clear.");

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	void RenderTarget::ClearColor(unsigned index) const
	{
		// Use the current clear color.
		vec4 color;
		glGetFloatv(GL_COLOR_CLEAR_VALUE, &color.x);

		ClearColor(index, color);
	}

	void RenderTarget::ClearColor(unsigned index, const vec4& color) const
	{
		ASSERT(numColorTextures > 0, "RenderTarget does not have a color texture to clear.");
		ASSERT(index < numColorTextures, "'index' must specify a valid color texture.");

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glClearBufferfv(GL_COLOR, static_cast<int>(index), &color.x);
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	void RenderTarget::Bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glEnable(GL_FRAMEBUFFER_SRGB);
	}

	void RenderTarget::UnBind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

		// Weather or not the backbuffer will be considered an sRGB buffer is not well defined.
		// For this reason we disable the automatic linear->SRGB conversions when targeting the backbuffer.
		// This allows us to maintain consistent behaviour across all graphics drivers.
		glDisable(GL_FRAMEBUFFER_SRGB);
	}

	void RenderTarget::ResolveMultisampling(const RenderTarget& target) const
	{
		ResolveMultisamplingDepth(target);
		ResolveMultisamplingColor(target);
	}

	void RenderTarget::ResolveMultisamplingDepth(const RenderTarget& target) const
	{
		ASSERT(HasDepth() && target.HasDepth(), "Both RenderTargets must have depth textures.");
		ASSERT(IsMultisampled(), "Cannot resolve from a RenderTarget that is not multisampled.");
		ASSERT(target.numSamples == 1, "'target' must not be multisampled.");
		ASSERT(width == target.width && height == target.height, "RenderTargets must have the same dimensions to resolve a depth texture.");

		glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.FBO);

		glBlitFramebuffer(0, 0, width, height, 0, 0, target.width, target.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	void RenderTarget::ResolveMultisamplingColor(const RenderTarget& target) const
	{
		ASSERT(numColorTextures == target.numColorTextures, "Both RenderTargets must have the same number of color textures.");
		ASSERT(IsMultisampled(), "Cannot resolve from a RenderTarget that is not multisampled.");
		ASSERT(target.numSamples == 1, "'target' must not be multisampled.");

		const GLenum filter = (width == target.width) && (height == target.height) ? GL_NEAREST : GL_LINEAR;

		glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.FBO);

		for (unsigned i = 0; i < numColorTextures; ++i)
		{
			glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
			glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
			glBlitFramebuffer(0, 0, width, height, 0, 0, target.width, target.height, GL_COLOR_BUFFER_BIT, filter);
		}

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	bool RenderTarget::Resize(unsigned newWidth, unsigned newHeight)
	{
		ASSERT(newWidth > 0, "'newWidth' must be greater than 0.");
		ASSERT(newHeight > 0, "'newHeight' must be greater than 0.");

		if (width == newWidth && height == newHeight)
			return true;

		width = newWidth;
		height = newHeight;

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		if (depth)
		{
			depth->Unload();
			depth->CreateTexture(
				newWidth, newHeight,
				TextureFormat::DEPTH_24, TextureFilter::Point,
				TextureWrap::Clamp, 1.0f, numSamples);

			glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				GL_DEPTH_ATTACHMENT,
				IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
				depth->GetHandle(),
				0);
		}

		for (unsigned i = 0; i < numColorTextures; ++i)
		{
			// Preserve the states of the texture, just in case the user changed them.
			const TextureFormat oldFormat = colors[i]->GetFormat();
			const TextureFilter oldFilter = colors[i]->GetFilter();
			const TextureWraps oldWrap = colors[i]->GetWrap();
			const float oldAnisotropicLevel = colors[i]->GetAnisotropicLevel();
			colors[i]->Unload();

			colors[i]->CreateTexture(newWidth, newHeight, oldFormat, oldFilter, oldWrap, oldAnisotropicLevel, numSamples);

			glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0 + i,
				IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
				colors[i]->GetHandle(),
				0);
		}

		// Validate will also unbind the buffer for us.
		return Validate();
	}

	void RenderTarget::CopyDepth(const RenderTarget& target) const
	{
		ASSERT(HasDepth(), "RenderTarget must have depth texture to copy.");
		ASSERT(target.HasDepth(), "'target' must have depth texture.");
		ASSERT(numSamples == target.numSamples, "Source and target RenderTargets must have the same sample count.");
		ASSERT(width == target.width && height == target.height, "Source and target RenderTargets must have the same dimensions.");

		glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.FBO);

		glBlitFramebuffer(0, 0, width, height, 0, 0, target.width, target.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	void RenderTarget::CopyColor(const RenderTarget& target, unsigned index) const
	{
		ASSERT(numColorTextures > 0, "RenderTarget does not have a color texture to copy.");
		ASSERT(target.numColorTextures > 0, "'target' RenderTarget does not have a color texture to copy.");
		ASSERT(index < numColorTextures, "'index' must specify a valid color texture.");
		ASSERT(index < target.numColorTextures, "'target' does not have a color texture at index %u", index);

		const GLenum filter = (width == target.width) && (height == target.height) ? GL_NEAREST : GL_LINEAR;

		glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.FBO);
		glReadBuffer(GL_COLOR_ATTACHMENT0 + index);
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + index);

		glBlitFramebuffer(0, 0, width, height, 0, 0, target.width, target.height, GL_COLOR_BUFFER_BIT, filter);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	void RenderTarget::CopyDepthToBackBuffer() const
	{
		ASSERT(HasDepth(), "RenderTarget does not have a depth texture to copy.");
		ASSERT(width == static_cast<unsigned>(Application.GetScreenWidth()) && height == static_cast<unsigned>(Application.GetScreenHeight()),
			"RenderTarget and BackBuffer must have the same dimensions.");

		glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);

		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	void RenderTarget::CopyColorToBackBuffer(unsigned index) const
	{
		ASSERT(numColorTextures > 0, "RenderTarget does not have a color texture to copy.");
		ASSERT(index < numColorTextures, "'index' must specify a valid color texture.");

		const GLenum filter = (width == static_cast<unsigned>(Application.GetScreenWidth())) &&
			(height == static_cast<unsigned>(Application.GetScreenHeight())) ? GL_NEAREST : GL_LINEAR;

		glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);
		glReadBuffer(GL_COLOR_ATTACHMENT0 + index);

		glBlitFramebuffer(0, 0, width, height, 0, 0, Application.GetScreenWidth(), Application.GetScreenHeight(), GL_COLOR_BUFFER_BIT, filter);

		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	}

	vec4 RenderTarget::ReadPixel(unsigned index, const vec2& position) const
	{
		ASSERT(index < numColorTextures, "'index' must specify a valid color texture.");
		ASSERT(numSamples == 1, "RenderTarget must not be multi-sampled.");

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glReadBuffer(GL_COLOR_ATTACHMENT0 + index);

		unsigned format = 0;
		switch (CountChannels(colors[index]->GetFormat()))
		{
		case 4: format = GL_RGBA;
			break;
		case 3: format = GL_RGB;
			break;
		case 1: format = GL_RED;
			break;
		default:
			ASSERT(false, "Unsupported texture format.");
		}

		vec4 result;
		switch (colors[index]->GetFormat())
		{
		case TextureFormat::R_8:
		case TextureFormat::RGB_8:
		case TextureFormat::RGBA_8:
		case TextureFormat::sRGB_8:
		case TextureFormat::sRGBA_8:
			{
				unsigned char pixel[4] = { 0 };
				glReadPixels(static_cast<int>(position.x), static_cast<int>(position.y), 1, 1, format, GL_UNSIGNED_BYTE, pixel);
				// Normalize color range to [0, 1].
				result = vec4(pixel[0], pixel[1], pixel[2], pixel[3]) / static_cast<float>(UCHAR_MAX);
			} break;

		case TextureFormat::R_16:
		case TextureFormat::RGB_16:
		case TextureFormat::RGBA_16:
			{
				unsigned short pixel[4] = { 0 };
				glReadPixels(static_cast<int>(position.x), static_cast<int>(position.y), 1, 1, format, GL_UNSIGNED_SHORT, pixel);
				// Normalize color range to [0, 1].
				result = vec4(pixel[0], pixel[1], pixel[2], pixel[3]) / static_cast<float>(USHRT_MAX);
			} break;

		case TextureFormat::R_32:
		case TextureFormat::RGB_32:
		case TextureFormat::RGBA_32:
			{
				unsigned pixel[4] = { 0 };
				glReadPixels(static_cast<int>(position.x), static_cast<int>(position.y), 1, 1, format, GL_UNSIGNED_INT, pixel);
				// Normalize color range to [0, 1].
				result = vec4(
					static_cast<float>(pixel[0]),
					static_cast<float>(pixel[1]),
					static_cast<float>(pixel[2]),
					static_cast<float>(pixel[3])) / static_cast<float>(UINT_MAX);
			} break;

		case TextureFormat::R_16F:
		case TextureFormat::R_32F:
		case TextureFormat::RGB_16F:
		case TextureFormat::RGB_32F:
		case TextureFormat::RGBA_16F:
		case TextureFormat::RGBA_32F:
			glReadPixels(static_cast<int>(position.x), static_cast<int>(position.y), 1, 1, format, GL_FLOAT, &result.x);
			break;

		default:
			ASSERT(false, "Unsupported texture format.");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

		return result;
	}

	bool RenderTarget::HasDepth() const
	{
		return !!depth;
	}

	bool RenderTarget::IsMultisampled() const
	{
		return numSamples != 1;
	}

	unsigned RenderTarget::GetNumColorTextures() const
	{
		return numColorTextures;
	}

	unsigned RenderTarget::GetNumSamples() const
	{
		return numSamples;
	}

	Texture::Ptr RenderTarget::GetDepthTexture() const
	{
		return depth;
	}

	Texture::Ptr RenderTarget::GetColorTexture(unsigned index) const
	{
		ASSERT(numColorTextures > 0, "RenderTarget does not have any color textures.");
		ASSERT(index < numColorTextures, "'index' must specify a valid color texture.");

		return colors[index];
	}

	Viewport RenderTarget::GetViewport() const
	{
		return Viewport(0, 0, width, height);
	}

	unsigned RenderTarget::GetWidth() const
	{
		return width;
	}

	unsigned RenderTarget::GetHeight() const
	{
		return height;
	}
}
