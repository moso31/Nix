#include "BaseDefs/NixCore.h"
#include "BaseDefs/CrtDbg.h"
#include "BaseDefs/DearImGui.h"
#include "NXGlobalDefinitions.h"

#include "App.h"
#include "NXInput.h"
#include "NXTimer.h"
#include "NXGUI.h"
#include "NXLog.h"

//#define ENABLE_SPLASH_SCREEN

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SplashWndProc(HWND, UINT, WPARAM, LPARAM);

void PaintNixLogo(HWND hWnd, int count, HBRUSH hBrushes[], UINT nBrushesSize)
{
	const static UINT latticeCoordsX[] = { 
		180, 220, 260, 300,     380,      460,			 580,
		180,           300,     380,           500,
		180,           300,     380,				540,
		180,           300,     380,      460,			 580,
	};
	const static UINT latticeCoordsY[] = { 
		120, 120, 120, 120,     120,      120,			 120, 
		160,		   160,		160,           160,
		200,		   200,		200,				200,
		240,		   240,		240,      240,			 240,
	};

	HDC hdc = GetDC(hWnd);
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);

	int arrSize = ARRAYSIZE(latticeCoordsX);
	int round = (count / arrSize) % nBrushesSize;
	int index = count % arrSize;
	RECT rc = { (LONG)latticeCoordsX[index], (LONG)latticeCoordsY[index], (LONG)latticeCoordsX[index] + 40, (LONG)latticeCoordsY[index] + 40 };

	// 然后刷上新的颜色
	FillRect(hdc, &rc, hBrushes[round]);

	ReleaseDC(hWnd, hdc);
}

DWORD WINAPI SplashScreenThread(LPVOID lpParam) 
{
	// 预加载笔刷
	HBRUSH hBrushes[] = {
		CreateSolidBrush(RGB(0, 128, 255)),
		CreateSolidBrush(RGB(128, 220, 0)),
		CreateSolidBrush(RGB(220, 0, 128)),
		CreateSolidBrush(RGB(255, 255, 255)),
	};
	UINT nBrushesSize = ARRAYSIZE(hBrushes);

	HANDLE exitEvent = static_cast<HANDLE>(lpParam);

	HINSTANCE hInstance = GetModuleHandle(nullptr);

	// 获取屏幕宽度和高度
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	int windowWidth = 800;
	int windowHeight = 400;
	int posX = (screenWidth - windowWidth) / 2;
	int posY = (screenHeight - windowHeight) / 2;

	// 创建闪屏窗口
	HWND hWnd = CreateWindow(L"SplashScreenClass", L"Splash Screen", WS_POPUP | WS_VISIBLE,
		posX, posY, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

	// 检查 hWnd 是否为空
	if (!hWnd) {
		DWORD error = GetLastError();
		return 0;
	}

	// 显示闪屏
	ShowWindow(hWnd, SW_SHOWDEFAULT);  // 摸鱼时记得将 SW_SHOWDEFAULT 换成 SW_HIDE (._.!!!)
	UpdateWindow(hWnd);

	int count = 0;
	MSG msg;
	while (true) 
	{
		// 使用PeekMessage处理消息，以便在没有消息时检查退出事件
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// 检查退出事件是否被设置
		if (WaitForSingleObject(exitEvent, 0) == WAIT_OBJECT_0) 
		{
			break;
		}

		// 在此处更新闪屏
		PaintNixLogo(hWnd, count++, hBrushes, nBrushesSize);

		// 限制闪屏线程的CPU占用
		Sleep(60);
	}

	// 销毁闪屏窗口
	DestroyWindow(hWnd);

	for (int i = 0; i < ARRAYSIZE(hBrushes); ++i)
		DeleteObject(hBrushes[i]);

	return 0;
}

HRESULT InitSplashWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = SplashWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, L"");
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"SplashScreenClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, 0);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	return S_OK;
}

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	AllocConsole();

	InitSplashWindow(hInstance);

	FILE* fp = 0;
	freopen_s(&fp, "CONOUT$", "w", stdout);

	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, L"");
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"NixWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, 0);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	NXGlobalWindows::hInst = hInstance;
	LONG offset = 50;
	RECT rc = { 0 + offset, 0 + offset, 1600 + offset, 900 + offset };
	//RECT rc = { 0, 0, 120, 90 };

	NXGlobalWindows::hWnd = CreateWindow(L"NixWindowClass", L"Nix",
		WS_OVERLAPPEDWINDOW,
		0, 0, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);

	//SetWindowLong(NXGlobalWindows::hWnd, GWL_STYLE, 5);
	ShowWindow(NXGlobalWindows::hWnd, SW_HIDE);

	return S_OK;
}

#include "NXFileSystemHelper.h"
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// 加载应用程序的核心组件和资源
	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

#ifdef ENABLE_SPLASH_SCREEN
	// 创建一个事件，用于通知闪屏线程退出
	HANDLE exitEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	// 创建并启动闪屏线程
	std::thread splashThread(SplashScreenThread, exitEvent);
#endif

	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	NXLog::Init();
	NXLog::Log("-----Init-----");

	NXGlobalApp::App = new App();
	NXGlobalApp::App->Init();

	NXGlobalApp::Timer = new NXTimer();

#ifdef ENABLE_SPLASH_SCREEN
	// 加载完成后，通知闪屏线程退出
	SetEvent(exitEvent);

	// 等待闪屏线程结束
	splashThread.join();
#endif

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		//NXInput::GetInstance()->Update();

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			NXGlobalApp::Timer->Tick();

			NXGlobalApp::App->FrameBegin();
			NXGlobalApp::App->ResizeCheck();

			NXGlobalApp::App->Reload();

			if (!IsWindowVisible(NXGlobalWindows::hWnd))
			{
				ShowWindow(NXGlobalWindows::hWnd, SW_SHOWMAXIMIZED);
				SetForegroundWindow(NXGlobalWindows::hWnd);
			}

			NXGlobalApp::App->Update();
			NXGlobalApp::App->Draw();

			NXInput::GetInstance()->RestoreData(); // 清空鼠标/键盘的激活状态
		}
	}

	NXLog::Log("-----Release-----");

	SafeDelete(NXGlobalApp::Timer);
	SafeRelease(NXGlobalApp::App);

	NXLog::Release();

	CoUninitialize();
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_SIZE:
		if (NXGlobalApp::App) NXGlobalApp::App->OnWindowResize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_INPUT:
		NXInput::GetInstance()->UpdateRawInput(lParam);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

LRESULT CALLBACK SplashWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rect;
		GetClientRect(hWnd, &rect);
		HBRUSH hBrush = CreateSolidBrush(RGB(220, 220, 220));
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
