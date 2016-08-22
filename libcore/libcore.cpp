/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include <Windows.h>
#include <comdef.h>
#include <strsafe.h>
#include "..\include\libcore.h"

_declspec(dllexport) void PrintComError(HRESULT hr, TCHAR *pszExtra)
{
    _com_error err(hr);
    LPCTSTR szErrorText = err.ErrorMessage();
    TCHAR szDump[MAX_PATH];
    StringCchPrintf(szDump, 100, L"%s: %s (0x%x)\n", pszExtra, szErrorText, hr);
    OutputDebugString(szDump);
}

_declspec(dllexport) HRESULT GetComTextError(HRESULT hr, wchar_t *pszOut, DWORD *pcchLen)
{
    if (!pcchLen) return E_INVALIDARG;
    if (*pcchLen < 1) return E_INVALIDARG;

    TCHAR szTrace[MAX_PATH];
    _com_error err(hr);
    LPCTSTR szErrorText = err.ErrorMessage();

    StringCchPrintf(szTrace, *pcchLen, L"%s (0x%x)", szErrorText, hr);

    return StringCchCopy(pszOut, *pcchLen, szTrace);
}