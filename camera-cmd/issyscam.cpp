/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include "issyscam.h"
#include <atlbase.h>
#include "..\include\libcamera.h"
#include "common.h"
#include "appcontext.h"

int IsSystemCamera(wchar_t *pszFname, BOOL *pIsSysCam)
{
	CComPtr<ICameraMf> spCameraMf = nullptr;
	int retcode = DEFAULT_ERROR;
	HRESULT hr = S_OK;

	_tprintf(L"Checking if input is system camera.\n");

	hr = CreateCameraMfInstance(&spCameraMf);

	if (SUCCEEDED(hr) && spCameraMf)
	{
		hr = spCameraMf->IsSystemCamera(pszFname, pIsSysCam, (LONG*)&retcode);

		if (SUCCEEDED(hr))
		{
			*pIsSysCam
				? _tprintf(L"\n%s is installed in the system (index = %d).\n", pszFname, retcode)
				: _tprintf(L"\n%s is not installed in the system.\n", pszFname);
		}
		else
		{			
			ErrorCom(hr, L"IsSystemCamera");
			retcode = DEFAULT_ERROR;
		}
	}

	return retcode;
}

int DispatchIsSystemCamera(wchar_t *pszParam, wchar_t *pszSubParam, PVOID pContext)
{
	CContext *pCt = (CContext*)pContext;
	wchar_t szFname[MAX_PATH] = { 0 };
	int retcode = DEFAULT_ERROR;

	wstring wstrparam(pszSubParam);
	size_t fname = wstrparam.find(L"-fname:");
	BOOL bIsSysCam = FALSE;

	if (fname != wstring::npos)
	{
		CopyMemory(szFname, &pszSubParam[fname + 7], ((pCt->m_cchlen - 1) - (fname + 7)) * sizeof(wchar_t));

		retcode = IsSystemCamera(szFname, &bIsSysCam);

		*pCt->m_pCmdSupported = TRUE;
	}

	return retcode;
}
