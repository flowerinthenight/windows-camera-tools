/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#pragma once

#include <Windows.h>

int SetupPrivacy(wchar_t *pszFname, wchar_t *pszState, BOOL bSetPrivacy);
int DispatchPrivacy(wchar_t *pszParam, wchar_t *pszSubParam, PVOID pContext);
