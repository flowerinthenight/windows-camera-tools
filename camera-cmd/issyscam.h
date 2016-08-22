/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#pragma once

#include <Windows.h>

int IsSystemCamera(wchar_t *pszFname, BOOL *pIsSysCam);
int DispatchIsSystemCamera(wchar_t *pszParam, wchar_t *pszSubParam, PVOID pContext);
