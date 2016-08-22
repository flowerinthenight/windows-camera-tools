/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include "common.h"
#include <sstream>
using namespace std;

void ErrorCom(HRESULT hr, TCHAR *pszExtra)
{
    _com_error err(hr);
    LPCTSTR szErrorText = err.ErrorMessage();
    TCHAR szDump[MAX_PATH];
    StringCchPrintf(szDump, 100, L"Error %s: %s (hr = 0x%x)\n", pszExtra, szErrorText, hr);
    _tprintf(szDump);
}

vector<wstring> &split(const wstring &s, wchar_t delim, vector<wstring> &elems)
{
    wstringstream ss(s);
    wstring item;

    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }

    return elems;
}