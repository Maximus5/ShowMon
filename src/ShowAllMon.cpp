// ShowAllMon.cpp : Defines the entry point for the application.
//

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "Resource.h"

HINSTANCE hInst;
HWND ghWnd = NULL;
wchar_t szTitle[] = L"Monitor monitoring";
wchar_t szWindowClass[] = L"MonitorMonitoring";

#define REFRESH_TIMER_ID 101
UINT REFRESH_TIMER_DELAY = 2500;

RECT rcMainWnd = {};
RECT rcSecondary = {};

SIZE GetVirtualScreenSize()
{
	SIZE sz = { GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN) };
	return sz;
}

RECT GetPrimaryMonitorRect()
{
	POINT pt = {};
	HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO mi = { sizeof(mi) }; GetMonitorInfo(hMon, &mi);
	return mi.rcWork;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	LPMONITORINFO p = (LPMONITORINFO)dwData;
	if (GetMonitorInfo(hMonitor, p)
		&& !(p->dwFlags & MONITORINFOF_PRIMARY))
		return FALSE;
	return TRUE;
}

RECT GetSecondaryMonitorRect()
{
	MONITORINFO mi = { sizeof(mi) };
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&mi);
	if (!IsRectEmpty(&mi.rcMonitor))
		return mi.rcMonitor;
	return GetPrimaryMonitorRect();
}

RECT EvalWindowRect()
{
	rcSecondary = GetSecondaryMonitorRect();
	RECT rc = GetPrimaryMonitorRect();
	int iMaxW = ((rc.right - rc.left) / 6);
	int iMaxH = ((rc.bottom - rc.top) / 6);
	int szX = (rcSecondary.right - rcSecondary.left);
	int szY = (rcSecondary.bottom - rcSecondary.top);
	int iZoom = min((iMaxW * 100 / szX), (iMaxH * 100 / szY));
	int iEvalW = (rc.right - rc.left) * iZoom / 100;
	int iEvalH = (rc.bottom - rc.top) * iZoom / 100;
	RECT rcCurrent = {};
	if (ghWnd)
	{
		GetWindowRect(ghWnd, &rcCurrent);
	}
	else
	{
		rcCurrent.left = rc.right - 50 - iEvalW;
		rcCurrent.top = rc.bottom - 50 - iEvalH;
	}
	RECT rcEval = { rcCurrent.left, rcCurrent.top, rcCurrent.left + iEvalW, rcCurrent.top + iEvalH };
	if (ghWnd && memcmp(&rcEval, &rcCurrent, sizeof(rcEval)))
	{
		MoveWindow(ghWnd, rcEval.left, rcEval.top, iEvalW, iEvalH, FALSE);
		GetWindowRect(ghWnd, &rcMainWnd);
	}
	else
	{
		rcMainWnd = rcEval;
	}
	return rcEval;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	RECT rc = EvalWindowRect();

	ghWnd = CreateWindowEx(WS_EX_TOPMOST,
		szWindowClass, szTitle, WS_POPUP,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);

	if (!ghWnd)
	{
		return FALSE;
	}

	ShowWindow(ghWnd, nCmdShow);
	UpdateWindow(ghWnd);

	return TRUE;
}

void OnPaint(HDC hdc, PAINTSTRUCT* ps)
{
	HDC hScreen = GetDC(NULL);
	RECT rcClient = {}; GetClientRect(ghWnd, &rcClient);
	int iSrcWidth = rcSecondary.right-rcSecondary.left;
	int iSrcHeight = rcSecondary.bottom-rcSecondary.top;
	POINT ptCur = {}; GetCursorPos(&ptCur);
	if (PtInRect(&rcSecondary, ptCur))
	{
		int srcX = max(rcSecondary.left, min(ptCur.x-(rcClient.right/2), rcSecondary.right-rcClient.right));
		int srcY = max(rcSecondary.top, min(ptCur.y-(rcClient.bottom/2), rcSecondary.bottom-rcClient.bottom));
		BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom,
			hScreen, srcX, srcY,
			SRCCOPY);

		int curX = ptCur.x - srcX;
		int curY = ptCur.y - srcY;

		HBRUSH hBr = CreateSolidBrush(0xC0C0C0);
		HBRUSH hOld = (HBRUSH)SelectObject(hdc, hBr);
		int nd = 30;
		BitBlt(hdc, max(0,curX-1), max(0,curY-nd), 2, nd+min(nd,curY), hdc, 0,0, PATINVERT);
		BitBlt(hdc, max(0,curX-nd), max(0,curY-1), nd+min(nd,curX), 2, hdc, 0,0, PATINVERT);
		SelectObject(hdc, hOld);
		DeleteObject(hBr);
	}
	else
	{
		StretchBlt(hdc, 0, 0, rcClient.right, rcClient.bottom,
			hScreen, rcSecondary.left, rcSecondary.top,
			iSrcWidth, iSrcHeight,
			SRCCOPY);
	}
	ReleaseDC(NULL, hScreen);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
		SetTimer(hWnd, REFRESH_TIMER_ID, REFRESH_TIMER_ID, NULL);
		break;
	case WM_PAINT:
		if ((hdc = BeginPaint(hWnd, &ps)) != NULL)
		{
			OnPaint(hdc, &ps);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_TIMER:
		EvalWindowRect();
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_NCHITTEST:
		return HTCAPTION;
	case WM_DESTROY:
		KillTimer(hWnd, REFRESH_TIMER_ID);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SHOWALLMON));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
	wcex.lpszClassName = szWindowClass;
	//wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;

	// Initialize global strings
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}
