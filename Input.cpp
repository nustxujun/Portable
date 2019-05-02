#include "Input.h"

Input::Input()
{
	mKeyboard = std::make_unique<DirectX::Keyboard>();
	mMouse = std::make_unique<DirectX::Mouse>();
}

Input::~Input()
{
}

void Input::update()
{
	auto keystate = mKeyboard->GetState();
	auto mousestate = mMouse->GetState();
	for (auto& i : mListeners)
	{
		i(mousestate, keystate);
	}
}


