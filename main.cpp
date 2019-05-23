// deferredRendering.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "deferredRendering.h"
#include <memory>


#include <sstream>

#include "Framework.h"
#include "Observer.h"
#include "MultipleLights.h"
#include "Network.h"
#include "Setting.h"

#include <functional>
#define MAX_LOADSTRING 100


std::shared_ptr<Framework> framework;

std::string show;

std::shared_ptr<Network> network;

class Relay: public Setting::Modifier
{
public:
	Relay()
	{
		network->setListener([=](const char* b, size_t s) {
			std::string str(b, s);
			json j = json::parse(str);

			std::string type = j["type"];
			if ( type == "set")
			{
				setValue(j["key"], j["value"]);
			}
		});
	}

	void onChanged(const std::string& key, const nlohmann::json::value_type& value)
	{
		json j = value;

		if (j["type"] == "set")
			j["key"] = key;
		network->send(j);
	}
};

std::shared_ptr<Relay> relay;

void tryConnect(const asio::error_code& err)
{
	if (err)
	{
		network->connect("127.0.0.1", 8888, tryConnect);
	}
	else
	{
		json j = { {"type", "init"}, {"msg", u8"连接成功!"} };
		network->send(j);
		framework->init();

	}
};

void initNetwork()
{
	network = std::make_shared<Network>();


	network->connect("127.0.0.1", 8888, tryConnect);

}

void initRenderer(HWND win)
{
	framework = std::shared_ptr<Framework>(new MultipleLights(win));
	//framework = std::shared_ptr<Framework>(new Observer(win));
	//framework = std::make_shared<Framework>(win);

	relay = std::make_shared<Relay>();
	relay->setSetting(framework->getSetting());
}

void framemove()
{
	//json jo;
	//jo["message"] = framework->getFPS();
	//network->send(jo);
	network->update();
	framework->update();
	//input->update();
	//auto light = scene->createOrGetLight("main");
	//float len = 300;
	//auto t = GetTickCount() * 0.0002f;
	//auto x = cos(t) * len;
	//auto y = sin(t) * len;
	//
	////light->setDirection({x,y,x });
	//

	static auto time = GetTickCount();
	//static size_t count = 0;
	//count++;

	//if (count == 100)
	//{
	//	float fps = count * 1000.0f / std::max((unsigned long)1,(GetTickCount() - time));
	//	time = GetTickCount();
	//	count = 0;
	//	std::stringstream ss;

	//	ss << fps;
	//	show = ss.str();
	//}


	//renderer->setRenderTarget(bb);
	//auto font = renderer->createOrGetFont(L"myfile.spritefont");
	//font.lock()->drawText(show.c_str(), Vector2(10, 10));
	//renderer->present();
}


// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DEFERREDRENDERING, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DEFERREDRENDERING));

	MSG msg = {};

    // 主消息循环:
	while (WM_QUIT != msg.message)
	{
		//if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			framemove();
		}
	}


    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DEFERREDRENDERING));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DEFERREDRENDERING);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}




//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   initNetwork();
   initRenderer(hWnd);
   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
		
		DirectX::Mouse::ProcessMessage(message, wParam, lParam);
		DirectX::Keyboard::ProcessMessage(message, wParam, lParam);

        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
