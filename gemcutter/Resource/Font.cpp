// Copyright (c) 2017 Emilian Cioca
#include "Font.h"
#include "gemcutter/Application/Logging.h"
#include "gemcutter/Math/Math.h"
#include "gemcutter/Rendering/Rendering.h"
#include "gemcutter/Resource/Texture.h"
#include "gemcutter/Utilities/ScopeGuard.h"
#include "gemcutter/Utilities/String.h"

#include <cstdio>
#include <glew/glew.h>

namespace gem
{
	unsigned Font::VAO = 0;
	unsigned Font::VBO = 0;

	Font::Font()
	{
		memset(dimensions, 0, sizeof(CharData) * 94);
		memset(positions,  0, sizeof(CharData) * 94);
		memset(advances,   0, sizeof(CharData) * 94);
		memset(masks,      false, sizeof(bool) * 94);
	}

	Font::~Font()
	{
		Unload();
	}

	bool Font::Load(std::string filePath)
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
				// face 1
				0.0f, 0.0f,
				1.0f, 0.0f,
				0.0f, 1.0f,
				// face 2
				1.0f, 0.0f,
				1.0f, 1.0f,
				0.0f, 1.0f,

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
		auto ext = ExtractFileExtension(filePath);
		if (ext.empty())
		{
			filePath += ".font";
		}
		else if (!CompareLowercase(ExtractFileExtension(filePath), ".font"))
		{
			Error("Font: ( %s )\nAttempted to load unknown file type as a font.", filePath.c_str());
			return false;
		}

		FILE* fontFile = fopen(filePath.c_str(), "rb");
		if (fontFile == nullptr)
		{
			Error("Font: ( %s )\nUnable to open file.", filePath.c_str());
			return false;
		}

		// Load bitmap data.
		TextureFilter filter = TextureFilter::Point;
		unsigned textureWidth = 0;
		unsigned char* bitmap = nullptr;
		defer{ free(bitmap); };

		// Read header.
		fread(&textureWidth, sizeof(unsigned), 1, fontFile);
		fread(&width, sizeof(unsigned), 1, fontFile);
		fread(&height, sizeof(unsigned), 1, fontFile);
		fread(&filter, sizeof(TextureFilter), 1, fontFile);

		// Load Data.
		bitmap = static_cast<unsigned char*>(malloc(sizeof(unsigned char) * textureWidth * textureWidth));
		fread(bitmap, sizeof(unsigned char), textureWidth * textureWidth, fontFile);
		fread(dimensions, sizeof(CharData), 94, fontFile);
		fread(positions, sizeof(CharData), 94, fontFile);
		fread(advances, sizeof(CharData), 94, fontFile);
		fread(masks, sizeof(bool), 94, fontFile);

		fclose(fontFile);

		// Upload data to OpenGL.
		texture = Texture::MakeNew();
		texture->Create(textureWidth, textureWidth, TextureFormat::R_8, filter, TextureWrap::Clamp);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		texture->SetData(bitmap, TextureFormat::R_8);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		return true;
	}

	void Font::Unload()
	{
		texture->Unload();
	}

	int Font::GetStringWidth(std::string_view text) const
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

	Texture::Ptr Font::GetTexture() const
	{
		return texture;
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
