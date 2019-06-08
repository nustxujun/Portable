#pragma once
#include "Input.h"
#include "renderer.h"
class Overlay
{
public:
	using Ptr = std::shared_ptr<Overlay>;

	Overlay();
	virtual ~Overlay() {};

	using Keyboard = DirectX::Keyboard::State;
	using Mouse = DirectX::Mouse::State;

	void setInput(Input::Ptr input);
	void setRenderer(Renderer::Ptr input);
	virtual bool handleEvent(const Input::Mouse& m, const Input::Keyboard& k) = 0;
	virtual void render() = 0;
protected:
	Renderer::Ptr mRenderer;
};