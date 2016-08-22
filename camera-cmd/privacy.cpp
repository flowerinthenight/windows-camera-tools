/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include "privacy.h"
#include <atlbase.h>
#include "..\include\libcamera.h"
#include "common.h"
#include "appcontext.h"

int SetupPrivacy(wchar_t *pszFname, wchar_t *pszState, BOOL bSetPrivacy)
{
	CComPtr<ICameraDs> spCameraDs = nullptr;
	HRESULT hr = S_OK;
	int retcode = DEFAULT_ERROR;

	hr = CreateCameraDsInstance(&spCameraDs);

	if (SUCCEEDED(hr) && spCameraDs)
	{
		hr = spCameraDs->Initialize(pszFname);

		if (SUCCEEDED(hr))
		{
			if (bSetPrivacy)
			{
				hr = spCameraDs->SetPrivacy(_wtol(pszState));

				if (SUCCEEDED(hr))
				{
					_tprintf(L"Output: Operation successful.\n");
					retcode = NOERROR;
				}
				else
				{					
					ErrorCom(hr, L"SetPrivacy");
					retcode = DEFAULT_ERROR;
				}
			}
			else
			{
				LONG lValue = 0, lFlags = 0;

				hr = spCameraDs->GetPrivacy(&lValue, &lFlags);

				if (SUCCEEDED(hr))
				{
					_tprintf(L"Output: Operation successful.\n");
					_tprintf(L"Output: Current privacy settings: %d\n", lValue);
					_tprintf(L"Output: Current privacy flags: %x\n", lFlags);
					retcode = (int)lValue;
				}
				else
				{					
					ErrorCom(hr, L"GetPrivacy");
					retcode = DEFAULT_ERROR;
				}
			}

			spCameraDs->CloseInterfaces();
		}
		else
		{			
			ErrorCom(hr, L"Initialize");
			retcode = DEFAULT_ERROR;
		}
	}

	return retcode;
}

int DispatchPrivacy(wchar_t *pszParams, wchar_t *pszSubParam, PVOID pContext)
{
	CContext *pCt = (CContext*)pContext;
	BOOL bSetPrivacy = FALSE;
	wchar_t szFname[MAX_PATH] = { 0 };
	wchar_t szState[10] = { 0 };
	int retcode = DEFAULT_ERROR;

	wstring wstrparam(pszSubParam);
	size_t fname = wstrparam.find(L"-fname:");
	size_t state = wstrparam.find(L"-state:");

	if (fname != wstring::npos)
	{
		bSetPrivacy = (state == wstring::npos) ? FALSE : TRUE;

		if (state > fname)
		{
			if (bSetPrivacy)
			{
				_tprintf(L"\nSet privacy mode option selected.\n");
				CopyMemory(szFname, &pszSubParam[fname + 7], ((state - 1) - (fname + 7)) * sizeof(wchar_t));
				CopyMemory(szState, &pszSubParam[state + 7], ((pCt->m_cchlen - 1) - (state + 7)) * sizeof(wchar_t));
				szState[1] = L'\0';
			}
			else
			{
				_tprintf(L"\nGet privacy mode option selected.\n");
				CopyMemory(szFname, &pszSubParam[fname + 7], ((pCt->m_cchlen - 1) - (fname + 7)) * sizeof(wchar_t));
			}

			*pCt->m_pCmdSupported = TRUE;
		}

		if (*pCt->m_pCmdSupported == TRUE)
		{
			retcode = SetupPrivacy(szFname, szState, bSetPrivacy);
		}
	}

	return retcode;
}
