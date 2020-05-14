// Copyright (c) 2021 Emilian Cioca
#include "Button.h"

#include "gemcutter/Application/HierarchicalEvent.h"
#include "gemcutter/GUI/Image.h"
#include "gemcutter/Input/Input.h"
#include "gemcutter/Rendering/Text.h"
#include "gemcutter/Resource/Material.h"
#include "gemcutter/Sound/SoundSource.h"

namespace gem
{
	Button::Button(Entity& _owner)
		: Widget(_owner)
	{
		auto& onKeyPressed  = owner.Require<HierarchicalListener<KeyPressed>>();
		auto& onKeyReleased = owner.Require<HierarchicalListener<KeyReleased>>();
		owner.Require<SoundSource>();

		onKeyPressed.callback.Bind(owner.GetWeakPtr(), [this](const KeyPressed& e) {
			if (state == State::Hover && e.key == Key::MouseLeft)
			{
				SetState(State::Pressed);
				return true;
			}

			return false;
		});

		onKeyReleased.callback.Bind(owner.GetWeakPtr(), [this](const KeyReleased& e) {
			if (state == State::Pressed && e.key == Key::MouseLeft)
			{
				const Rectangle& bounds = GetAbsoluteBounds();
				vec2 mousePos = Input.GetMousePos();
				if (!bounds.IsPointInside(mousePos.x, mousePos.y))
				{
					SetState(Idle);
					return true;
				}

				onClick.Dispatch(*this);

				if (pressedSound)
				{
					auto& soundSource = owner.Get<SoundSource>();
					soundSource.SetData(pressedSound);
					soundSource.Play();
				}

				SetState(State::Hover);

				return true;
			}

			return false;
		});

		auto& hierarchy = owner.Get<Hierarchy>();

		imageEntity = hierarchy.CreateChild();
		imageEntity->Add<Image>();

		textEntity = hierarchy.CreateChild();

		auto material = Material::MakeNew();
		material->shader = Load<Shader>("Shaders/Default/Font");
		material->blendMode = BlendFunc::Linear;

		auto& textComponent = textEntity->Add<Text>(std::move(material));
		textComponent.centeredX = true;
		textComponent.centeredY = true;
	}

	Button::Button(Entity& _owner, Font::Ptr _font, std::string_view _text)
		: Button(_owner)
	{
		SetFont(std::move(_font));
		SetText(_text);
	}

	Button::Button(Entity& _owner, Texture::Ptr idle, Texture::Ptr hover, Texture::Ptr pressed)
		: Button(_owner)
	{
		idleTexture = std::move(idle);
		hoverTexture = std::move(hover);
		pressedTexture = std::move(pressed);

		SetTexture(State::Idle);
	}

	Button::Button(Entity& _owner, Font::Ptr _font, std::string_view _text, Texture::Ptr idle, Texture::Ptr hover, Texture::Ptr pressed)
		: Button(_owner)
	{
		SetFont(std::move(_font));
		SetText(_text);

		idleTexture = std::move(idle);
		hoverTexture = std::move(hover);
		pressedTexture = std::move(pressed);

		SetTexture(State::Idle);
	}

	void Button::Update()
	{
		const Rectangle& bounds = GetAbsoluteBounds();
		const vec2 mousePos = Input.GetMousePos();
		const bool isInside = bounds.IsPointInside(mousePos.x, mousePos.y);

		if (state == State::Pressed)
		{
			SetTexture(isInside ? State::Pressed : State::Hover);
		}
		else
		{
			if (isInside && !Input.IsDown(Key::MouseLeft))
			{
				SetState(State::Hover);
			}
			else
			{
				SetState(State::Idle);
			}
		}
	}

	void Button::SetFont(Font::Ptr font)
	{
		textEntity->Get<Text>().font = std::move(font);
	}

	Font::Ptr Button::GetFont() const
	{
		return textEntity->Get<Text>().font;
	}

	void Button::SetText(std::string_view string)
	{
		auto& textComponent = textEntity->Get<Text>();
		textComponent.text = string;

		textWidth = textComponent.GetLineWidth(1);
	}

	const std::string& Button::GetText() const
	{
		return textEntity->Get<Text>().text;
	}

	void Button::UpdateContent()
	{
		textEntity->position = owner.position;
		textEntity->rotation = owner.rotation;

		const float widgetWidth = GetAbsoluteBounds().width;

		if (textWidth > widgetWidth)
		{
			const float scalar = (widgetWidth / textWidth) * textScale;

			textEntity->scale = vec3(scalar);
		}
		else
		{
			textEntity->scale = vec3(textScale);
		}
	}

	void Button::SetState(State newState)
	{
		if (state == newState)
		{
			return;
		}

		state = newState;
		SetTexture(state);
	}

	void Button::SetTexture(State _state)
	{
		switch(_state)
		{
			case State::Idle:
			imageEntity->Get<Image>().SetTexture(idleTexture);
			break;

			case State::Hover:
			imageEntity->Get<Image>().SetTexture(hoverTexture);
			break;

			case State::Pressed:
			imageEntity->Get<Image>().SetTexture(pressedTexture);
			break;
		}
	}
}
