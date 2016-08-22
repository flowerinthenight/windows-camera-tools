/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include "fnames.h"
#include "..\include\libcamera.h"
#include "common.h"
#include "appcontext.h"

int DispatchFriendlyNames(wchar_t *pszParam, wchar_t *pszSubParam, PVOID pContext)
{
	CContext *pCt = (CContext*)pContext;
	ICameraMf *pCamMf = NULL;
	HRESULT hr = E_FAIL;
	int retcode = DEFAULT_ERROR;

	hr = CreateCameraMfInstance(&pCamMf);

	if (SUCCEEDED(hr) && pCamMf)
	{
		wchar_t *pszNames = NULL;
		LONG cbSize = 0;

		hr = pCamMf->GetFriendlyNames(&pszNames, &cbSize);

		if (pszNames)
		{
			vector<std::wstring> names;
			wstring wstrnames(pszNames);

			split(wstrnames, L';', names);

			if (names.size() > 0)
			{
				_tprintf(L"Available camera(s):\n");

				for (int i = 0; i < names.size(); i++)
				{
					_tprintf(L"%d. %s\n", i + 1, names.at(i).c_str());
				}

				retcode = names.size();
			}

			free(pszNames);
		}

		pCamMf->Release();
	}

	*pCt->m_pCmdSupported = TRUE;

	return retcode;
}
