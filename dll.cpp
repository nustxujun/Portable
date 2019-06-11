
#include <Windows.h>
#include "MultipleLights.h"
#include <thread>
#include "json.hpp"
#include <sstream>

using namespace nlohmann;

#ifdef _WINDLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

using Ptr = std::shared_ptr<MultipleLights>;
Ptr framework;
std::shared_ptr<std::thread> loop;
HWND parentWnd;

using Callback = void(*)(const char*, size_t );
Callback electronCallback;

void callElectron(const json& j)
{
	if (electronCallback)
	{
		auto str = j.dump();
		electronCallback(str.c_str(), str.size());
	}
}

LRESULT CALLBACK process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
	case WM_DESTROY:
		{	
#ifdef _WINDLL
			electronCallback = nullptr;
			PostMessage(parentWnd, WM_CLOSE, 0, 0);
#else
			PostMessage(hWnd, WM_CLOSE, 0, 0);
#endif
		}
		return 0;
	}
	DirectX::Mouse::ProcessMessage(message, wParam, lParam);
	DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
	return DefWindowProc(hWnd, message, wParam, lParam);
}

WCHAR windowClass[] = L"window";
HINSTANCE instance = ::GetModuleHandle(NULL);
void registerWindow()
{
	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = process;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = instance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = windowClass;
	wcex.hIconSm = NULL;

	RegisterClassExW(&wcex);
}

HWND createWindow(int width, int height)
{
	HWND hWnd = CreateWindowW(windowClass, L"", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, width, height, nullptr, nullptr, instance, nullptr);



	RECT win, client;
	::GetClientRect(hWnd, &client);
	::GetWindowRect(hWnd, &win);

	auto w = win.right - win.left - client.right;
	auto h = win.bottom  - win.top - client.bottom;

	::MoveWindow(hWnd, win.left, win.top, w + width, h + height, FALSE);

	ShowWindow(hWnd, true);
	UpdateWindow(hWnd);


	return hWnd;
}

class ElectronOverlay :public Overlay, public Setting::Modifier
{
public:
	using Ptr = std::shared_ptr<ElectronOverlay>;
	int state = 0;
	std::vector<char> overlaydata;
	Renderer::Texture2D::Ptr mTarget;
	Quad::Ptr mQuad;
	HWND window;
	bool visible = true;

	void init(int width, int height)
	{
		overlaydata.resize(width * height * 4);
		D3D11_TEXTURE2D_DESC desc = {0};
		desc.Width = width;
		desc.Height = height;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.ArraySize = 1;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		mTarget = mRenderer->createTexture(desc);
		mQuad = Quad::Ptr(new Quad(mRenderer));
	}

	bool isKeyUp(const Input::Keyboard& k, DirectX::Keyboard::Keys key)
	{
		static Input::Keyboard lastState = k;
		if (lastState.IsKeyDown(key) && k.IsKeyUp(key))
		{
			lastState = k;
			return true;
		}

		lastState = k;
		return false;
	}

	bool handleEvent(const Input::Mouse& m, const Input::Keyboard& k)
	{
		if (isKeyUp(k, DirectX::Keyboard::Tab))
			visible = !visible;

		auto desc = mTarget.lock()->getDesc();
		
		if (m.x >= 0 && m.x <= desc.Width &&
			m.y >= 0 && m.y <= desc.Height && visible)
		{
			static Input::Mouse lastState = m;
			if (lastState.leftButton != m.leftButton)
			{
				json e;
				e["mouse"]["x"] = m.x;
				e["mouse"]["y"] = m.y;
				e["mouse"]["modifiers"] = { "left" };
				e["mouse"]["type"] = m.leftButton?"mouseDown": "mouseUp";
				callElectron(e);
			}

			if (lastState.rightButton != m.rightButton)
			{
				json e;
				e["mouse"]["x"] = m.x;
				e["mouse"]["y"] = m.y;
				e["mouse"]["modifiers"] = { "left" };
				e["mouse"]["type"] = m.rightButton ? "mouseDown" : "mouseUp";
				callElectron(e);
			}

			if (lastState.x != m.x || lastState.y != m.y)
			{
				json e;
				e["mouse"]["x"] = m.x;
				e["mouse"]["y"] = m.y;
				e["mouse"]["type"] = "mouseMove";
				callElectron(e);
			}


			lastState = m;
			return true;
		}
		else
		{
			json e;
			e["mouse"]["x"] = m.x;
			e["mouse"]["y"] = m.y;
			e["mouse"]["modifiers"] = { "left","right" };
			e["mouse"]["type"] = "mouseUp";
			callElectron(e);

			return false;
		}
	}
	void render()
	{
		if (!visible)
			return;
		refresh();
		mQuad->setDefaultBlend(true);
		mQuad->setDefaultPixelShader();
		mQuad->setDefaultSampler();

		auto desc = mTarget.lock()->getDesc();

		D3D11_VIEWPORT vp;
		vp.Width = desc.Width;
		vp.Height = desc.Height;
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		mQuad->setViewport(vp);
		mQuad->setTextures({ mTarget });
		mQuad->setRenderTarget(mRenderer->getBackbuffer());
		mQuad->draw();

	}

	void refresh()
	{
		if (state == 1)
		{
			state = 0;
			mTarget.lock()->blit(overlaydata.data(), overlaydata.size());
		}
	}

	void onChanged(const std::string& key, const nlohmann::json::value_type& value)
	{
		json j = value;

		if (j["type"] == "set" || j["type"] == "stage")
			j["key"] = key;
		callElectron(j);
	}

	void receive(const char* s, size_t size)
	{
		std::string str(s, size);
		json j = json::parse(str);

		std::string type = j["type"];
		if (type == "set")
		{
			setValue(j["key"], j["value"]);
		}
	}
};

ElectronOverlay::Ptr overlay;

extern "C"
{
	EXPORT void init(int width, int height ,   int overlayWidth, int overlayHeight, HWND overlayhandle)
	{
		parentWnd = overlayhandle;
#ifdef _WINDLL
		::MessageBoxA(NULL, "start", NULL, NULL);
		::SetCurrentDirectoryA("../");
#endif

		loop = std::shared_ptr<std::thread>(new std::thread([width, height, overlayWidth, overlayHeight]()
		{
			registerWindow();
			auto win = createWindow(width, height);
			framework = Ptr(new MultipleLights(win));
			overlay = ElectronOverlay::Ptr(new ElectronOverlay());
			overlay->setSetting(framework->getSetting());
			overlay->window = win;
			framework->init();
			framework->setOverlay(overlay);
			overlay->init(overlayWidth, overlayHeight);
			MSG msg = {};
			while (WM_QUIT != msg.message && WM_CLOSE != msg.message
#ifdef _WINDLL
				&& electronCallback
#endif
				)
			{
				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{
					overlay->refresh();
					framework->update();
					std::stringstream ss;
					ss.precision(4);
					ss << framework->getFPS();
					SetWindowTextA(overlay->window, ss.str().c_str());
				}
			}


			framework = nullptr;
			overlay = nullptr;
		}));
	}

	EXPORT void paint(char* data, size_t size)
	{
		if (!overlay) return;
		memcpy(overlay->overlaydata.data(), data, std::min(size, overlay->overlaydata.size()));
		overlay->state = 1;
	}

	EXPORT void setCallback(Callback cb)
	{
		electronCallback = cb;
	}

	EXPORT void handleEvent(char* data, size_t size)
	{
		overlay->receive(data, size);
	}

	EXPORT void close()
	{
		electronCallback = nullptr;
		loop->join();
	}


}

#ifndef _WINDLL
int main()
{
	init(1600,800,1,1,0);
	loop->join();
	return 0;
}
#endif