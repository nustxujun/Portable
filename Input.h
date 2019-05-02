#pragma once

#include "Common.h"
#include "Mouse.h"
#include "Keyboard.h"

class Input
{
public:
	using Keyboard = DirectX::Keyboard::State;
	using Mouse = DirectX::Mouse::State;

	using Listener = std::function<void(const Mouse &, const Keyboard &)>;

public:
	Input();
	~Input();

	void update();

	void listen(Listener lis) { mListeners.push_back(lis); }
private:
	std::unique_ptr<DirectX::Keyboard> mKeyboard;
	std::unique_ptr<DirectX::Mouse> mMouse;

	std::vector<Listener> mListeners;
};