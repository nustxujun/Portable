
#include <Windows.h>
#include "MultipleLights.h"
#include <thread>


#ifdef _WINDLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

using Ptr = std::shared_ptr<MultipleLights>;
Ptr framework;
std::shared_ptr<std::thread> loop;
int state = 0;
std::vector<char> overlaydata(400 * 800 * 4);

LRESULT CALLBACK process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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

HWND createWindow()
{
	HWND hWnd = CreateWindowW(windowClass, L"", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, instance, nullptr);

	ShowWindow(hWnd, true);
	UpdateWindow(hWnd);
	return hWnd;
}

class ElectronOverlay :public Overlay
{
public:
	using Ptr = std::shared_ptr<ElectronOverlay>;

	Renderer::Texture2D::Ptr mTarget;
	Quad::Ptr mQuad;
	void init(int width, int height)
	{
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

	bool handleEvent(const Input::Mouse& m, const Input::Keyboard& k)
	{
		auto desc = mTarget.lock()->getDesc();

		if (m.x >= 0 && m.x <= desc.Width &&
			m.y >= 0 && m.y <= desc.Height)
		{

			return true;
		}
		else
			return false;
		
	}
	void render()
	{
		mQuad->setDefaultBlend(false);
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

	void refresh(const char* data, size_t size)
	{
		mTarget.lock()->blit(data, size);
	}
};

ElectronOverlay::Ptr overlay;

extern "C"
{
	EXPORT void init(HWND overlayhandle)
	{
		::MessageBoxA(NULL, "start", NULL, NULL);
		::SetCurrentDirectoryA("D:/workspace/Portable");

		RECT rect;
		GetClientRect(overlayhandle, &rect);
		overlaydata.resize(rect.right * rect.bottom * 4);

		loop = std::shared_ptr<std::thread>(new std::thread([rect]()
		{
			registerWindow();
			auto win = createWindow();
			framework = Ptr(new MultipleLights(win));
			framework->init();
			overlay = ElectronOverlay::Ptr(new ElectronOverlay());
			framework->setOverlay(overlay);
			overlay->init(rect.right, rect.bottom);
			MSG msg = {};
			while (WM_QUIT != msg.message)
			{
				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{

					if (state == 1)
					{
						state = 0;
						overlay->refresh(overlaydata.data(), overlaydata.size());
					}
					framework->update();
				}
			}
		}));
	}

	EXPORT void paint(char* data, size_t size)
	{
		memcpy(overlaydata.data(), data, std::min(size,overlaydata.size()));
		state = 1;
	}

}

#ifndef _WINDLL
int main()
{
	init();
	loop->join();
	return 0;
}
#endif