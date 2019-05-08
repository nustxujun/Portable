﻿// deferredRendering.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "deferredRendering.h"
#include "renderer.h"
#include <memory>

#include "Quad.h"
#include "Mesh.h"
#include "Scene.h"

#include "DeferredRenderer.h"
#include "Input.h"

#include "ShadowMap.h"
#include "Quad.h"


#include <sstream>

#define MAX_LOADSTRING 100


template<class T>
class Instance
{
public:
	using Shared = std::shared_ptr<T>;

	template<class ... Args>
	void operator()(Args ... args)
	{
		instance = Shared(new T(args...));
	}

	Shared operator->()const { return instance; }
	operator Shared()const { return instance; }

	Shared instance;
};


Renderer::Ptr renderer;
Instance<Quad> quad;


std::shared_ptr<Scene> scene;
std::shared_ptr< DeferredRenderer> deferred;
std::shared_ptr<Input> input;
std::shared_ptr<ShadowMap> sm;

std::string show;



void initRenderer(HWND win)
{

	RECT rect;
	GetClientRect(win, &rect);
	renderer = std::move(std::unique_ptr<Renderer>(new Renderer));
	renderer->init(win, rect.right, rect.bottom);

	quad(renderer);

	//test = decltype(test)(new Test(renderer));
	scene = decltype(scene)(new Scene(renderer));
	deferred = decltype(deferred)(new DeferredRenderer(renderer, scene));
	input = decltype(input)(new Input());
	sm = decltype(sm)(new ShadowMap(renderer, scene,quad));

	Parameters params;
	params["file"] = "sponza/sponza.obj";
	auto model = scene->createModel("test", params);
	model->attach(scene->getRoot());
	model->getNode()->setPosition(0.0f, 0.f, 0.0f);
	auto aabb = model->getWorldAABB();


	auto cam = scene->createOrGetCamera("main");
	DirectX::XMFLOAT3 eye(aabb.second);
	DirectX::XMFLOAT3 at(aabb.first);
	cam->lookat(eye, at);
	cam->setViewport(0, 0, rect.right, rect.bottom);
	cam->setNearFar(1, 10000);
	cam->setFOVy(0.785398185);

	Vector3 vec = aabb.second - aabb.first;
	float com_step= std::min(std::min(vec.x, vec.y), vec.z) * 0.001;

	auto light = scene->createOrGetLight("main");
	light->setDirection({ 0,-1,0.1 });

	input->listen([cam, com_step](const Input::Mouse& m, const Input::Keyboard& k) {
		static auto lasttime = GetTickCount();
		auto dtime = GetTickCount() - lasttime;
		lasttime = GetTickCount();
		float step = dtime * com_step;
		
		auto node = cam->getNode();
		auto pos = node->getPosition();
		auto dir = cam->getDirection();
		Vector3 up(0, 1, 0);
		Vector3 r = up.Cross(dir);
		r.Normalize();

		Vector2 cur = Vector2(m.x, m.y);
		static Vector2 last = cur;
		Vector2 d = cur - last;



		d = Vector2(d.x  , d.y );
		last = cur;
		if (k.W )
		{
			pos = dir * step + pos;
		}
		else if (k.S)
		{
			pos = dir * -step + pos;
		}
		else if (k.A)
		{
			pos = r * -step + pos;
		}
		else if (k.D)
		{
			pos = r * step + pos;
		}

		if (m.leftButton)
		{
			float pi = 3.14159265358;
			Quaternion rot = Quaternion::CreateFromYawPitchRoll(d.x * 0.005, d.y  * 0.005, 0);

			rot *= node->getOrientation();
			dir = Vector3::Transform(Vector3(0, 0, 1), rot);
			cam->setDirection(dir);
		}

		node->setPosition(pos);
	});


}

void framemove()
{


	input->update();
	auto light = scene->createOrGetLight("main");
	float len = 300;
	auto t = GetTickCount() * 0.0005f;
	auto x = cos(t) * len;
	auto y = sin(t) * len;
	
	//light->setDirection({0,y,x });
	deferred->render();

	static auto time = GetTickCount();
	static size_t count = 0;
	count++;

	if (count == 100)
	{
		float fps = count * 1000.0f / std::max((unsigned long)1,(GetTickCount() - time));
		time = GetTickCount();
		count = 0;
		std::stringstream ss;

		ss << fps;
		show = ss.str();
	}


	sm->render();

	auto bb = renderer->getBackbuffer();
	renderer->setRenderTarget(bb);
	auto font = renderer->createOrGetFont(L"myfile.spritefont");
	font.lock()->drawText(show.c_str(), Vector2(10, 10));
	renderer->present();
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
		renderer->uninit();
		renderer.reset();
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
