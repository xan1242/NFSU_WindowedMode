// NFS Underground - proper windowed mode
// by Xan/Tenjoin

#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\injector\injector.hpp"
#include "..\includes\IniReader.h"

bool bEnableWindowed = false;
bool bBorderlessWindowed = false;
bool bEnableWindowResize = false;
bool bRelocateWindow = false;

int RelocateX = 0;
int RelocateY = 0;

int DesktopX = 0;
int DesktopY = 0;

int WSFixResX = 0;
int WSFixResY = 0;

HWND GameHWND = NULL;
RECT windowRect;

int GetDesktopRes(int32_t* DesktopResW, int32_t* DesktopResH)
{
    HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &info);
    *DesktopResW = info.rcMonitor.right - info.rcMonitor.left;
    *DesktopResH = info.rcMonitor.bottom - info.rcMonitor.top;
    return 0;
}

BOOL WINAPI AdjustWindowRect_Hook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
{
    DWORD newStyle = 0;

    if (!bBorderlessWindowed)
        newStyle = WS_CAPTION;

    return AdjustWindowRect(lpRect, newStyle, bMenu);
}

HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    int WindowPosX = RelocateX;
    int WindowPosY = RelocateY;

    GetDesktopRes(&DesktopX, &DesktopY);

    if (!bRelocateWindow)
    {
        // fix the window to open at the center of the screen...
        WindowPosX = (int)(((float)DesktopX / 2.0f) - ((float)nWidth / 2.0f));
        WindowPosY = (int)(((float)DesktopY / 2.0f) - ((float)nHeight / 2.0f));
    }

    GameHWND = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, 0, WindowPosX, WindowPosY, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    LONG lStyle = GetWindowLong(GameHWND, GWL_STYLE);

    if (bBorderlessWindowed)
        lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    else
    {
        lStyle |= (WS_MINIMIZEBOX | WS_SYSMENU);
        if (bEnableWindowResize)
            lStyle |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
    }

    SetWindowLong(GameHWND, GWL_STYLE, lStyle);
    SetWindowPos(GameHWND, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

    return GameHWND;
}

void InitConfig()
{
    CIniReader inireader("");
    bEnableWindowed = inireader.ReadInteger("NFSUWindow", "EnableWindowed", 0);
    bBorderlessWindowed = inireader.ReadInteger("NFSUWindow", "BorderlessWindowed", 0);
    bEnableWindowResize = inireader.ReadInteger("NFSUWindow", "EnableWindowResize", 0);
    bRelocateWindow = inireader.ReadInteger("NFSUWindow", "RelocateWindow", 0);

    if (bRelocateWindow)
    {
        RelocateX = inireader.ReadInteger("WindowLocation", "X", 0);
        RelocateY = inireader.ReadInteger("WindowLocation", "Y", 0);
    }
}

void InitWSFixConfig()
{
    CIniReader inireader("NFSUnderground.WidescreenFix.ini");
    WSFixResX = inireader.ReadInteger("MAIN", "ResX", 0);
    WSFixResY = inireader.ReadInteger("MAIN", "ResY", 0);

    if ((!WSFixResX) || (!WSFixResY))
        GetDesktopRes(&WSFixResX, &WSFixResY);
}

int Init()
{
    // skip SetWindowLong because it messes things up (also negates icon fixes by widescreen fix but oh well)
    injector::MakeJMP(0x00408AA1, 0x00408AB5, true);
    // hook the offending functions
    injector::MakeNOP(0x004089B1, 6, true);
    injector::MakeCALL(0x004089B1, CreateWindowExA_Hook, true);
    injector::MakeNOP(0x00408975, 6, true);
    injector::MakeCALL(0x00408975, AdjustWindowRect_Hook, true);
    injector::MakeNOP(0x004086CB, 2, true); // perhaps not necessary
    
    if (bEnableWindowed)
        *(int*)0x0073637C = 1;

    // initial ResX
    *(int*)0x00701034 = WSFixResX;
    // initial ResY
    *(int*)0x00701038 = WSFixResY;


	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
        InitConfig();
        InitWSFixConfig();
		Init();
	}
	return TRUE;
}

