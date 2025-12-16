#include "pch.h"
#include "FScreenShot.h"
#include "FUtil.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>


FWindowScreenshot::FWindowScreenshot()
{
	InitializeDXGI();
}

FWindowScreenshot::~FWindowScreenshot()
{
	Cleanup();
}

bool FWindowScreenshot::InitializeDXGI()
{
	HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&pFactory));
	if (FAILED(hr)) {
		std::cerr << "Failed to create DXGI factory" << std::endl;
		return false;
	}

	IDXGIAdapter* pAdapter = nullptr;
	hr = pFactory->EnumAdapters(0, &pAdapter);
	if (FAILED(hr)) {
		std::cerr << "Failed to enumerate adapters" << std::endl;
		return false;
	}

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	hr = D3D11CreateDevice(
		pAdapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		0,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&pDevice,
		nullptr,
		&pContext
	);

	if (pAdapter) pAdapter->Release();

	if (FAILED(hr)) {
		std::cerr << "Failed to create D3D11 device" << std::endl;
		return false;
	}

	return true;
}

void FWindowScreenshot::Cleanup()
{
	if (pContext) pContext->Release();
	if (pDevice) pDevice->Release();
	if (pFactory) pFactory->Release();
}

bool FWindowScreenshot::CaptureWindowDXGI(HWND hWnd, const std::wstring& filename)
{
	HMODULE hUser32 = LoadLibrary(_T("user32.dll"));
	if (!hUser32) {
		std::cerr << "Failed to load user32.dll" << std::endl;
		return false;
	}

	auto DwmGetDxSharedSurface = (HRESULT(WINAPI*)(HWND, HANDLE*, LUID*, ULONG*, ULONG*, ULONGLONG*))
		GetProcAddress(hUser32, "DwmGetDxSharedSurface");

	if (!DwmGetDxSharedSurface) {
		std::cerr << "DwmGetDxSharedSurface not found" << std::endl;
		FreeLibrary(hUser32);
		return false;
	}

	HANDLE phSurface = NULL;
	HRESULT hr = DwmGetDxSharedSurface(hWnd, &phSurface, NULL, NULL, NULL, NULL);
	if (FAILED(hr) || !phSurface) {
		std::cerr << "Failed to get shared surface" << std::endl;
		FreeLibrary(hUser32);
		return false;
	}

	ID3D11Texture2D* pSharedTexture = nullptr;
	hr = pDevice->OpenSharedResource(phSurface, __uuidof(ID3D11Texture2D), (void**)(&pSharedTexture));
	if (FAILED(hr)) {
		std::cerr << "Failed to open shared resource" << std::endl;
		FreeLibrary(hUser32);
		return false;
	}

	D3D11_TEXTURE2D_DESC shared_desc;
	pSharedTexture->GetDesc(&shared_desc);

	// 创建CPU可读的纹理
	shared_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	shared_desc.Usage = D3D11_USAGE_STAGING;
	shared_desc.BindFlags = 0;
	shared_desc.MiscFlags = 0;

	ID3D11Texture2D* pStagingTexture = nullptr;
	hr = pDevice->CreateTexture2D(&shared_desc, nullptr, &pStagingTexture);
	if (FAILED(hr)) {
		std::cerr << "Failed to create staging texture" << std::endl;
		pSharedTexture->Release();
		FreeLibrary(hUser32);
		return false;
	}

	pContext->CopyResource(pStagingTexture, pSharedTexture);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	hr = pContext->Map(pStagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
	if (FAILED(hr)) {
		std::cerr << "Failed to map texture" << std::endl;
		pStagingTexture->Release();
		pSharedTexture->Release();
		FreeLibrary(hUser32);
		return false;
	}

	// 使用OpenCV处理图像数据
	cv::Mat mat(shared_desc.Height, shared_desc.Width, CV_8UC4, mappedResource.pData, mappedResource.RowPitch);

	// 转换为BGR格式（OpenCV默认格式）
	cv::Mat bgrMat;
	cv::cvtColor(mat, bgrMat, cv::COLOR_BGRA2BGR);

	// 保存为无损JPEG（质量100%）
	std::vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100); // 100%质量，接近无损

	std::string path = FUtil::Wstring2String(filename);
	bool success = cv::imwrite(path, bgrMat, compression_params);

	pContext->Unmap(pStagingTexture, 0);
	pStagingTexture->Release();
	pSharedTexture->Release();
	FreeLibrary(hUser32);

	return success;
}

bool FWindowScreenshot::CaptureWindowGDI(HWND hWnd, const std::wstring& filename)
{
	// 获取窗口真实尺寸（考虑DPI缩放）
	RECT rect;
	if (FAILED(DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(RECT)))) {
		GetWindowRect(hWnd, &rect);
	}

	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	if (width <= 0 || height <= 0) {
		std::cerr << "Invalid window size" << std::endl;
		return false;
	}

	HDC screenDC = GetDC(NULL);
	HDC memDC = CreateCompatibleDC(screenDC);
	HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, width, height);

	SelectObject(memDC, hBitmap);

	// 截图
	BOOL result = BitBlt(memDC, 0, 0, width, height, screenDC, rect.left, rect.top, SRCCOPY);
	if (!result) {
		std::cerr << "BitBlt failed" << std::endl;
		DeleteObject(hBitmap);
		DeleteDC(memDC);
		ReleaseDC(NULL, screenDC);
		return false;
	}

	// 转换到OpenCV格式
	cv::Mat img(height, width, CV_8UC4);
	GetBitmapBits(hBitmap, img.total() * img.elemSize(), img.data);

	// 转换为BGR格式
	cv::Mat bgrImg;
	cv::cvtColor(img, bgrImg, cv::COLOR_BGRA2BGR);

	// 保存为无损JPEG
	std::vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100); // 100%质量

	bool success = cv::imwrite(FUtil::Wstring2String(filename), bgrImg, compression_params);

	// 清理资源
	DeleteObject(hBitmap);
	DeleteDC(memDC);
	ReleaseDC(NULL, screenDC);

	return success;
}

std::map<HWND, std::wstring> FWindowScreenshot::EnumWindowsList()
{
	std::map<HWND, std::wstring> windows;

	::EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
		auto& windows = *reinterpret_cast<std::map<HWND, std::wstring>*>(lParam);

		if (IsWindowVisible(hwnd)) {
			TCHAR title[256];
			GetWindowText(hwnd, title, sizeof(title) / sizeof(TCHAR));
			if (_tcslen(title) > 0) {
				windows[hwnd] = title;
			}
		}
		return TRUE;
		}, reinterpret_cast<LPARAM>(&windows));

	return windows;
}

HWND FWindowScreenshot::FindWindowByTitle(const std::wstring& title)
{
	auto windows = EnumWindowsList();
	for (const auto& [hwnd, winTitle] : windows) {
		if (winTitle.find(title) != std::wstring::npos) {
			return hwnd;
		}
	}
	return nullptr;
}
