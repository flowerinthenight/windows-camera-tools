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

#pragma once

#include <Windows.h>
#include <time.h>
#include <vector>
using namespace std;
#include <DShow.h>
#include <Ks.h>
#include <KsMedia.h>
#include <mfidl.h>

#define FOURCC_DMP(dw, a, b, c, d) \
{ \
	WORD hw = HIWORD(dw); \
	WORD lw = LOWORD(dw); \
	*a = (CHAR)HIBYTE(hw); \
	*b = (CHAR)LOBYTE(hw); \
	*c = (CHAR)HIBYTE(lw); \
	*d = (CHAR)LOBYTE(lw); \
}

struct __declspec(uuid("BE660B75-1315-4A97-8297-1E678E28953D")) IBufferLock;
struct __declspec(uuid("91B64872-05C4-405F-9266-B7AAFF000A35")) ICameraDs;
struct __declspec(uuid("5B57A953-D677-4073-B64B-78BD270E56CB")) ICameraMf;
struct __declspec(uuid("9ADA3F32-E5F5-415E-A120-2AB0AAA700C0")) IWebcamWrapper;

typedef HRESULT (CALLBACK *FrameCallback)(HRESULT hrStatus,
	DWORD dwStreamIndex,
	DWORD dwStreamFlags,
	LONGLONG llTimestamp,
	IMFSample *pSample
);

typedef enum {
	CameraFlashOff = 0,
	CameraFlashOn,
	CameraFlashAuto,
	CameraFlashCheckCount,
} CameraFlashEnum;

struct IBufferLock : public IUnknown {
	virtual HRESULT LockBuffer(LONG lDefaultStride, DWORD dwHeightInPixels, BYTE **ppbScanLine0, LONG *plStride) = 0;
	virtual void UnlockBuffer() = 0;
};

//
// Our very simple COM-like wrapper interface for camera access using DirectShow interfaces.
//
struct ICameraDs : public IUnknown {
	virtual HRESULT Initialize(wchar_t *pszFriendlyName) = 0;
	virtual HRESULT IsSystemCamera(wchar_t *pszFriendlyName, PBOOL pbSysCam, wchar_t *pszDevPath, DWORD cchDevPathSize) = 0;
	virtual HRESULT SetPrivacy(LONG lNewValue) = 0;
	virtual HRESULT GetPrivacy(LONG *plValue, LONG *plFlags) = 0;
	virtual HRESULT SetBrightness(LONG lNewValue) = 0;
	virtual HRESULT GetBrightness(LONG *plValue, LONG *plFlags) = 0;
	virtual HRESULT SetContrast(LONG lNewValue) = 0;
	virtual HRESULT GetContrast(LONG *plValue, LONG *plFlags) = 0;
	virtual HRESULT SetSaturation(LONG lNewValue) = 0;
	virtual HRESULT GetSaturation(LONG *plValue, LONG *plFlags) = 0;
	virtual HRESULT SetExposure(LONG lNewValue) = 0;
	virtual HRESULT GetExposure(LONG *plValue, LONG *plFlags) = 0;
	virtual HRESULT SetFlash(CameraFlashEnum newValue) = 0;
	virtual HRESULT GetFlash(CameraFlashEnum *pValue, LONG *plFlags) = 0;
	virtual HRESULT GetBrightnessRange(LONG *plMin, LONG *plMax, LONG *plDelta, LONG *plDefault, LONG *plCaps) = 0;
	virtual HRESULT GetContrastRange(LONG *plMin, LONG *plMax, LONG *plDelta, LONG *plDefault, LONG *plCaps) = 0;
	virtual HRESULT GetSaturationRange(LONG *plMin, LONG *plMax, LONG *plDelta, LONG *plDefault, LONG *plCaps) = 0;
	virtual HRESULT GetExposureRange(LONG *plMin, LONG *plMax, LONG *plDelta, LONG *plDefault, LONG *plCaps) = 0;
	virtual HRESULT GetCameraProvider(wchar_t *pszFriendlyName, wchar_t *pszProvider, DWORD cchProvSize) = 0;
	virtual HRESULT DumpCameraInfo(wchar_t *pszFriendlyName) = 0;
	virtual HRESULT LaunchPropertiesFrame() = 0;
	virtual HRESULT CloseInterfaces() = 0;
};

//
// Our very simple COM-like wrapper interface for camera access using Media Foundation interfaces.
//
struct ICameraMf : public IUnknown {
	virtual HRESULT Initialize(LONG lWidth, LONG lHeight, FrameCallback pfnFrameCallback) = 0;
	virtual HRESULT GetFriendlyNames(wchar_t **ppFriendlyNames, LONG *pcbSize) = 0;
	virtual HRESULT StartRenderAsync(wchar_t *pszFriendlyName) = 0;
	virtual HRESULT StopRenderAsync() = 0;
	virtual HRESULT IsSystemCamera(wchar_t *pszFriendlyName, PBOOL pbSystemCamera, LONG *pIndex) = 0;
	virtual HRESULT MfDumpCameraInfo(wchar_t *pszFriendlyName) = 0;
};

#ifdef __cplusplus
extern "C" {
#endif

	_declspec(dllexport) HRESULT GetDefaultImageStride(IMFMediaType *pType, LONG *plStride);

	//
	// Factory functions.
	//
	_declspec(dllexport) HRESULT CreateBufferLockInstance(IMFMediaBuffer *pBuffer, IBufferLock **ppObj);
	_declspec(dllexport) HRESULT CreateCameraDsInstance(ICameraDs **ppObj);
	_declspec(dllexport) HRESULT CreateCameraMfInstance(ICameraMf **ppObj);
	_declspec(dllexport) HRESULT CreateWebcamWrapperInstance(IWebcamWrapper **ppObj);

#ifdef __cplusplus
}
#endif