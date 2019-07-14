#include "Overlay.h"

Overlay::Overlay()
{
}

void Overlay::setInput(Input::Ptr input)
{
	auto l = std::bind(&Overlay::handleEvent, this, std::placeholders::_1, std::placeholders::_2);
	input->listen(l, 2);
}

void Overlay::setRenderer(Renderer::Ptr r)
{
	mRenderer = r;
}

void Overlay::update()
{
	render();
}

