
#include <Windows.h>

#include <thread>
#include "json.hpp"
#include <sstream>



#include "MultipleLights.h"
#include "PBRMaterial.h"
#include "Reflections.h"
#include "SHSample.h"
#include "VoxelConeTracing.h"
#include "SubsurfaceScattering.h"
#include "ImGuiOverlay.h"

using namespace nlohmann;

#ifdef _WINDLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

using FRAMEWORK = PBRMaterial;


using Ptr = std::shared_ptr<FRAMEWORK>;
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

class Watcher : public Setting::Modifier
{
public:
	void onChanged(const std::string& key, const nlohmann::json::value_type& value)
	{
		static ImguiWindow* panel = (ImguiWindow*)ImguiObject::root()->createChild<ImguiWindow>("panel");
		panel->setSize(300, 400);
		json j = value;

		if (j["type"] == "set" || j["type"] == "stage")
			j["key"] = key;

		if (j["type"] == "stage")
		{
			static std::map<std::string, ImguiObject*> stages;
			if (stages.find(key) == stages.end())
			{
				stages[key] = panel->createChild<ImguiText>();
			}
			std::string cost = j["cost"];

			((ImguiText*)stages[key])->text = key + ": " + cost + "ms";
		}
		if (j["type"] == "set")
		{
			static std::map<std::string, ImguiSlider*> sliders;
			float v = j["value"];
			if (sliders.find(key) == sliders.end())
			{
				float vmin = j["min"];
				float vmax = j["max"];
				sliders[key] = panel->createChild<ImguiSlider>(key.c_str(), v, vmin, vmax,
					[this, key](ImguiSlider* slider)
					{
						this->setValue(key, slider->value);
					});
			}
			sliders[key]->value = v;
		}
	}


};

ImguiOverlay::Ptr overlay;

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
			try
			{
				registerWindow();
				auto win = createWindow(width, height);
				framework = Ptr(new FRAMEWORK(win));
	

				Watcher watcher;
				watcher.setSetting(framework->getSetting());
				framework->init();

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
						framework->update();

					}
				}
			}
			catch (std::exception& e)
			{
				MessageBoxA(NULL, e.what(), NULL, NULL);
				throw;
			}


			framework = nullptr;
		}));
	}

	EXPORT void paint(char* data, size_t size)
	{

	}

	EXPORT void setCallback(Callback cb)
	{
		electronCallback = cb;
	}

	EXPORT void handleEvent(char* data, size_t size)
	{
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
	try {
		init(1600, 900, 1, 1, 0);
		loop->join();
	}
	catch (std::exception& e)
	{
		MessageBoxA(NULL, e.what(), NULL, NULL);
		throw;
	}
	return 0;
}
#endif