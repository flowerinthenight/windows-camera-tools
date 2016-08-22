/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include <Windows.h>
#include <DShow.h>
#include <Ks.h>
#include <KsMedia.h>
#include <ksproxy.h>
#include <string>
using namespace std;
#include <strsafe.h>
#include <comdef.h>
#include <atlbase.h>
#include "..\include\libcore.h"
#include "..\include\libcamera.h"
#include "appcontext.h"
#include "flash.h"
#include "privacy.h"
#include "issyscam.h"
#include "fnames.h"
#include "mediainfo.h"
#include "proppage.h"
#include "common.h"
#include "..\etw\jytrace.h"

#pragma comment(lib, "..\\out\\libcore.lib")

static CContext ct;

int Help(wchar_t *pszParam, wchar_t *pszSubParam, PVOID pContext)
{
    CContext *pCt = (CContext*)pContext;

    _tprintf(L"Synopsis:\n\n");
    _tprintf(L"    camera-cmd.exe option <params...>\n\n");
    _tprintf(L"Options:\n\n");
    _tprintf(L"    fnames\n\n");
    _tprintf(L"        Lists the available/attached camera(s) in the system. Returns\n");
    _tprintf(L"        the number of available camera(s) found if any, otherwise,\n");
    _tprintf(L"        returns -1.\n\n");
    _tprintf(L"    mediainfo\n\n");
    _tprintf(L"        Lists media-related information of the provided camera name.\n");
    _tprintf(L"        Returns the number of information structures retrieved.\n\n");
    _tprintf(L"        Supported parameters:\n\n");
    _tprintf(L"        -fname:<camera_friendly_name>\n\n");
    _tprintf(L"        camera_friendly_name\n\n");
    _tprintf(L"            Friendly name of the camera device/driver. You can use\n");
    _tprintf(L"            the 'fnames' parameter for the friendly name(s) or see\n");
    _tprintf(L"            Device Manager -> Imaging devices.\n\n");
    _tprintf(L"        Example:\n\n");
    _tprintf(L"        camera-cmd.exe mediainfo -fname:Integrated Camera\n\n");
    _tprintf(L"    proppage\n\n");
    _tprintf(L"        Opens the driver-provided (if any) extended property page(s).\n\n");
    _tprintf(L"        Supported parameters:\n\n");
    _tprintf(L"        -fname:<camera_friendly_name>\n\n");
    _tprintf(L"        camera_friendly_name\n\n");
    _tprintf(L"            Friendly name of the camera device/driver. You can use\n");
    _tprintf(L"            the 'fnames' parameter for the friendly name(s) or see\n");
    _tprintf(L"            Device Manager -> Imaging devices.\n\n");
    _tprintf(L"        Example:\n\n");
    _tprintf(L"        camera-cmd.exe proppage -fname:Integrated Camera\n\n");
    _tprintf(L"    issyscam\n\n");
    _tprintf(L"        Returns the index of the camera friendly name being queried.\n");
    _tprintf(L"        Returns -1 on failure, or when the camera is not found.\n\n");
    _tprintf(L"        Supported parameters:\n\n");
    _tprintf(L"        -fname:<camera_friendly_name>\n\n");
    _tprintf(L"        camera_friendly_name\n\n");
    _tprintf(L"            Friendly name of the camera device/driver. You can use\n");
    _tprintf(L"            the 'fnames' parameter for the friendly name(s) or see\n");
    _tprintf(L"            Device Manager -> Imaging devices.\n\n");
    _tprintf(L"        Example:\n\n");
    _tprintf(L"        camera-cmd.exe issyscam -fname:Integrated Camera\n\n");
    _tprintf(L"    privacy\n\n");
    _tprintf(L"        Controls the camera privacy properties. On query, it will\n");
    _tprintf(L"        return the current state. On set, it will return 0 on\n");
    _tprintf(L"        success, -1 on failure.\n\n");
    _tprintf(L"        Supported parameters:\n\n");
    _tprintf(L"        -fname:<camera_friendly_name>\n\n");
    _tprintf(L"        camera_friendly_name\n\n");
    _tprintf(L"            Friendly name of the camera device/driver. You can use\n");
    _tprintf(L"            the 'fnames' parameter for the friendly name(s) or see\n");
    _tprintf(L"            Device Manager -> Imaging devices.\n\n");
    _tprintf(L"        -state:<0|1>\n\n");
    _tprintf(L"        0 - turn privacy mode off.\n");
    _tprintf(L"        1 - turn privacy mode on.\n\n");
    _tprintf(L"        Examples:\n\n");
    _tprintf(L"        camera-cmd.exe privacy -fname:Integrated Camera\n");
    _tprintf(L"        camera-cmd.exe privacy -fname:Integrated Camera -state:1\n\n");
    _tprintf(L"    flash\n\n");
    _tprintf(L"        Controls the camera flash properties. On query, it will\n");
    _tprintf(L"        return the current state. On set, it will return 0 on\n");
    _tprintf(L"        success, -1 on failure.\n\n");
    _tprintf(L"        Supported parameters:\n\n");
    _tprintf(L"        -fname:<camera_friendly_name>\n\n");
    _tprintf(L"        camera_friendly_name\n\n");
    _tprintf(L"            Friendly name of the camera device/driver. You can use\n");
    _tprintf(L"            the 'fnames' parameter for the friendly name(s) or see\n");
    _tprintf(L"            Device Manager -> Imaging devices.\n\n");
    _tprintf(L"        -state:<0|1|2>\n\n");
    _tprintf(L"        0 - turn camera flash off.\n");
    _tprintf(L"        1 - turn camera flash on.\n");
    _tprintf(L"        2 - set camera flash to auto flash.\n\n");
    _tprintf(L"        Examples:\n\n");
    _tprintf(L"        camera-cmd.exe flash -fname:Integrated Camera\n");
    _tprintf(L"        camera-cmd.exe flash -fname:Integrated Camera -state:2\n\n");

    *pCt->m_pCmdSupported = TRUE;

    return NOERROR;
}

int Junk(wchar_t *pszParam, wchar_t *pszSubParam, PVOID pContext)
{
    CContext *pCt = (CContext*)pContext;

    ICameraDs *pCamDs = NULL;
    HRESULT hr = E_FAIL;

    hr = CreateCameraDsInstance(&pCamDs);

    if (SUCCEEDED(hr) && pCamDs)
    {
        hr = pCamDs->Initialize(L"Integrated Camera");
        hr = pCamDs->LaunchPropertiesFrame();
        pCamDs->Release();
    }

    *pCt->m_pCmdSupported = TRUE;

    return NOERROR;
}

static ARG_DISPATCH_TABLE pdt[] = {
    { L"junk", Junk },
    { L"help", Help },
    { L"fnames", DispatchFriendlyNames },
    { L"mediainfo", DispatchMediaInfo },
    { L"flash", DispatchFlash },
    { L"privacy", DispatchPrivacy },
    { L"issyscam", DispatchIsSystemCamera },
    { L"proppage", DispatchPropertyPage },
};

//
// Main entry.
//
int _tmain(int argc, _TCHAR* argv[])
{
    wchar_t szCtrl[MAX_PATH] = { 0 };
    wchar_t szParam[MAX_PATH] = { 0 };
    int retcode = DEFAULT_ERROR;
    BOOL bSupportedCmd = FALSE;
    HRESULT hr = S_OK;

    ct.m_pCmdSupported = &bSupportedCmd;
    ct.m_argc = argc;
    ct.m_ppargv = argv;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (argc >= 2)
    {
        StringCchPrintf(szCtrl, MAX_PATH, L"%s", argv[1]);

        for (int itr = 0; itr < ARRAYSIZE(pdt); itr++)
        {
            if (_wcsicmp(szCtrl, pdt[itr].szParam) == 0)
            {
                for (int i = 2; i < argc; i++)
                {
                    StringCchCat(szParam, MAX_PATH, argv[i]);
                    StringCchCat(szParam, MAX_PATH, L" ");
                }

                StringCchLength(szParam, MAX_PATH, &ct.m_cchlen);

                //
                // Call dispatch function.
                //
                retcode = pdt[itr].pfnDispatch(pdt[itr].szParam, szParam, &ct);

                break;
            }
        }
    }

    CoUninitialize();

    if (!bSupportedCmd)
    {
        _tprintf(L"\nCommand not supported.\n\n");
        Help(NULL, NULL, &ct);
    }

    return retcode;
}