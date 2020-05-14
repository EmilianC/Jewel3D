// Copyright (c) 2017 Emilian Cioca
#pragma once
#include "gemcutter/Rendering/Rendering.h"
#include "gemcutter/Resource/Resource.h"
#include "gemcutter/Resource/Shareable.h"

#include <string_view>
#include <vector>

namespace gem
{
	// A 2D texture, renderTarget, or cubemap.
	class Texture : public Resource<Texture>, public Shareable<Texture>
	{
	public:
		Texture() = default;
		~Texture();

		// Creates an empty texture that can be rendered to.
		void CreateTexture(
			unsigned width, unsigned height,
			TextureFormat format,
			TextureFilter filter = TextureFilter::Point,
			TextureWraps wraps = TextureWrap::Clamp,
			float anisotropicLevel = 1.0f,
			unsigned numSamples = 1);

		// Loads packed *.texture resources, as well as *.png, *.jpg, *.tga, *.bmp.
		bool Load(std::string filePath);
		void Unload();

		void Bind(unsigned slot);
		void UnBind(unsigned slot);

		void SetFilter(TextureFilter filter);
		void SetWrap(TextureWraps wraps);
		// Must be in the range of [1, 16].
		void SetAnisotropicLevel(float level);

		unsigned GetHandle() const;
		unsigned GetNumSamples() const;
		unsigned GetBindingTarget() const;
		unsigned GetWidth() const;
		unsigned GetHeight() const;
		TextureFormat GetFormat() const;
		TextureFilter GetFilter() const;
		TextureWraps GetWrap() const;
		float GetAnisotropicLevel() const;
		float GetAspectRatio() const;

		bool IsCubeMap() const;

		void RegenerateMipmaps();

	private:
		unsigned hTex       = 0;
		unsigned numSamples = 1;
		unsigned target     = 0;
		int width           = 0;
		int height          = 0;
		TextureFormat format = TextureFormat::RGB_8;
		TextureFilter filter = TextureFilter::Point;
		TextureWraps wraps = TextureWrap::Clamp;
		float anisotropicLevel = 1.0f;
	};

	// Used to associate a Texture with a particular binding point.
	struct TextureSlot
	{
		TextureSlot() = default;
		TextureSlot(Texture::Ptr tex, unsigned unit = 0);

		void Bind() const;
		void UnBind() const;

		Texture::Ptr tex;
		// Unit the texture should be bound to when rendering.
		unsigned unit = 0;
	};

	// A group of textures that are bound and unbound together.
	class TextureList
	{
	public:
		void Bind() const;
		void UnBind() const;

		void Add(Texture::Ptr tex, unsigned unit = 0);
		void Remove(unsigned unit);
		void Clear();

		// Returns the buffer bound at the specified unit.
		Texture::Ptr& operator[](unsigned unit);

		const auto& GetAll() const { return textureSlots; }

	private:
		std::vector<TextureSlot> textureSlots;
	};

	// Loads raw image data from the disk.
	// Can be used when an image is needed for non-rendering use.
	class RawImage
	{
	public:
		~RawImage();

		// Loads *.png, *.jpg, *.tga, and *.bmp.
		static RawImage Load(std::string_view file, bool flipY, bool sRGB);

		const int width;
		const int height;
		const TextureFormat format;
		const unsigned char* data;

	private:
		RawImage(int width, int height, TextureFormat format, const unsigned char* data);
	};

	// Flips the image contained in `data` on the Y axis.
	void FlipImage(unsigned char* data, int width, int height, int numChannels);
}
