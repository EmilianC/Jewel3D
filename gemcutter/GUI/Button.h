// Copyright (c) 2021 Emilian Cioca
#pragma once
#include "gemcutter/Application/Delegate.h"
#include "gemcutter/GUI/Widget.h"
#include "gemcutter/Resource/Sound.h"
#include "gemcutter/Resource/Texture.h"
#include "gemcutter/Resource/Font.h"

#include <string_view>
#include <string>

namespace gem
{
	//
	class Button : public Widget
	{
	public:
		Button(Entity& owner);
		Button(Entity& owner, Font::Ptr font, std::string_view text);
		Button(Entity& owner, Texture::Ptr idle, Texture::Ptr hover, Texture::Ptr pressed);
		Button(Entity& owner, Font::Ptr font, std::string_view text, Texture::Ptr idle, Texture::Ptr hover, Texture::Ptr pressed);

		enum State
		{
			Idle,
			Hover,
			Pressed
		};

		State GetState() const { return state; }

		void Update();

		void SetFont(Font::Ptr font);
		Font::Ptr GetFont() const;

		void SetText(std::string_view string);
		const std::string& GetText() const;

		Sound::Ptr pressedSound;
		Texture::Ptr idleTexture;
		Texture::Ptr hoverTexture;
		Texture::Ptr pressedTexture;
		float textScale = 1.0f;

		Dispatcher<void(Button&)> onClick;

	private:
		void UpdateContent() override;

		void SetState(State);
		void SetTexture(State);

		State state = State::Idle;
		Entity::Ptr imageEntity;
		Entity::Ptr textEntity;
		float textWidth = 0.0f;
	};
}
