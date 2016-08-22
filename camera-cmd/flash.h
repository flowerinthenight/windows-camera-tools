/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#pragma once

#include <Windows.h>

int SetupFlash(wchar_t *pszFname, wchar_t *pszState, BOOL bSetFlash);
int DispatchFlash(wchar_t *pszParam, wchar_t *pszSubParam, PVOID pContext);
