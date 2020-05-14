#include "TextureEncoder.h"
#include <gemcutter/Rendering/Rendering.h>
#include <gemcutter/Resource/Texture.h>
#include <gemcutter/Utilities/String.h>

#define CURRENT_VERSION 2

TextureEncoder::TextureEncoder()
	: gem::Encoder(CURRENT_VERSION)
{
}

gem::ConfigTable TextureEncoder::GetDefault() const
{
	gem::ConfigTable defaultConfig;

	defaultConfig.SetValue("version", CURRENT_VERSION);
	defaultConfig.SetValue("anisotropic_level", 1.0f);
	defaultConfig.SetValue("cubemap", "false");
	defaultConfig.SetValue("filter", "linear");
	defaultConfig.SetValue("wrap_x", "clamp");
	defaultConfig.SetValue("wrap_y", "clamp");
	defaultConfig.SetValue("s_rgb", "true");

	return defaultConfig;
}

bool TextureEncoder::Validate(const gem::ConfigTable& metadata, unsigned loadedVersion) const
{
	auto validateAnisotropicLevel = [](const gem::ConfigTable& data)
	{
		if (!data.HasSetting("anisotropic_level"))
		{
			gem::Error("Missing \"anisotropic_level\" value.");
			return false;
		}

		float anisotropicLevel = data.GetFloat("anisotropic_level");
		if (anisotropicLevel < 1.0f || anisotropicLevel > 16.0f)
		{
			gem::Error("\"anisotropic_level\" must be in the range of [1, 16].");
			return false;
		}

		return true;
	};

	auto validateTextureFilter = [](const gem::ConfigTable& data)
	{
		if (!data.HasSetting("filter"))
		{
			gem::Error("Missing \"filter\" value.");
			return false;
		}

		auto str = data.GetString("filter");
		if (!gem::CompareLowercase(str, "point") &&
			!gem::CompareLowercase(str, "linear") &&
			!gem::CompareLowercase(str, "bilinear") &&
			!gem::CompareLowercase(str, "trilinear"))
		{
			gem::Error("\"filter\" is invalid. Valid options are \"point\", \"linear\", \"bilinear\", or \"trilinear\".");
			return false;
		}

		return true;
	};

	auto validateWrap = [](const gem::ConfigTable& data, const char* mode)
	{
		if (!data.HasSetting(mode))
		{
			gem::Error("Missing \"%s\" value.", mode);
			return false;
		}

		auto str = data.GetString(mode);
		if (!gem::CompareLowercase(str, "clamp") &&
			!gem::CompareLowercase(str, "clampWithBorder") &&
			!gem::CompareLowercase(str, "repeat") &&
			!gem::CompareLowercase(str, "repeatMirrored") &&
			!gem::CompareLowercase(str, "repeatMirroredOnce"))
		{
			gem::Error("\"%s\" is invalid. Valid options are \"clamp\", \"clampWithBorder\", \"repeat\", \"repeatMirrored\", or \"repeatMirroredOnce\".", mode);
			return false;
		}

		return true;
	};

	if (!metadata.HasSetting("cubemap"))
	{
		gem::Error("Missing \"cubemap\" value.");
		return false;
	}

	if (!validateAnisotropicLevel(metadata)) return false;
	if (!validateTextureFilter(metadata)) return false;
	if (!validateWrap(metadata, "wrap_x")) return false;
	if (!validateWrap(metadata, "wrap_y")) return false;

	switch (loadedVersion)
	{
	case 1:
		if (metadata.GetSize() != 6)
		{
			gem::Error("Incorrect number of value entries.");
			return false;
		}
		break;

	case 2:
		if (!metadata.HasSetting("s_rgb"))
		{
			gem::Error("Missing \"s_rgb\" value.");
			return false;
		}

		if (metadata.GetSize() != 7)
		{
			gem::Error("Incorrect number of value entries.");
			return false;
		}
		break;
	}

	return true;
}

bool TextureEncoder::Convert(std::string_view source, std::string_view destination, const gem::ConfigTable& metadata) const
{
	const std::string outputFile = std::string(destination) + gem::ExtractFilename(source) + ".texture";
	const float anisotropicLevel = metadata.GetFloat("anisotropic_level");
	const bool isCubemap = metadata.GetBool("cubemap");
	const bool isSRGB = metadata.GetBool("s_rgb");
	const gem::TextureFilter filter = gem::StringToTextureFilter(metadata.GetString("filter"));
	const gem::TextureWrap wrapX = gem::StringToTextureWrap(metadata.GetString("wrap_x"));
	const gem::TextureWrap wrapY = gem::StringToTextureWrap(metadata.GetString("wrap_y"));

	auto image = gem::RawImage::Load(source, !isCubemap, isSRGB);
	if (image.data == nullptr)
		return false;

	const unsigned elementCount = gem::CountChannels(image.format);

	// Save file.
	FILE* fontFile = fopen(outputFile.c_str(), "wb");
	if (fontFile == nullptr)
	{
		gem::Error("Output file could not be created.");
		return false;
	}

	if (isCubemap)
	{
		// Validate size. Width to height ratio must be 4/3.
		if (image.width * 3 != image.height * 4)
		{
			fclose(fontFile);
			gem::Error("Cubemap texture layout is incorrect.");
			return false;
		}

		if (wrapX != gem::TextureWrap::Clamp ||
			wrapY != gem::TextureWrap::Clamp)
		{
			gem::Warning("Cubemaps must have \"clamp\" wrap modes. Forcing wrap modes to \"clamp\".");
		}

		const unsigned faceSize = image.width / 4;
		const gem::TextureWrap cubeMapWrapMode = gem::TextureWrap::Clamp;

		// Write header.
		fwrite(&isCubemap, sizeof(bool), 1, fontFile);
		fwrite(&faceSize, sizeof(unsigned), 1, fontFile);
		fwrite(&faceSize, sizeof(unsigned), 1, fontFile);
		fwrite(&image.format, sizeof(gem::TextureFormat), 1, fontFile);
		fwrite(&filter, sizeof(gem::TextureFilter), 1, fontFile);
		fwrite(&cubeMapWrapMode, sizeof(gem::TextureWrap), 1, fontFile);
		fwrite(&cubeMapWrapMode, sizeof(gem::TextureWrap), 1, fontFile);
		fwrite(&anisotropicLevel, sizeof(float), 1, fontFile);

		// Write faces.
		const unsigned textureSize = faceSize * faceSize * elementCount;
		unsigned char* data = static_cast<unsigned char*>(malloc(sizeof(unsigned char) * textureSize * 6));
		defer { free(data); };

		// Strip out the dead space of the source texture, the final data is 6 textures in sequence.
		auto copyFace = [&image, faceSize, elementCount](unsigned char* dest, unsigned startX, unsigned startY)
		{
			for (unsigned y = startY; y < startY + faceSize; ++y)
			{
				for (unsigned x = startX; x < startX + faceSize; ++x)
				{
					unsigned index = (x + y * image.width) * elementCount;

					memcpy(dest, image.data + index, elementCount);
					dest += elementCount;
				}
			}
		};

		copyFace(data, faceSize * 2, faceSize);						// +X
		copyFace(data + 1 * textureSize, 0, faceSize);				// -X
		copyFace(data + 2 * textureSize, faceSize, 0);				// +Y
		copyFace(data + 3 * textureSize, faceSize, faceSize * 2);	// -Y
		copyFace(data + 4 * textureSize, faceSize, faceSize);		// +Z
		copyFace(data + 5 * textureSize, faceSize * 3, faceSize);	// -Z

		fwrite(data, sizeof(unsigned char), textureSize * 6, fontFile);
	}
	else
	{
		// Write header.
		fwrite(&isCubemap, sizeof(bool), 1, fontFile);
		fwrite(&image.width, sizeof(unsigned), 1, fontFile);
		fwrite(&image.height, sizeof(unsigned), 1, fontFile);
		fwrite(&image.format, sizeof(gem::TextureFormat), 1, fontFile);
		fwrite(&filter, sizeof(gem::TextureFilter), 1, fontFile);
		fwrite(&wrapX, sizeof(gem::TextureWrap), 1, fontFile);
		fwrite(&wrapY, sizeof(gem::TextureWrap), 1, fontFile);
		fwrite(&anisotropicLevel, sizeof(float), 1, fontFile);

		// Write Data.
		const unsigned textureSize = image.width * image.height * elementCount;
		fwrite(image.data, sizeof(unsigned char), textureSize, fontFile);
	}

	auto result = fclose(fontFile);
	if (result != 0)
	{
		gem::Error("Failed to generate Texture Binary\nOutput file could not be saved.");
		return false;
	}

	return true;
}

bool TextureEncoder::Upgrade(gem::ConfigTable& metadata, unsigned loadedVersion) const
{
	switch (loadedVersion)
	{
	case 1:
		// Added s_rgb field.
		metadata.SetValue("s_rgb", "true");
		break;
	}

	return true;
}
