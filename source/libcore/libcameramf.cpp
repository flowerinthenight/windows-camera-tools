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
#include <Windows.h>
#include <Shlwapi.h>
#include <comdef.h>
#include "..\include\libcamera.h"
#include "..\include\libcore.h"
#include <strsafe.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <Mferror.h>
#include "..\trace\trace.h"

#pragma comment(lib,"WtsApi32.lib")
#pragma comment(lib,"UserEnv.lib")
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mf.lib")
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"shlwapi.lib")

_declspec(dllexport) HRESULT GetDefaultImageStride(IMFMediaType *pType, LONG *plStride)
{
	LONG lStride = 0;

	//
	// Try to get the default stride from the media type.
	//
	HRESULT hr = pType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&lStride);

	if (FAILED(hr))
	{
		GUID subType = GUID_NULL;
		UINT32 width = 0;
		UINT32 height = 0;

		//
		// Get the subtype and the image size.
		//
		hr = pType->GetGUID(MF_MT_SUBTYPE, &subType);
		if (SUCCEEDED(hr)) hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
		if (SUCCEEDED(hr)) hr = MFGetStrideForBitmapInfoHeader(subType.Data1, width, &lStride);

		//
		// Set the attribute for later reference.
		//
		if (SUCCEEDED(hr)) (void)pType->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(lStride));
	}

	if (SUCCEEDED(hr)) *plStride = lStride;

	return hr;
}

typedef HRESULT(*COPY_TRANSFORM_FN)(BYTE*, LONG, BYTE*, LONG, UINT, UINT);

struct ChooseDeviceParam {
	IMFActivate **ppDevices;		// Array of IMFActivate pointers.
	UINT32 count;					// Number of elements in the array.
	UINT32 selection;				// Selected device, by array index.
};

struct ConversionFunction {
	BOOL bSelect;
	GUID guidSubType;
	LPCWSTR pwszGuidSubType;
	COPY_TRANSFORM_FN FnCopyTransform;
};

ConversionFunction FormatConversions[] = {
	/* Uncompressed RGB formats */
	{ FALSE, MFVideoFormat_RGB8, L"MFVideoFormat_RGB8", NULL },
	{ FALSE, MFVideoFormat_RGB555, L"MFVideoFormat_RGB555", NULL },
	{ FALSE, MFVideoFormat_RGB565, L"MFVideoFormat_RGB565", NULL },
	{ FALSE, MFVideoFormat_RGB32, L"MFVideoFormat_RGB32", NULL },
	{ FALSE, MFVideoFormat_RGB24, L"MFVideoFormat_RGB24", NULL },
	{ FALSE, MFVideoFormat_ARGB32, L"MFVideoFormat_ARGB32", NULL },
	/* YUV Formats: 8-Bit and Palettized */
	{ FALSE, MFVideoFormat_AI44, L"MFVideoFormat_AI44", NULL },
	{ FALSE, MFVideoFormat_AYUV, L"MFVideoFormat_AYUV", NULL },
	{ FALSE, MFVideoFormat_I420, L"MFVideoFormat_I420", NULL },
	{ FALSE, MFVideoFormat_IYUV, L"MFVideoFormat_IYUV", NULL },
	{ FALSE, MFVideoFormat_NV11, L"MFVideoFormat_NV11", NULL },
	{ FALSE, MFVideoFormat_NV12, L"MFVideoFormat_NV12", NULL },
	{ FALSE, MFVideoFormat_UYVY, L"MFVideoFormat_UYVY", NULL },
	{ FALSE, MFVideoFormat_Y41P, L"MFVideoFormat_Y41P", NULL },
	{ FALSE, MFVideoFormat_Y41T, L"MFVideoFormat_Y41T", NULL },
	{ FALSE, MFVideoFormat_Y42T, L"MFVideoFormat_Y42T", NULL },
	{ FALSE, MFVideoFormat_YUY2, L"MFVideoFormat_YUY2", NULL /*FnCopyYUY2ToRGB32*/ },
	{ FALSE, MFVideoFormat_YV12, L"MFVideoFormat_YV12", NULL },
	/* YUV Formats: 10-Bit and 16-Bit */
	{ FALSE, MFVideoFormat_P010, L"MFVideoFormat_P010", NULL },
	{ FALSE, MFVideoFormat_P016, L"MFVideoFormat_P016", NULL },
	{ FALSE, MFVideoFormat_P210, L"MFVideoFormat_P210", NULL },
	{ FALSE, MFVideoFormat_P216, L"MFVideoFormat_P216", NULL },
	{ FALSE, MFVideoFormat_v210, L"MFVideoFormat_v210", NULL },
	{ FALSE, MFVideoFormat_v216, L"MFVideoFormat_v216", NULL },
	{ FALSE, MFVideoFormat_v410, L"MFVideoFormat_v410", NULL },
	{ FALSE, MFVideoFormat_Y210, L"MFVideoFormat_Y210", NULL },
	{ FALSE, MFVideoFormat_Y216, L"MFVideoFormat_Y216", NULL },
	{ FALSE, MFVideoFormat_Y410, L"MFVideoFormat_Y410", NULL },
	{ FALSE, MFVideoFormat_Y416, L"MFVideoFormat_Y416", NULL },
	/* Encoded Video Types */
	{ FALSE, MFVideoFormat_DV25, L"MFVideoFormat_DV25", NULL },
	{ FALSE, MFVideoFormat_DV50, L"MFVideoFormat_DV50", NULL },
	{ FALSE, MFVideoFormat_DVC, L"MFVideoFormat_DVC", NULL },
	{ FALSE, MFVideoFormat_DVH1, L"MFVideoFormat_DVH1", NULL },
	{ FALSE, MFVideoFormat_DVHD, L"MFVideoFormat_DVHD", NULL },
	{ FALSE, MFVideoFormat_DVSD, L"MFVideoFormat_DVSD", NULL },
	{ FALSE, MFVideoFormat_DVSL, L"MFVideoFormat_DVSL", NULL },
	{ FALSE, MFVideoFormat_H264, L"MFVideoFormat_H264", NULL },
	{ FALSE, MFVideoFormat_M4S2, L"MFVideoFormat_M4S2", NULL },
	{ FALSE, MFVideoFormat_MJPG, L"MFVideoFormat_MJPG", NULL },
	{ FALSE, MFVideoFormat_MP43, L"MFVideoFormat_MP43", NULL },
	{ FALSE, MFVideoFormat_MP4S, L"MFVideoFormat_MP4S", NULL },
	{ FALSE, MFVideoFormat_MP4V, L"MFVideoFormat_MP4V", NULL },
	{ FALSE, MFVideoFormat_MPEG2, L"MFVideoFormat_MPEG2", NULL },
	{ FALSE, MFVideoFormat_MPG1, L"MFVideoFormat_MPG1", NULL },
	{ FALSE, MFVideoFormat_MSS1, L"MFVideoFormat_MSS1", NULL },
	{ FALSE, MFVideoFormat_MSS2, L"MFVideoFormat_MSS2", NULL },
	{ FALSE, MFVideoFormat_WMV1, L"MFVideoFormat_WMV1", NULL },
	{ FALSE, MFVideoFormat_WMV2, L"MFVideoFormat_WMV2", NULL },
	{ FALSE, MFVideoFormat_WMV3, L"MFVideoFormat_WMV3", NULL },
	{ FALSE, MFVideoFormat_WVC1, L"MFVideoFormat_WVC1", NULL },
};

class CMFCameraSource : public IMFSourceReaderCallback {
public:
	static HRESULT CreateInstance(LONG lWidth, LONG lHeight, FrameCallback pfnCallback, CMFCameraSource **ppSource)
	{
		if (ppSource == NULL) return E_POINTER;

		CMFCameraSource *pSource = new (std::nothrow) CMFCameraSource();
		if (pSource == NULL) return E_OUTOFMEMORY;

		HRESULT hr = pSource->Initialize(lWidth, lHeight, pfnCallback);

		if (SUCCEEDED(hr))
		{
			*ppSource = pSource;

			//
			// Client should Release after using this instance.
			//
			(*ppSource)->AddRef();
		}

		return hr;
	}

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface(REFIID iid, void **ppv)
	{
		static const QITAB qit[] =
		{
			QITABENT(CMFCameraSource, IMFSourceReaderCallback),
			{ 0 },
		};

		return QISearch(this, qit, iid, ppv);
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_lRefCount);
	}

	STDMETHODIMP_(ULONG) Release()
	{
		ULONG uCount = InterlockedDecrement(&m_lRefCount);

		if (uCount == 0)
		{
			delete this;
		}

		return uCount;
	}

	//
	// IMFSourceReaderCallback methods
	//
	STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample)
	{
		HRESULT hr = hrStatus;
		IMFMediaBuffer *pBuffer = NULL;
		DWORD dwStream = (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM;

		EnterCriticalSection(&m_csLock);

		if (m_pfnFrameCallback)
		{
			//
			// We just call the provided callback if available. We will continue to request for more frames if callback will
			// return S_OK.
			//
			hr = m_pfnFrameCallback(hrStatus, dwStreamIndex, dwStreamFlags, llTimestamp, pSample);
		}

		//
		// Request the next frame. If callback function returns != S_OK, then streaming will be stopped.
		//
		if (SUCCEEDED(hr))
		{
			hr = m_pIReader->ReadSample(dwStream, 0, NULL, NULL, NULL, NULL);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"ReadSample", hr);
			}
		}

		LeaveCriticalSection(&m_csLock);

		return hr;
	}

	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent*) { return S_OK; }
	STDMETHODIMP OnFlush(DWORD) { return S_OK; }

	HRESULT SetDevice(IMFActivate *pActivate)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;

		HRESULT hr = S_OK;
		IMFMediaSource *pSource = NULL;
		IMFAttributes *pAttributes = NULL;
		IMFMediaType *pType = NULL;
		IMFMediaType *pOutType = NULL;
		wchar_t szTrace[MAX_PATH] = { 0 };

		DWORD dwStream = (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM;

		EnterCriticalSection(&m_csLock);

		do
		{
			//
			// Create the media source for the device...
			//
			hr = pActivate->ActivateObject(__uuidof(IMFMediaSource), (void**)&pSource);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"ActivateObject", hr);
				break;
			}

			//
			// Get the symbolic link...
			//
			hr = pActivate->GetAllocatedString(
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
				&m_pszSymbolicLink,
				&m_uiSymbolicLinkLen);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"GetAllocatedString", hr);
				break;
			}

			//
			// Create an attribute store to hold initialization settings...
			//
			hr = MFCreateAttributes(&pAttributes, 2);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"MFCreateAttributes", hr);
				break;
			}

			hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"SetUINT32", hr);
				break;
			}

			//
			// Set the callback pointer to this object...
			//
			hr = pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"SetUnknown", hr);
				break;
			}

			//
			// Then create the source reader...
			//
			hr = MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &m_pIReader);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"MFCreateSourceReaderFromMediaSource", hr);
				break;
			}

			//
			// Try to find a suitable output type...
			//
			BOOL bFound = FALSE;

			for (DWORD j = 0;; j++)
			{
				hr = m_pIReader->GetNativeMediaType(dwStream, j, &pType);

				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"GetNativeMediaType", hr);
					break;
				}

				GUID subType = { 0 };
				hr = pType->GetGUID(MF_MT_SUBTYPE, &subType);

				//
				// Check if its in our table of supported types...
				//
				for (DWORD k = 0; k < ARRAYSIZE(FormatConversions); k++)
				{
					if (subType == FormatConversions[k].guidSubType)
					{
						UINT32 uFrNume, uFrDeno;
						UINT32 uParNume, uParDeno;
						UINT32 uiTempX, uiTempY;

						MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &uiTempX, &uiTempY);
						MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, &uFrNume, &uFrDeno);
						MFGetAttributeRatio(pType, MF_MT_PIXEL_ASPECT_RATIO, &uParNume, &uParDeno);

						hr = GetDefaultImageStride(pType, &m_lDefaultStride);

						StringCchPrintf(szTrace, MAX_PATH, L"Native(j,k):(%d,%d) %s, W: %d, H: %d, Stride: %d, F.Rate: "
							L"%d:%d, PAR: %d:%d",
							j,
							k,
							FormatConversions[k].pwszGuidSubType,
							m_uiWidth,
							m_uiHeight,
							m_lDefaultStride,
							uFrNume,
							uFrDeno,
							uParNume,
							uParDeno);

						EventWriteInfoW(M, FL, FN, szTrace);

						if (FormatConversions[k].guidSubType == MFVideoFormat_YUY2 &&
							uiTempX == m_uiWidth && uiTempY == m_uiHeight)
						{
							// __TRACE(pTrace, TL_INFO, KW_INFO, L"Set YUY2 as media subtype format.");

							hr = m_pIReader->SetCurrentMediaType(dwStream, NULL, pType);

							if (FAILED(hr))
							{
								EventWriteHresultError(M, FL, FN, L"SetCurrentMediaType", hr);
							}

							bFound = TRUE;
							break;
						}
					}
				}

				SafeRelease(&pType);
				if (bFound) break;
				if (hr == MF_E_NO_MORE_TYPES || hr == MF_E_INVALIDSTREAMNUMBER) break;
			}

			if (!bFound)
			{
				//
				// Set output media type to RGB32. Whatever native type is supported in our source, source reader will automatically
				// insert required decoders/transforms in the pipeline.
				//
				// __TRACE(pTrace, TL_INFO, KW_INFO, L"Use native as media type format.");

				hr = MFCreateMediaType(&pOutType);

				if (FAILED(hr))
				{					
					EventWriteHresultError(M, FL, FN, L"MFCreateMediaType", hr);
				}

				hr = pOutType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);

				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"SetGUID(MF_MT_MAJOR_TYPE...)", hr);					
				}

				hr = pOutType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);

				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"SetGUID(MF_MT_SUBTYPE...)", hr);					
				}

				hr = MFSetAttributeSize(pOutType, MF_MT_FRAME_SIZE, m_uiWidth, m_uiHeight);

				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"MFSetAttributeSize", hr);					
				}

				hr = m_pIReader->SetCurrentMediaType(dwStream, NULL, pOutType);

				if (FAILED(hr))
				{					
					EventWriteHresultError(M, FL, FN, L"SetCurrentMediaType", hr);
				}

				SafeRelease(&pOutType);
			}

			if (SUCCEEDED(hr))
			{
				//
				// Ask for the first sample. This call will cause our OnReadSample callback to be called back. Call ReadSample
				// again within the callback function to get the next frame and so on.
				//
				hr = m_pIReader->ReadSample(dwStream, 0, NULL, NULL, NULL, NULL);

				if (FAILED(hr))
				{					
					EventWriteHresultError(M, FL, FN, L"ReadSample", hr);
				}
			}
		}
		while (false);

		SafeRelease(&pSource);
		SafeRelease(&pAttributes);
		SafeRelease(&pType);
		SafeRelease(&pOutType);

		LeaveCriticalSection(&m_csLock);

		return hr;
	}

	HRESULT CloseDevice()
	{
		EnterCriticalSection(&m_csLock);
		SafeRelease(&m_pIReader);

		CoTaskMemFree(m_pszSymbolicLink);
		m_pszSymbolicLink = NULL;
		m_uiSymbolicLinkLen = 0;

		LeaveCriticalSection(&m_csLock);

		return S_OK;
	}

	HRESULT ResizeVideo(WORD wWidth, WORD wHeight)
	{
		return E_NOTIMPL;
	}

protected:
	CMFCameraSource()
	{
		InitializeCriticalSection(&m_csLock);

		m_lRefCount = 0;
		m_pszSymbolicLink = NULL;
		m_uiSymbolicLinkLen = 0;

		m_pIReader = NULL;

		m_uiWidth = m_uiHeight = m_lDefaultStride = 0;
		ZeroMemory(&m_mfrAspectRatio, sizeof(MFRatio));
	}

	virtual ~CMFCameraSource()
	{
		DeleteCriticalSection(&m_csLock);
	}

	HRESULT Initialize(LONG lWidth, LONG lHeight, FrameCallback pfnCallback)
	{
		m_uiWidth = (UINT)lWidth;
		m_uiHeight = (UINT)lHeight;
		m_pfnFrameCallback = pfnCallback;
		m_bInitialized = TRUE;
		return S_OK;
	}

protected:
	LONG m_lRefCount;
	CRITICAL_SECTION m_csLock;
	IMFSourceReader *m_pIReader;
	wchar_t *m_pszSymbolicLink;
	UINT32 m_uiSymbolicLinkLen;
	UINT m_uiWidth;
	UINT m_uiHeight;
	LONG m_lDefaultStride;
	MFRatio m_mfrAspectRatio;
	FrameCallback m_pfnFrameCallback;
	BOOL m_bInitialized;
};

class CameraMf : public ICameraMf {
public:
	CameraMf() :
		m_lRefCount(0),
		m_bInitialized(FALSE),
		m_pfnFrameCallback(NULL),
		m_lWidth(640),
		m_lHeight(480),
		m_pCamSource(NULL)
	{}

	virtual ~CameraMf()
	{
		if (m_bRendered) StopRenderAsync();
		SafeRelease(&m_pCamSource);
	}

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface(REFIID iid, void **ppv)
	{
		if ((iid == __uuidof(IUnknown)) || (iid == __uuidof(ICameraMf)))
		{
			*ppv = static_cast<CameraMf*>(this);
		}
		else
		{
			*ppv = NULL;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_lRefCount);
	}

	STDMETHODIMP_(ULONG) Release()
	{
		ULONG uCount = InterlockedDecrement(&m_lRefCount);

		if (uCount == 0)
		{
			delete this;
		}

		return uCount;
	}

	//
	// ICameraMf methods.
	//
	HRESULT Initialize(LONG lWidth, LONG lHeight, FrameCallback pfnFrameCallback)
	{
		m_lWidth = lWidth;
		m_lHeight = lHeight;
		m_pfnFrameCallback = pfnFrameCallback;

		HRESULT hr = CMFCameraSource::CreateInstance(m_lWidth, m_lHeight, m_pfnFrameCallback, &m_pCamSource);

		if (SUCCEEDED(hr)) m_bInitialized = TRUE;

		return hr;
	}

	//
	// Param 'ppFriendlyNames' will be allocated here based on the required size. Caller should call free on 'ppFriendlyNames'
	// after call. If more than one cameras detected, names will be separated by a semicolon (;).
	//
	HRESULT GetFriendlyNames(wchar_t **ppFriendlyNames, LONG *pcbSize)
	{
		ChooseDeviceParam param = { 0 };
		IMFAttributes *pAttributes = NULL;
		BOOL bReturn = FALSE;
		HRESULT hr = E_FAIL;
		DWORD dwDevIndex = -1;
		wchar_t *pszOldNames = NULL;
		wchar_t *pszNames = NULL;
		size_t cchTotal = 0;

		do
		{
			*pcbSize = 0;

			//
			// Initialize an attribute store to specify enumeration parameters.
			//
			hr = MFCreateAttributes(&pAttributes, 1);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"MFCreateAttributes", hr);
				break;
			}

			//
			// Ask for source type = video capture devices.
			//
			hr = pAttributes->SetGUID(
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"SetGUID", hr);
				break;
			}

			//
			// Enumerate devices.
			//
			hr = MFEnumDeviceSources(pAttributes, &param.ppDevices, &param.count);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"MFEnumDeviceSources", hr);
				break;
			}

			if (param.count)
			{
				BOOL bSuccess = TRUE;

				for (DWORD i = 0; i < param.count; i++)
				{
					wchar_t *pszFriendlyNameTmp = NULL;

#pragma warning(suppress: 6387)
					hr = param.ppDevices[i]->GetAllocatedString(
						MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
						&pszFriendlyNameTmp,
						NULL);

					if (FAILED(hr))
					{
						EventWriteHresultError(M, FL, FN, L"HrError MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME", hr);
						bSuccess = FALSE;
						break;
					}

					if (pszFriendlyNameTmp)
					{
						EventWriteWideStrInfo(M, FL, FN, L"FriendlyName", pszFriendlyNameTmp);

						size_t cchLen = 0;

						StringCchLength(pszFriendlyNameTmp, MAX_PATH, &cchLen);
						EventWriteNumberInfo(M, FL, FN, L"cchLen", cchLen);
						
						cchTotal += cchLen + 2;

						pszOldNames = pszNames;

						pszNames = (wchar_t*)realloc(pszNames, cchTotal * sizeof(wchar_t));

						if (pszNames)
						{
							if (i == 0)
							{
								//
								// So 'StringCchCat' will work on first concatenate.
								//
								pszNames[0] = L'\0';
							}

							StringCchCat(pszNames, cchTotal, pszFriendlyNameTmp);
							StringCchCat(pszNames, cchTotal, L";");							
						}
						else
						{
							EventWriteLastError(M, FL, FN, L"Error realloc", GetLastError());
							free(pszOldNames);
							bSuccess = FALSE;
							break;
						}

						CoTaskMemFree(pszFriendlyNameTmp);
					}
				}

				if (bSuccess)
				{
					size_t cchLen = 0, alloc = 0;

					StringCchLength(pszNames, cchTotal, &cchLen);
					EventWriteNumberInfo(M, FL, FN, L"cchLen (1st)", cchLen);

					//
					// Remove the last semicolon.
					//
					pszNames[cchLen - 1] = L'\0';
					EventWriteWideStrInfo(M, FL, FN, L"Name list", pszNames);

					StringCchLength(pszNames, cchTotal, &cchLen);
					EventWriteNumberInfo(M, FL, FN, L"cchLen (2nd)", cchLen);

					alloc = (cchLen * sizeof(wchar_t)) + sizeof(wchar_t);

					*ppFriendlyNames = (wchar_t*)malloc(alloc);

					if (*ppFriendlyNames)
					{
						StringCchCopy(*ppFriendlyNames, alloc, pszNames);
						*pcbSize = alloc;
					}
				}
			}
		}
		while (false);

		if (pszNames) free(pszNames);

		SafeRelease(&pAttributes);

		for (DWORD i = 0; i < param.count; i++)
		{
			SafeRelease(&param.ppDevices[i]);
		}

		CoTaskMemFree(param.ppDevices);

		return hr;
	}

	HRESULT StartRenderAsync(wchar_t *pszFriendlyName)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;

		ChooseDeviceParam param = { 0 };
		IMFAttributes *pAttributes = NULL;
		BOOL bCamPresent = FALSE;
		BOOL bReturn = FALSE;
		HRESULT hr = S_OK;
		DWORD dwDevIndex = -1;
		m_bRendered = FALSE;

		do
		{
			//
			// Initialize an attribute store to specify enumeration parameters.
			//
			hr = MFCreateAttributes(&pAttributes, 1);

			if (FAILED(hr))
			{				
				EventWriteHresultError(M, FL, FN, L"MFCreateAttributes", hr);
				break;
			}

			//
			// Ask for source type = video capture devices.
			//
			hr = pAttributes->SetGUID(
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"pAttributes->SetGUID", hr);				
				break;
			}

			//
			// Enumerate devices.
			//
			hr = MFEnumDeviceSources(pAttributes, &param.ppDevices, &param.count);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"MFEnumDeviceSources", hr);				
				break;
			}

			if (param.count)
			{
				for (DWORD i = 0; i < param.count; i++)
				{
					wchar_t *pszFriendlyNameTmp = NULL;

#pragma warning(suppress: 6387)
					hr = param.ppDevices[i]->GetAllocatedString(
						MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
						&pszFriendlyNameTmp,
						NULL);

					if (FAILED(hr)) break;

					if (pszFriendlyNameTmp)
					{
						EventWriteWideStrInfo(M, FL, FN, L"FriendlyName", pszFriendlyNameTmp);

						if ((_wcsicmp(pszFriendlyNameTmp, pszFriendlyName)) == 0)
						{
							bCamPresent = TRUE;
							dwDevIndex = i;
							break;
						}

						CoTaskMemFree(pszFriendlyNameTmp);
					}
				}
			}

			if (bCamPresent)
			{
				hr = m_pCamSource->SetDevice(param.ppDevices[dwDevIndex]);
				
				if (FAILED(hr))
				{
					m_bRendered = FALSE;
				}
				else
				{
					m_bRendered = TRUE;
					bReturn = TRUE;
				}
			}
			else
			{				
				EventWriteErrorW(M, FL, FN, L"Camera not available! Maybe disabled, not installed, or used?");
			}
		}
		while (false);

		SafeRelease(&pAttributes);

		for (DWORD i = 0; i < param.count; i++)
			SafeRelease(&param.ppDevices[i]);

		CoTaskMemFree(param.ppDevices);

		return S_OK;
	}

	HRESULT StopRenderAsync()
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;

		m_pCamSource->CloseDevice();
		m_bRendered = FALSE;

		return S_OK;
	}

	HRESULT IsSystemCamera(wchar_t *pszFriendlyName, PBOOL pbSystemCamera, LONG *pIndex)
	{
		//
		// We just do camera enumeration here so no need for initialization. We just go through the whole process
		// or open, read and close.
		//

		ChooseDeviceParam param = { 0 };
		IMFAttributes *pAttributes = NULL;
		BOOL bCamPresent = FALSE;
		BOOL bReturn = FALSE;
		HRESULT hr = E_FAIL;
		LONG lDevIndex = -1;

		do
		{
			*pbSystemCamera = FALSE;

			//
			// Initialize an attribute store to specify enumeration parameters.
			//
			hr = MFCreateAttributes(&pAttributes, 1);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"MFCreateAttributes", hr);
				break;
			}

			//
			// Ask for source type = video capture devices.
			//
			hr = pAttributes->SetGUID(
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"SetGUID", hr);
				break;
			}

			//
			// Enumerate devices.
			//
			hr = MFEnumDeviceSources(pAttributes, &param.ppDevices, &param.count);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"MFEnumDeviceSources", hr);
				break;
			}

			if (param.count)
			{
				for (DWORD i = 0; i < param.count; i++)
				{
					wchar_t *pszFriendlyNameTmp = NULL;

#pragma warning(suppress: 6387)
					hr = param.ppDevices[i]->GetAllocatedString(
						MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
						&pszFriendlyNameTmp,
						NULL);

					if (FAILED(hr)) break;

					if (pszFriendlyNameTmp)
					{
						EventWriteWideStrInfo(M, FL, FN, L"FriendlyName", pszFriendlyNameTmp);

						if ((_wcsicmp(pszFriendlyNameTmp, pszFriendlyName)) == 0)
						{
							bCamPresent = TRUE;
							lDevIndex = i;
							*pIndex = i;

							EventWriteNumberInfo(M, FL, FN, L"Found. Index @", lDevIndex);

							break;
						}

						CoTaskMemFree(pszFriendlyNameTmp);
					}
				}
			}

			if (bCamPresent)
			{
				*pbSystemCamera = TRUE;
				hr = S_OK;
			}
			else
			{
				EventWriteWideStrInfo(M, FL, FN, L"Not available", pszFriendlyName);
			}
		}
		while (false);

		SafeRelease(&pAttributes);

		for (DWORD i = 0; i < param.count; i++)
		{
			SafeRelease(&param.ppDevices[i]);
		}

		CoTaskMemFree(param.ppDevices);

		return hr;
	}

	HRESULT MfDumpCameraInfo(wchar_t *pszFriendlyName)
	{
		HRESULT hr = MFStartup(MF_VERSION);

		if (FAILED(hr))
		{
			EventWriteHresultError(M, FL, FN, L"MFStartup", hr);
			return hr;
		}

		ChooseDeviceParam param = { 0 };
		IMFAttributes *pAttributes = NULL;
		BOOL bCamPresent = FALSE;
		BOOL bReturn = FALSE;
		DWORD dwDevIndex = -1;
		wchar_t szTrace[MAX_PATH] = { 0 };

		do
		{
			//
			// Initialize an attribute store to specify enumeration parameters.
			//
			hr = MFCreateAttributes(&pAttributes, 1);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"MFCreateAttributes", hr);
				break;
			}

			//
			// Ask for source type = video capture devices.
			//
			hr = pAttributes->SetGUID(
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

			if (FAILED(hr))
			{				
				EventWriteHresultError(M, FL, FN, L"pAttributes->SetGUID", hr);
				break;
			}

			//
			// Enumerate devices.
			//
			hr = MFEnumDeviceSources(pAttributes, &param.ppDevices, &param.count);

			if (FAILED(hr))
			{				
				EventWriteHresultError(M, FL, FN, L"MFEnumDeviceSources", hr);
				break;
			}

			if (param.count)
			{
				for (DWORD i = 0; i < param.count; i++)
				{
					wchar_t *pszFriendlyName = NULL;

#pragma warning(suppress: 6387)
					hr = param.ppDevices[i]->GetAllocatedString(
						MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
						&pszFriendlyName,
						NULL);

					if (FAILED(hr) || !pszFriendlyName) break;

					if ((_wcsicmp(pszFriendlyName, L"Integrated Camera")) == 0)
					{
						bCamPresent = TRUE;
						dwDevIndex = i;
						CoTaskMemFree(pszFriendlyName);
						break;
					}

					CoTaskMemFree(pszFriendlyName);
				}
			}

			if (bCamPresent)
			{
				//
				// Enumerate supported resolutions
				//
				IMFMediaSource *pSource = NULL;
				IMFAttributes *pAttrib = NULL;
				IMFMediaType *pType = NULL;
				IMFMediaType *pOutType = NULL;
				IMFActivate *pActivate = param.ppDevices[dwDevIndex];
				IMFSourceReader *pIReader = NULL;
				wchar_t *pszSymbolicLink;
				UINT32 uiSymbolicLinkLen;
				UINT ctr1, ctr2, ctr3;

				DWORD dwStream = (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM;

				//
				// Create the media source for the device...
				//
				hr = pActivate->ActivateObject(__uuidof(IMFMediaSource), (void**)&pSource);

				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"ActivateObject", hr);
					break;
				}

				//
				// Get the symbolic link...
				//
				hr = pActivate->GetAllocatedString(
					MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
					&pszSymbolicLink,
					&uiSymbolicLinkLen);

				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"GetAllocatedString", hr);
					break;
				}

				//
				// Create an attribute store to hold initialization settings...
				//
				hr = MFCreateAttributes(&pAttrib, 2);

				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"MFCreateAttributes", hr);
					break;
				}

				hr = pAttrib->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"SetUINT32", hr);			
					break;
				}

				//
				// Then create the source reader...
				//
				hr = MFCreateSourceReaderFromMediaSource(pSource, pAttrib, &pIReader);

				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"MFCreateSourceReaderFromMediaSource", hr);					
					break;
				}

				//
				// Try to find a suitable output type...
				//
				BOOL bFound = FALSE;

				for (ctr1 = 0;; ctr1++)
				{
					hr = pIReader->GetNativeMediaType(dwStream, ctr1, &pType);

					if (FAILED(hr))
					{						
						EventWriteHresultError(M, FL, FN, L"GetNativeMediaType", hr);
						break;
					}

					GUID subType = { 0 };
					hr = pType->GetGUID(MF_MT_SUBTYPE, &subType);

					//
					// Check if its in our table of supported types...
					//
					for (DWORD k = 0; k < ARRAYSIZE(FormatConversions); k++)
					{
						if (subType == FormatConversions[k].guidSubType)
						{
							UINT32 uFrNume, uFrDeno;
							UINT32 uParNume, uParDeno;
							UINT uiWidth, uiHeight;
							LONG lDefaultStride;
							MFRatio mfrAspectRatio;

							MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &uiWidth, &uiHeight);
							MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, &uFrNume, &uFrDeno);
							MFGetAttributeRatio(pType, MF_MT_PIXEL_ASPECT_RATIO, &uParNume, &uParDeno);
							hr = GetDefaultImageStride(pType, &lDefaultStride);

							StringCchPrintf(
								szTrace,
								MAX_PATH,
								L"%s, W: %d, H: %d, Stride: %d, F.Rate: "
								L"%d:%d, PAR: %d:%d",
								FormatConversions[k].pwszGuidSubType,
								uiWidth,
								uiHeight,
								lDefaultStride,
								uFrNume,
								uFrDeno,
								uParNume,
								uParDeno);

							EventWriteInfoW(M, FL, FN, szTrace);
						}
					}

					SafeRelease(&pType);

					if (hr == MF_E_NO_MORE_TYPES || hr == MF_E_INVALIDSTREAMNUMBER) break;
				}

				if (hr == MF_E_NO_MORE_TYPES) hr = S_OK;

				SafeRelease(&pActivate);
				SafeRelease(&pIReader);

				CoTaskMemFree(pszSymbolicLink);

				if (pSource) pSource->Shutdown();
				SafeRelease(&pSource);

				SafeRelease(&pAttributes);
				SafeRelease(&pType);
				SafeRelease(&pOutType);
			}
		}
		while (false);

		SafeRelease(&pAttributes);

		for (DWORD i = 0; i < param.count; i++)
		{
			SafeRelease(&param.ppDevices[i]);
		}

		CoTaskMemFree(param.ppDevices);

		hr = MFShutdown();

		if (FAILED(hr))
		{
			EventWriteHresultError(M, FL, FN, L"MFShutdown", hr);
		}

		return hr;
	}

private:
	BOOL m_bRendered;
	BOOL m_bInitialized;
	LONG m_lRefCount;
	FrameCallback m_pfnFrameCallback;
	LONG m_lWidth;
	LONG m_lHeight;
	CMFCameraSource *m_pCamSource;
};

//
// Our exported CameraMf factory function.
//
_declspec(dllexport) HRESULT CreateCameraMfInstance(ICameraMf **ppObj)
{
	if (ppObj == NULL) return E_POINTER;

	ICameraMf *pObj = new (std::nothrow) CameraMf();
	if (pObj == NULL) return E_OUTOFMEMORY;

	*ppObj = pObj;

	(*ppObj)->AddRef();

	return S_OK;
}
