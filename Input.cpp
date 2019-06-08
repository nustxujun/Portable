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
	for (auto& vec : mListeners)
	{
		for (auto& i : vec.second)
		{
			if (i(mousestate, keystate))
				return;
		}
	}
}


