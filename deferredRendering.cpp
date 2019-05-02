// deferredRendering.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "deferredRendering.h"
#include "renderer.h"
#include <memory>

#include "Quad.h"
#include "Mesh.h"
#include "Scene.h"
#include "test.h"

#include "DeferredRenderer.h"
#include "Input.h"



#define MAX_LOADSTRING 100

Renderer::Ptr renderer;
std::shared_ptr<Quad> quad;
std::shared_ptr<Scene> scene;
std::shared_ptr<Test> test;
std::shared_ptr< DeferredRenderer> deferred;
std::shared_ptr<Input> input;
void initRenderer(HWND win)
{
	RECT rect;
	GetClientRect(win, &rect);
	renderer = std::move(std::unique_ptr<Renderer>(new Renderer));
	renderer->init(win, rect.right, rect.bottom);


	//test = decltype(test)(new Test(renderer));
	quad =  decltype(quad)(new Quad(renderer));
	scene = decltype(scene)(new Scene(renderer));
	deferred = decltype(deferred)(new DeferredRenderer(renderer, scene));
	input = decltype(input)(new Input());

	Parameters params;
	params["file"] = "tiny.x";
	auto model = scene->createModel("test", params);
	model->attach(scene->getRoot());
	model->getNode()->setPosition(0.0f, 0.0f, 0.0f);

	auto cam = scene->createOrGetCamera("main");
	DirectX::XMFLOAT3 eye(0, 0, -300);
	DirectX::XMFLOAT3 at(0, 0, 0);
	cam->lookat(eye, at);
	cam->setViewport(0, 0, rect.right, rect.bottom);
	cam->setNearFar(0.01, 1000);

	input->listen([cam](const Input::Mouse& m, const Input::Keyboard& k) {
		auto node = cam->getNode();
		if (k.W)
		{
			auto pos = node->getPosition();
			node->setPosition(pos.x, pos.y, pos.z + 1);
		}
	
	});


	//cam->setProjectType(Scene::Camera::PT_ORTHOGRAPHIC);
}

void framemove()
{
	input->update();
	/*auto cam = scene->createOrGetCamera("main");
	float len = 300;
	auto time = GetTickCount() * 0.0005f;
	auto x = cos(time) * len;
	auto z = sin(time) * len;
	cam->lookat(DirectX::XMFLOAT3(x,0,z), DirectX::XMFLOAT3(0,0,0));*/
	deferred->render();
	//std::vector<ID3D11ShaderResourceView*> srvs;
	//gbuffer->render(scene);
	//lighting->prepare(scene->createOrGetCamera("main"));
	//srvs.push_back(gbuffer->getDiffuse().lock()->getShaderResourceView());
	//srvs.push_back(gbuffer->getNormal().lock()->getShaderResourceView());
	//srvs.push_back(gbuffer->getDepth().lock()->getShaderResourceView());

	//quad->draw(lighting->getRenderTarget(), srvs, lighting->getPS());
	//srvs.clear();
	//srvs.push_back(lighting->getRenderTarget().lock()->getShaderResourceView());

	//quad->draw(renderer->getBackbuffer(), srvs, Renderer::PixelShader::Weak());

	//test->draw(nullptr);
	//quad->draw(test->mRenderTarget.lock()->getShaderResourceView());
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
