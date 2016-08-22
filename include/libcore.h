/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
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
