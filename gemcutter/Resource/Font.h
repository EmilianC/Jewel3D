// Copyright (c) 2017 Emilian Cioca
#pragma once
#include "gemcutter/Resource/Resource.h"
#include "gemcutter/Resource/Shareable.h"
#include "gemcutter/Resource/Texture.h"

#include <string_view>

namespace gem
{
	struct CharData
	{
		int x;
		int y;
	};

	// A typeface that can be used to render text.
	// To be rendered, this must be set on an Entity's Text component.
	class Font : public Resource<Font>, public Shareable<Font>
	{
	public:
		Font();
		~Font();

		// Loads pre-packed *.font resources.
		bool Load(std::string filePath);
		void Unload();

		// Returns the real world unit width of the string.
		// If the string is multi-line, the length of the longest line is returned.
		int GetStringWidth(std::string_view text) const;
		int GetStringHeight() const;

		Texture::Ptr GetTexture() const;
		const CharData* GetDimensions() const;
		const CharData* GetPositions() const;
		const CharData* GetAdvances() const;
		const bool* GetMasks() const;
		unsigned GetFontWidth() const;
		unsigned GetFontHeight() const;

		static unsigned GetVAO();
		static unsigned GetVBO();

	private:
		Texture::Ptr texture;
		// Each character's dimensions.
		CharData dimensions[94];
		// The position of each character.
		CharData positions[94];
		// The distance from one character to the next.
		CharData advances[94];
		// Whether the character is included in the font.
		bool masks[94];
		unsigned width  = 0;
		unsigned height = 0;

		static unsigned VBO;
		static unsigned VAO;
	};
}
