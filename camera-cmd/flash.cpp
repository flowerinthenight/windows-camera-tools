/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include "flash.h"
#include <atlbase.h>
#include "..\include\libcamera.h"
#include "common.h"
#include "appcontext.h"

int SetupFlash(wchar_t *pszFname, wchar_t *pszState, BOOL bSetFlash)
{
    CComPtr<ICameraDs> spCameraDs = nullptr;
    int retcode = DEFAULT_ERROR;
    HRESULT hr = S_OK;

    hr = CreateCameraDsInstance(&spCameraDs);

    if (SUCCEEDED(hr) && spCameraDs)
    {
        hr = spCameraDs->Initialize(pszFname);

        if (SUCCEEDED(hr))
        {
            if (bSetFlash)
            {
                int newstate = _wtoi(pszState);
                CameraFlashEnum newFlashVal;

                switch (newstate)
                {
                case 0: newFlashVal = CameraFlashOff; break;
                case 1: newFlashVal = CameraFlashOn; break;
                case 2: newFlashVal = CameraFlashAuto; break;
                default: newFlashVal = CameraFlashAuto; break;
                }

                hr = spCameraDs->SetFlash(newFlashVal);

                if (SUCCEEDED(hr))
                {
                    _tprintf(L"Output: Operation successful.\n");
                    retcode = NOERROR;
                }
                else
                {
                    ErrorCom(hr, L"SetFlash");
                    retcode = DEFAULT_ERROR;
                }
            }
            else
            {
                CameraFlashEnum currFlash;
                LONG lFlags;

                hr = spCameraDs->GetFlash(&currFlash, &lFlags);

                if (SUCCEEDED(hr) && (currFlash >= -1 && currFlash <= CameraFlashCheckCount))
                {
                    _tprintf(L"Output: Operation successful.\n");
                    _tprintf(L"Output: Current flash settings: %d\n", currFlash);
                    _tprintf(L"Output: Current flash flags: 0x%x\n", lFlags);
                    retcode = (int)currFlash;
                }
                else
                {
                    ErrorCom(hr, L"GetFlash");
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

int DispatchFlash(wchar_t *pszParam, wchar_t *pszSubParam, PVOID pContext)
{
    CContext *pCt = (CContext*)pContext;
    BOOL bSetFlash = FALSE;
    wchar_t szFname[MAX_PATH] = { 0 };
    wchar_t szState[10] = { 0 };
    int retcode = DEFAULT_ERROR;

    wstring wstrparam(pszSubParam);
    size_t fname = wstrparam.find(L"-fname:");
    size_t state = wstrparam.find(L"-state:");

    if (fname != wstring::npos)
    {
        bSetFlash = (state == wstring::npos) ? FALSE : TRUE;

        if (state > fname)
        {
            if (bSetFlash)
            {
                _tprintf(L"\nSet flash option selected.\n");
                CopyMemory(szFname, &pszSubParam[fname + 7], ((state - 1) - (fname + 7)) * sizeof(wchar_t));
                CopyMemory(szState, &pszSubParam[state + 7], ((pCt->m_cchlen - 1) - (state + 7)) * sizeof(wchar_t));
                szState[1] = L'\0';
            }
            else
            {
                _tprintf(L"\nGet flash option selected.\n");
                CopyMemory(szFname, &pszSubParam[fname + 7], ((pCt->m_cchlen - 1) - (fname + 7)) * sizeof(wchar_t));
            }

            *pCt->m_pCmdSupported = TRUE;
        }

        if (*pCt->m_pCmdSupported == TRUE)
        {
            retcode = SetupFlash(szFname, szState, bSetFlash);
        }
    }

    return retcode;
}