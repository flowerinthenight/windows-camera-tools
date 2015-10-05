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
