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
		hr = spCameraMf->IsSystemCamera(pszFname, pIsSysCam);

		if (SUCCEEDED(hr))
		{
			if (*pIsSysCam)
			{
				_tprintf(L"\n%s is installed in the system.\n", pszFname);
				retcode = (int)TRUE;
			}
			else
			{
				_tprintf(L"\n%s is not installed in the system.\n", pszFname);
				retcode = (int)FALSE;
			}
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
