#pragma once

#include "pch.h"
#include <windows.h>
#include <dxgi.h>
#include <d3d11.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <codecvt>



class FWindowScreenshot {
private:
	ID3D11Device* pDevice = nullptr;
	ID3D11DeviceContext* pContext = nullptr;
	IDXGIFactory1* pFactory = nullptr;


public:
	FWindowScreenshot();

	~FWindowScreenshot();

private:
	bool InitializeDXGI();

	void Cleanup();

public:
	// 方法一：使用DXGI截图（高性能）
	bool CaptureWindowDXGI(HWND hWnd, const std::wstring& filename);

	// 方法二：使用Windows API + OpenCV（兼容性好）
	bool CaptureWindowGDI(HWND hWnd, const std::wstring& filename);

	// 枚举所有可见窗口
	std::map<HWND, std::wstring> EnumWindowsList();

	// 根据窗口标题查找窗口
	HWND FindWindowByTitle(const std::wstring& title);
};