// Copyright (c) 2017 Emilian Cioca
#include "Jewel3D/Precompiled.h"
#include "Font.h"
#include "Texture.h"
#include "Jewel3D/Application/Logging.h"
#include "Jewel3D/Math/Math.h"

#include <GLEW/GL/glew.h>
#include <algorithm>
#include <cstdio>

namespace Jwl
{
	unsigned Font::VAO = 0;
	unsigned Font::VBO = 0;

	Font::Font()
	{
		memset(textures,	0, sizeof(unsigned) * 94);
		memset(dimensions,	0, sizeof(CharData) * 94);
		memset(positions,	0, sizeof(CharData) * 94);
		memset(advances,	0, sizeof(CharData) * 94);
		memset(masks,		false, sizeof(bool) * 94);
	}

	Font::~Font()
	{
		Unload();
	}

	bool Font::Load(const std::string& filePath)
	{
		return Load(filePath, Texture::GetDefaultFilterMode());
	}

	bool Font::Load(const std::string& filePath, TextureFilterMode filter)
	{
		if (VAO == GL_NONE)
		{
			// Prepare VBO's and VAO.
			unsigned verticesSize = sizeof(float) * 6 * 3;
			unsigned texCoordsSize = sizeof(float) * 6 * 2;
			unsigned normalsSize = sizeof(float) * 6 * 3;
			GLfloat data[48] =
			{
				/* Vertices */
				// face 1
				0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f,
				// face 2
				0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f,

				/* Texcoords */
				// The texture's are upside down, so we flip the uvs
				// face 1
				0.0f, 1.0f,
				1.0f, 1.0f,
				0.0f, 0.0f,
				// face 2
				1.0f, 1.0f,
				1.0f, 0.0f,
				0.0f, 0.0f,

				/* Normals */
				// face 1
				0.0f, 0.0f, 1.0f,
				0.0f, 0.0f, 1.0f,
				0.0f, 0.0f, 1.0f,
				// face 2
				0.0f, 0.0f, 1.0f,
				0.0f, 0.0f, 1.0f,
				0.0f, 0.0f, 1.0f
			};

			glGenVertexArrays(1, &VAO);
			glBindVertexArray(VAO);

			glEnableVertexAttribArray(0); // Position
			glEnableVertexAttribArray(1); // UV
			glEnableVertexAttribArray(2); // Normals

			glGenBuffers(1, &VBO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, verticesSize + texCoordsSize + normalsSize, data, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(verticesSize));
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(verticesSize + texCoordsSize));

			glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
			glBindVertexArray(GL_NONE);
		}

		/* Load Font from file */
		if (filePath.find(".font") == std::string::npos)
		{
			Error("FontData: ( %s )\nAttempted to load unknown file type as font.", filePath.c_str());
			return false;
		}

		FILE* fontFile = fopen(filePath.c_str(), "rb");
		if (fontFile == nullptr)
		{
			Error("FontData: ( %s )\nUnable to open file.", filePath.c_str());
			return false;
		}

		// Load bitmap data.
		unsigned long int bitmapSize;
		unsigned char* bitmap = nullptr;

		// Read header.
		fread(&bitmapSize, sizeof(unsigned long int), 1, fontFile);
		fread(&width, sizeof(unsigned), 1, fontFile);
		fread(&height, sizeof(unsigned), 1, fontFile);

		// Load bitmap.
		bitmap = static_cast<unsigned char*>(malloc(sizeof(unsigned char) * bitmapSize));
		fread(bitmap, sizeof(unsigned char), bitmapSize, fontFile);
		// Load dimensions data.
		fread(dimensions, sizeof(int) * 2, 94, fontFile);
		// Load position data.
		fread(positions, sizeof(int) * 2, 94, fontFile);
		// Load advance data.
		fread(advances, sizeof(int) * 2, 94, fontFile);
		// Load mask.
		fread(masks, sizeof(unsigned char), 94, fontFile);

		fclose(fontFile);

		/* Upload data to OpenGL */
		unsigned char* bitmapItr = bitmap;
		for (unsigned i = 0; i < 94; i++)
		{
			if (!masks[i])
			{
				continue;
			}

			glGenTextures(1, &textures[i]);
			glBindTexture(GL_TEXTURE_2D, textures[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ResolveFilterMagMode(filter));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ResolveFilterMinMode(filter));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, Texture::GetDefaultAnisotropicLevel());

			// Send the texture data.
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4,
				dimensions[i].x,
				dimensions[i].y,
				0, GL_RGBA, GL_UNSIGNED_BYTE,
				bitmapItr);

			if (ResolveMipMapping(filter))
			{
				glGenerateMipmap(GL_TEXTURE_2D);
			}

			// Move the pointer to the next set of data.
			bitmapItr += dimensions[i].x * dimensions[i].y * 4;
		}

		glBindTexture(GL_TEXTURE_2D, GL_NONE);
		free(bitmap);

		return true;
	}

	void Font::Unload()
	{
		for (int i = 0; i < 94; i++)
		{
			glDeleteTextures(1, &textures[i]);
		}

		memset(textures, GL_NONE, sizeof(unsigned) * 94);
	}

	int Font::GetStringWidth(const std::string& text) const
	{
		int length = 0;
		int largest = INT_MIN;

		for (char ch : text)
		{
			if (ch == ' ')
			{
				length += advances['Z' - '!'].x;
			}
			else if (ch == '\n')
			{
				largest = Max(largest, length);
				length = 0;
			}
			else if (ch == '\t')
			{
				length += advances['Z' - '!'].x * 4;
			}
			else
			{
				length += advances[ch - '!'].x;
			}
		}

		return Max(largest, length);
	}

	int Font::GetStringHeight() const
	{
		return dimensions['Z' - '!'].y;
	}

	const unsigned* Font::GetTextures() const
	{
		return textures;
	}

	const CharData* Font::GetDimensions() const
	{
		return dimensions;
	}

	const CharData* Font::GetPositions() const
	{
		return positions;
	}

	const CharData* Font::GetAdvances() const
	{
		return advances;
	}

	const bool* Font::GetMasks() const
	{
		return masks;
	}

	unsigned Font::GetFontWidth() const
	{
		return width;
	}

	unsigned Font::GetFontHeight() const
	{
		return height;
	}

	unsigned Font::GetVAO()
	{
		return VAO;
	}

	unsigned Font::GetVBO()
	{
		return VBO;
	}
}
