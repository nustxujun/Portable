#pragma once

#include "Common.h"
#include "Mouse.h"
#include "Keyboard.h"

class Input
{
public:
	using Keyboard = DirectX::Keyboard::State;
	using Mouse = DirectX::Mouse::State;
	using Ptr = std::shared_ptr<Input>;

	using Listener = std::function<bool(const Mouse &, const Keyboard &)>;

public:
	Input();
	~Input();

	void update();

	void listen(Listener lis, int pri = 3) { mListeners[pri].push_back(lis); }
private:
	std::unique_ptr<DirectX::Keyboard> mKeyboard;
	std::unique_ptr<DirectX::Mouse> mMouse;

	std::map<int,std::vector<Listener>> mListeners;
};