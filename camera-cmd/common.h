/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#pragma once

#include <Windows.h>
#include <comdef.h>
#include <strsafe.h>
#include <string>
#include <vector>

void ErrorCom(HRESULT hr, TCHAR *pszExtra);
std::vector<std::wstring> &split(const std::wstring &s, wchar_t delim, std::vector<std::wstring> &elems);