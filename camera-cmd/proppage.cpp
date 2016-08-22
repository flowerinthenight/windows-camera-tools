/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include "proppage.h"
#include "..\include\libcamera.h"
#include "common.h"
#include "appcontext.h"

int DispatchPropertyPage(wchar_t *pszParam, wchar_t *pszSubParam, PVOID pContext)
{
    CContext *pCt = (CContext*)pContext;
    ICameraDs *pCamDs = NULL;
    HRESULT hr = E_FAIL;
    wchar_t szFname[MAX_PATH] = { 0 };
    wstring wstrparam(pszSubParam);
    size_t fname = wstrparam.find(L"-fname:");
    int retcode = DEFAULT_ERROR;

    if (fname != wstring::npos)
    {
        CopyMemory(szFname, &pszSubParam[fname + 7], ((pCt->m_cchlen - 1) - (fname + 7)) * sizeof(wchar_t));

        hr = CreateCameraDsInstance(&pCamDs);

        if (SUCCEEDED(hr) && pCamDs)
        {
            hr = pCamDs->Initialize(szFname);

            if (SUCCEEDED(hr))
            {
                hr = pCamDs->LaunchPropertiesFrame();
                retcode = NOERROR;
            }

            pCamDs->Release();
        }

        *pCt->m_pCmdSupported = TRUE;
    }

    return retcode;
}