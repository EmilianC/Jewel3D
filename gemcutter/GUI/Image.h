// Copyright (c) 2021 Emilian Cioca
#pragma once
#include "gemcutter/GUI/Widget.h"
#include "gemcutter/Resource/Texture.h"

namespace gem
{
	//
	class Image : public Widget
	{
	public:
		Image(Entity& owner);
		Image(Entity& owner, Texture::Ptr tex);

		void SetTexture(Texture::Ptr tex);
		Texture::Ptr GetTexture();

	private:
		void UpdateContent() override;

		Texture::Ptr texture;
	};
}
