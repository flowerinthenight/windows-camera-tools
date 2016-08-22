/*
*		Win-camera-tools: Generic camera tools for Windows.
*		Copyright (C) 2015 Chew Esmero
*
*		This file is part of win-camera-tools.
*
*		Win-camera-tools is free software: you can redistribute it and/or modify
*		it under the terms of the GNU General Public License as published by
*		the Free Software Foundation, either version 3 of the License, or
*		(at your option) any later version.
*
*		Win-camera-tools is distributed in the hope that it will be useful,
*		but WITHOUT ANY WARRANTY; without even the implied warranty of
*		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*		GNU General Public License for more details.
*
*		You should have received a copy of the GNU General Public License
*		along with win-camera-tools. If not, see <http://www.gnu.org/licenses/>.
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
