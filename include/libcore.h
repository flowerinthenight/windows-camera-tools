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

#pragma once

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif
	_declspec(dllexport) void PrintComError(HRESULT hr, wchar_t *pszExtra);
	_declspec(dllexport) HRESULT GetComTextError(HRESULT hr, wchar_t *pszOut, DWORD *pcchLen);
#ifdef __cplusplus
}
#endif

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

#define SAFE_RELEASE(ptr) \
{ \
	if (ptr) \
	{ \
		(ptr)->Release(); \
		(ptr) = NULL; \
	} \
}

typedef int(*FnDispatchParam)(wchar_t *pszParam, wchar_t *pszSubParam, PVOID pContext);

typedef struct __ARG_DISPATCH_TABLE {
	wchar_t szParam[MAX_PATH];
	FnDispatchParam pfnDispatch;
} ARG_DISPATCH_TABLE, *PARG_DISPATCH_TABLE;
