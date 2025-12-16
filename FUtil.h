#pragma once
#include <string>
#include <windows.h>

struct FUtil
{
	static std::string Wstring2String(std::wstring wstr)
	{
		// support chinese
		std::string res;
		int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), nullptr, 0, nullptr, nullptr);
		if (len <= 0) {
			return res;
		}
		char* buffer = new char[len + 1];
		if (buffer == nullptr) {
			return res;
		}
		WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, nullptr, nullptr);
		buffer[len] = '\0';
		res.append(buffer);
		delete[] buffer;
		return res;
	}

	static std::wstring String2Wstring(std::string wstr)
	{
		std::wstring res;
		int len = MultiByteToWideChar(CP_ACP, 0, wstr.c_str(), wstr.size(), nullptr, 0);
		if (len < 0) {
			return res;
		}
		wchar_t* buffer = new wchar_t[len + 1];
		if (buffer == nullptr) {
			return res;
		}
		MultiByteToWideChar(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len);
		buffer[len] = '\0';
		res.append(buffer);
		delete[] buffer;
		return res;
	}
};

