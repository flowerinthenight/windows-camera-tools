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
#include "mediainfo.h"
#include "..\include\libcamera.h"
#include "common.h"
#include "appcontext.h"

int DispatchMediaInfo(wchar_t *pszParam, wchar_t *pszSubParam, PVOID pContext)
{
	CContext *pCt = (CContext*)pContext;
	ICameraMf *pCamMf = NULL;
	HRESULT hr = E_FAIL;
	LONG lCount = DEFAULT_ERROR /* this is our return value */;
	wchar_t szFname[MAX_PATH] = { 0 };
	wstring wstrparam(pszSubParam);
	size_t fname = wstrparam.find(L"-fname:");

	if (fname != wstring::npos)
	{
		CopyMemory(szFname, &pszSubParam[fname + 7], ((pCt->m_cchlen - 1) - (fname + 7)) * sizeof(wchar_t));
	
		hr = CreateCameraMfInstance(&pCamMf);

		if (SUCCEEDED(hr) && pCamMf)
		{
			MFMEDIA_INFO *pInfo = NULL;

			hr = pCamMf->MfGetMediaInfo(szFname, &pInfo, &lCount);

			if (SUCCEEDED(hr) && lCount > 0 && pInfo)
			{
				for (int i = 0; i < lCount; i++)
				{
					_tprintf(L"Index: %d\n", pInfo[i].lIndex);
					_tprintf(L"Subtype: %s\n", pInfo[i].szSubtype);
					_tprintf(L"Resolution: %dx%d\n", pInfo[i].lResolutionX, pInfo[i].lResolutionY);
					_tprintf(L"Frame Rate: %d:%d\n", pInfo[i].lFrameRateNumerator, pInfo[i].lFrameRateDenominator);
					_tprintf(L"Pixel Aspect Ratio: %d:%d\n", pInfo[i].lPxAspectRatioNumerator, pInfo[i].lPxAspectRatioDenominator);
					_tprintf(L"Image Stride: %d\n\n", pInfo[i].lStride);
				}

				free(pInfo);
			}

			pCamMf->Release();
		}

		*pCt->m_pCmdSupported = TRUE;
	}	

	return lCount;
}
