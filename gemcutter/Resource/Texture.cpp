// Copyright (c) 2017 Emilian Cioca
#include "Texture.h"
#include "gemcutter/Application/Logging.h"
#include "gemcutter/Utilities/ScopeGuard.h"
#include "gemcutter/Utilities/String.h"

#include <algorithm>
#include <glew/glew.h>
#include <soil/SOIL.h>

namespace gem
{
	Texture::~Texture()
	{
		Unload();
	}

	void Texture::CreateTexture(unsigned _width, unsigned _height, TextureFormat _format, TextureFilter _filter, TextureWraps _wraps, float _anisotropicLevel, unsigned _numSamples)
	{
		ASSERT(hTex == 0, "Texture already has a texture loaded.");
		ASSERT(_anisotropicLevel >= 1.0f && _anisotropicLevel <= 16.0f, "'anisotropicLevel' must be in the range of [1, 16].");
		ASSERT(_numSamples == 1 || _numSamples == 2 || _numSamples == 4 || _numSamples == 8 || _numSamples == 16, "'numSamples' must be a power of 2 between 1 and 16.");

		width = static_cast<int>(_width);
		height = static_cast<int>(_height);
		format = _format;
		filter = _filter;
		wraps = _wraps;
		anisotropicLevel = _anisotropicLevel;
		numSamples = _numSamples;

		glGenTextures(1, &hTex);

		if (numSamples == 1)
		{
			glBindTexture(GL_TEXTURE_2D, hTex);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ResolveFilterMag(filter));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ResolveFilterMin(filter));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ResolveWrap(wraps.x));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ResolveWrap(wraps.y));
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropicLevel);

			unsigned numLevels = CountMipLevels(width, height, filter);
			glTexStorage2D(GL_TEXTURE_2D, numLevels, ResolveFormat(format), width, height);

			if (numLevels > 1)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
			}

			target = GL_TEXTURE_2D;
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, hTex);
			glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, ResolveFormat(format), width, height, false);

			target = GL_TEXTURE_2D_MULTISAMPLE;
		}

		glBindTexture(target, GL_NONE);
	}

	bool Texture::Load(std::string filePath)
	{
		ASSERT(hTex == 0, "Texture already has a texture loaded.");

		unsigned numLevels = 0;

		auto ext = ExtractFileExtension(filePath);
		if (ext.empty() || CompareLowercase(ext, ".texture"))
		{
			if (ext.empty())
			{
				filePath += ".texture";
			}

			FILE* fontFile = fopen(filePath.c_str(), "rb");
			if (fontFile == nullptr)
			{
				Error("Texture: ( %s )\nUnable to open file.", filePath.c_str());
				return false;
			}
			defer { fclose(fontFile); };

			// Read header.
			bool isCubeMap = false;
			fread(&isCubeMap, sizeof(bool), 1, fontFile);
			fread(&width, sizeof(unsigned), 1, fontFile);
			fread(&height, sizeof(unsigned), 1, fontFile);
			fread(&format, sizeof(TextureFormat), 1, fontFile);
			fread(&filter, sizeof(TextureFilter), 1, fontFile);
			fread(&wraps.x, sizeof(TextureWrap), 1, fontFile);
			fread(&wraps.y, sizeof(TextureWrap), 1, fontFile);
			fread(&anisotropicLevel, sizeof(float), 1, fontFile);

			numLevels = CountMipLevels(width, height, filter);
			const unsigned textureSize = width * height * CountChannels(format);
			const unsigned dataFormat = CountChannels(format) == 3 ? GL_RGB : GL_RGBA;
			if (isCubeMap)
			{
				auto* image = static_cast<unsigned char*>(malloc(sizeof(unsigned char) * textureSize * 6));
				defer{ free(image); };

				fread(image, sizeof(unsigned char), textureSize * 6, fontFile);

				glGenTextures(1, &hTex);
				glBindTexture(GL_TEXTURE_CUBE_MAP, hTex);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, ResolveFilterMag(filter));
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, ResolveFilterMin(filter));
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropicLevel);

				glTexStorage2D(GL_TEXTURE_CUBE_MAP, numLevels, ResolveFormat(format), width, height);

				for (unsigned i = 0; i < 6; ++i)
				{
					glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, image + (textureSize * i));
				}

				target = GL_TEXTURE_CUBE_MAP;
			}
			else
			{
				auto* image = static_cast<unsigned char*>(malloc(sizeof(unsigned char) * textureSize));
				defer{ free(image); };

				fread(image, sizeof(unsigned char), textureSize, fontFile);

				glGenTextures(1, &hTex);
				glBindTexture(GL_TEXTURE_2D, hTex);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ResolveFilterMag(filter));
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ResolveFilterMin(filter));
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ResolveWrap(wraps.x));
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ResolveWrap(wraps.y));
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropicLevel);

				glTexStorage2D(GL_TEXTURE_2D, numLevels, ResolveFormat(format), width, height);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, image);

				target = GL_TEXTURE_2D;
			}
		}
		else
		{
			auto image = RawImage::Load(filePath, true, false);
			if (image.data == nullptr)
				return false;

			width = image.width;
			height = image.height;
			format = image.format;
			numLevels = CountMipLevels(width, height, filter);

			glGenTextures(1, &hTex);
			glBindTexture(GL_TEXTURE_2D, hTex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ResolveFilterMag(filter));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ResolveFilterMin(filter));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ResolveWrap(wraps.x));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ResolveWrap(wraps.y));
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropicLevel);

			const unsigned dataFormat = CountChannels(format) == 3 ? GL_RGB : GL_RGBA;

			glTexStorage2D(GL_TEXTURE_2D, numLevels, ResolveFormat(format), width, height);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, image.data);

			target = GL_TEXTURE_2D;
		}

		if (numLevels > 1)
		{
			glGenerateMipmap(target);
		}

		glBindTexture(target, GL_NONE);

		return true;
	}

	void Texture::SetFilter(TextureFilter _filter)
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");
		ASSERT(numSamples == 1, "It is illegal to change the filter mode of a multisampled texture.");

		glBindTexture(target, hTex);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, ResolveFilterMag(_filter));
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, ResolveFilterMin(_filter));
		glBindTexture(target, GL_NONE);

		filter = _filter;
	}

	void Texture::SetWrap(TextureWraps _wraps)
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");
		ASSERT(numSamples == 1, "It is illegal to change the wrap mode of a multisampled texture.");

		glBindTexture(target, hTex);
		glTexParameteri(target, GL_TEXTURE_WRAP_S, ResolveWrap(_wraps.x));
		glTexParameteri(target, GL_TEXTURE_WRAP_T, ResolveWrap(_wraps.y));
		glBindTexture(target, GL_NONE);

		wraps = _wraps;
	}

	void Texture::SetAnisotropicLevel(float level)
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");
		ASSERT(level >= 1.0f && level <= 16.0f, "'level' must be in the range of [1, 16].");
		ASSERT(numSamples == 1, "It is illegal to change the anisotropic level of a multisampled texture.");

		glBindTexture(target, hTex);
		glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, level);
		glBindTexture(target, GL_NONE);

		anisotropicLevel = level;
	}

	void Texture::Unload()
	{
		if (hTex != GL_NONE)
		{
			glDeleteTextures(1, &hTex);
			hTex = GL_NONE;
			target = GL_NONE;
		}
	}

	void Texture::Bind(unsigned slot)
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");

		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(target, hTex);
	}

	void Texture::UnBind(unsigned slot)
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(target, GL_NONE);
	}

	unsigned Texture::GetHandle() const
	{
		return hTex;
	}

	unsigned Texture::GetNumSamples() const
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");

		return numSamples;
	}

	unsigned Texture::GetBindingTarget() const
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");

		return target;
	}

	unsigned Texture::GetWidth() const
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");

		return width;
	}

	unsigned Texture::GetHeight() const
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");

		return height;
	}

	TextureFormat Texture::GetFormat() const
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");

		return format;
	}

	TextureFilter Texture::GetFilter() const
	{
		return filter;
	}

	TextureWraps Texture::GetWrap() const
	{
		return wraps;
	}

	float Texture::GetAnisotropicLevel() const
	{
		return anisotropicLevel;
	}

	float Texture::GetAspectRatio() const
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");

		return static_cast<float>(width) / static_cast<float>(height);
	}

	bool Texture::IsCubeMap() const
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");

		return target == GL_TEXTURE_CUBE_MAP;
	}

	void Texture::RegenerateMipmaps()
	{
		ASSERT(hTex != 0, "A texture must be loaded to call this function.");
		ASSERT(numSamples == 1, "It is illegal to generate mipmaps on a multisampled texture.");

		glBindTexture(target, hTex);
		glGenerateMipmap(target);
		glBindTexture(target, GL_NONE);
	}

	//-----------------------------------------------------------------------------------------------------

	TextureSlot::TextureSlot(Texture::Ptr tex, unsigned unit)
		: tex(std::move(tex)), unit(unit)
	{
	}

	void TextureSlot::Bind() const
	{
		tex->Bind(unit);
	}

	void TextureSlot::UnBind() const
	{
		tex->UnBind(unit);
	}

	//-----------------------------------------------------------------------------------------------------

	void TextureList::Bind() const
	{
		for (auto& slot : textureSlots)
		{
			slot.Bind();
		}
	}

	void TextureList::UnBind() const
	{
		for (auto& slot : textureSlots)
		{
			slot.UnBind();
		}
	}

	void TextureList::Add(Texture::Ptr tex, unsigned unit)
	{
		ASSERT(tex, "'tex' cannot be null.");

		for (auto& slot : textureSlots)
		{
			if (slot.unit == unit)
			{
				slot.tex = std::move(tex);
				return;
			}
		}

		textureSlots.emplace_back(std::move(tex), unit);
	}

	void TextureList::Remove(unsigned unit)
	{
		for (auto itr = textureSlots.begin(); itr < textureSlots.end(); ++itr)
		{
			if (itr->unit == unit)
			{
				textureSlots.erase(itr);
				return;
			}
		}
	}

	void TextureList::Clear()
	{
		textureSlots.clear();
	}

	Texture::Ptr& TextureList::operator[](unsigned unit)
	{
		auto textureSlot = std::find_if(textureSlots.begin(), textureSlots.end(), [unit](TextureSlot& slot) {
			return slot.unit == unit;
		});

		ASSERT(textureSlot != textureSlots.end(), "No matching texture found.");
		return textureSlot->tex;
	}

	//-----------------------------------------------------------------------------------------------------

	RawImage::RawImage(int _width, int _height, TextureFormat _format, const unsigned char* _data)
		: width(_width)
		, height(_height)
		, format(_format)
		, data(_data)
	{
	}

	RawImage::~RawImage()
	{
		free(const_cast<unsigned char*>(data));
	}

	RawImage RawImage::Load(std::string_view file, bool flipY, bool sRGB)
	{
		int width = 0;
		int height = 0;
		int numChannels = 0;

		unsigned char* data = SOIL_load_image(file.data(), &width, &height, &numChannels, SOIL_LOAD_AUTO);
		if (data == nullptr)
		{
			Error("Texture: ( %s )\n%s", file.data(), SOIL_last_result());
			return RawImage(0, 0, TextureFormat::RGB_8, nullptr);
		}

		// Invert the Y axis for OpenGL texture addressing.
		if (flipY)
		{
			FlipImage(data, width, height, numChannels);
		}

		if (numChannels == 3)
		{
			return RawImage(width, height, sRGB ? TextureFormat::sRGB_8 : TextureFormat::RGB_8, data);
		}
		else if (numChannels == 4)
		{
			return RawImage(width, height, sRGB ? TextureFormat::sRGBA_8 : TextureFormat::RGBA_8, data);
		}
		else
		{
			Error("Texture: ( %s )\nUnsupported format. Must have 3 or 4 color channels.", file.data());
			return RawImage(0, 0, TextureFormat::RGB_8, nullptr);
		}
	}

	void FlipImage(unsigned char* data, int width, int height, int numChannels)
	{
		const int totalWidth = width * numChannels;
		for (int j = 0; j * 2 < height; ++j)
		{
			int index1 = j * totalWidth;
			int index2 = (height - 1 - j) * totalWidth;
			for (int i = totalWidth; i > 0; --i)
			{
				std::swap(data[index1++], data[index2++]);
			}
		}
	}
}
