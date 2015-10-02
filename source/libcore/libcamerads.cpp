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
#include "..\include\libcore.h"
#include "..\include\libcamera.h"
#include <Windows.h>
#include <comdef.h>
#include <Shlwapi.h>
#include <SetupAPI.h>
#include <ksproxy.h>
#include <strsafe.h>
#include <mfapi.h>
#include "..\trace\trace.h"

#pragma comment(lib, "strmiids")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "mfplat.lib")

//
// NOTE: Clients should call CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) before using any functions in this class.
//
class CameraDs : public ICameraDs {
public:
	CameraDs() :
		m_pVih2(NULL),
		m_pVideoProcAmp(NULL),
		m_pStrmConfig(NULL),
		m_pSrcFilter(NULL),
		m_pSampleGrabberFilter(NULL),
		m_pMediaCtrl(NULL),
		m_pGraphBldr(NULL),
		m_pCameraControl(NULL),
		m_pCapGraphBldr(NULL),
		m_pIKsControl(NULL),
		m_bInitialized(FALSE),
		m_lRefCount(0)
	{
		EventWriteInfoW(M, FL, FN, L"Constructed [this] CameraDs");
	}

	virtual ~CameraDs()
	{
		//
		// DANGER: Sometimes, this can cause a crash especially when using smart pointers. Don't really know why. Better call
		// CloseInterfaces() manually before (auto)deleting this object.
		//
		CloseInterfaces();
	}

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface(REFIID iid, void **ppv)
	{
		if ((iid == __uuidof(IUnknown)) || (iid == __uuidof(ICameraDs)))
		{
			*ppv = static_cast<CameraDs*>(this);
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
			EventWriteInfoW(M, FL, FN, L"Deleted [this] CameraDs");
			delete this;
		}

		return uCount;
	}

	//
	// ICameraDs methods
	//
	HRESULT Initialize(wchar_t *pszFriendlyName)
	{
		HRESULT hr = S_OK;

		do
		{
			hr = FindCaptureDevice(pszFriendlyName);
			if (FAILED(hr)) break;

			hr = GetInterfaces();
			if (FAILED(hr)) break;

			m_bInitialized = TRUE;
		}
		while (false);

		hr = m_bInitialized ? S_OK : E_FAIL;

		return hr;
	}

	HRESULT IsSystemCamera(wchar_t *pszFriendlyName, PBOOL pbSysCam, wchar_t *pszDevPath, DWORD cchDevPathSize)
	{
		IBaseFilter **ppSrcFilter = &m_pSrcFilter;
		HRESULT hr = S_OK;

		IBaseFilter *pSrc = NULL;
		IMoniker *pMoniker = NULL;
		ICreateDevEnum *pDevEnum = NULL;
		IEnumMoniker *pClassEnum = NULL;

		VARIANT varName;
		VARIANT varName2;

		if (!ppSrcFilter) return E_POINTER;

		//
		// Create the system device enumerator
		//
		hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void**)&pDevEnum);
		if (FAILED(hr)) return hr;

		//
		// Create an enumerator for the video capture devices
		//
		hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);

		if (FAILED(hr))
		{
			SAFE_RELEASE(pDevEnum);
			return hr;
		}

		//
		// If there are no enumerators for the requested type, then CreateClassEnumerator will succeed,
		// but pClassEnum will be NULL.
		//
		if (pClassEnum == NULL)
		{
			SAFE_RELEASE(pClassEnum);
			SAFE_RELEASE(pDevEnum);
			hr = E_FAIL;
			return hr;
		}

		//
		// Use the first video capture device on the device list. Note that if the Next() call succeeds but there are no
		// monikers, it will return S_FALSE (which is not a failure). Therefore, we check that the return code is S_OK
		// instead of using SUCCEEDED() macro.
		//
		pClassEnum->Reset();

		ULONG cFetched;
		*pbSysCam = FALSE;

		while (hr = pClassEnum->Next(1, &pMoniker, &cFetched), hr == S_OK)
		{
			IPropertyBag *pPropBag = NULL;
			HRESULT hrTmp = E_FAIL;

#pragma warning(suppress: 6387)
			hrTmp = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
			if (FAILED(hrTmp))
			{
				SAFE_RELEASE(pMoniker);
				continue;
			}

			VariantInit(&varName);
			VariantInit(&varName2);

			hrTmp = pPropBag->Read(L"FriendlyName", &varName, 0);
			if (FAILED(hrTmp))
			{
				VariantClear(&varName);
				VariantClear(&varName2);
				SAFE_RELEASE(pPropBag);
				continue;
			}

			hrTmp = pPropBag->Read(L"DevicePath", &varName2, 0);
			if (FAILED(hrTmp))
			{
				VariantClear(&varName);
				VariantClear(&varName2);
				SAFE_RELEASE(pPropBag);
				continue;
			}

			if (varName.bstrVal == NULL || varName2.bstrVal == NULL)
			{
				VariantClear(&varName);
				VariantClear(&varName2);
				SAFE_RELEASE(pPropBag);
				continue;
			}

			if (_wcsicmp(pszFriendlyName, varName.bstrVal) != 0)
			{
				VariantClear(&varName);
				VariantClear(&varName2);
				SAFE_RELEASE(pPropBag);
				continue;
			}

			EventWriteWideStrInfo(M, FL, FN, L"Name", varName.bstrVal);
			EventWriteWideStrInfo(M, FL, FN, L"DevPath", varName2.bstrVal);

			(void)StringCchCopy(pszDevPath, cchDevPathSize, varName2.bstrVal);

			VariantClear(&varName);
			VariantClear(&varName2);
			SAFE_RELEASE(pPropBag);

			*pbSysCam = TRUE;
			break;
		}

		SAFE_RELEASE(pClassEnum);
		SAFE_RELEASE(pDevEnum);
		SAFE_RELEASE(pMoniker);

		return hr;
	}

	HRESULT SetPrivacy(LONG lNewValue)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		if (!m_pCameraControl) return E_POINTER;

		EventWriteNumberInfo(M, FL, FN, L"NewValue", lNewValue);

		return m_pCameraControl->Set(KSPROPERTY_CAMERACONTROL_PRIVACY, lNewValue, CameraControl_Flags_Manual);
	}

	HRESULT GetPrivacy(LONG *plValue, LONG *plFlags)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		if (!m_pCameraControl) return E_POINTER;

		HRESULT hr = E_FAIL;

		//
		// Note: lValue will be valid only when hr = S_OK.
		//
		hr = m_pCameraControl->Get(KSPROPERTY_CAMERACONTROL_PRIVACY, plValue, plFlags);

		EventWriteNumberInfo(M, FL, FN, L"Value", *plValue);
		EventWriteHexInfo(M, FL, FN, L"Flags", *plFlags);
		EventWriteHexInfo(M, FL, FN, L"Status", hr);

		return hr;
	}

	HRESULT SetBrightness(LONG lNewValue)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		return SetVideoProcAmpParameter(KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS, lNewValue);
	}

	HRESULT GetBrightness(LONG *plValue, LONG *plFlags)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		return GetVideoProcAmpParameter(KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS, plValue, plFlags);
	}

	HRESULT SetContrast(LONG lNewValue)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		return SetVideoProcAmpParameter(KSPROPERTY_VIDEOPROCAMP_CONTRAST, lNewValue);
	}

	HRESULT GetContrast(LONG *plValue, LONG *plFlags)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		return GetVideoProcAmpParameter(KSPROPERTY_VIDEOPROCAMP_CONTRAST, plValue, plFlags);
	}

	HRESULT SetSaturation(LONG lNewValue)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		return SetVideoProcAmpParameter(KSPROPERTY_VIDEOPROCAMP_SATURATION, lNewValue);
	}

	HRESULT GetSaturation(LONG *plValue, LONG *plFlags)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		return GetVideoProcAmpParameter(KSPROPERTY_VIDEOPROCAMP_SATURATION, plValue, plFlags);
	}

	HRESULT SetExposure(LONG lNewValue)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		if (!m_pCameraControl) return E_POINTER;

		return m_pCameraControl->Set(KSPROPERTY_CAMERACONTROL_EXPOSURE, lNewValue, KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL);
	}

	HRESULT GetExposure(LONG *plValue, LONG *plFlags)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		if (!m_pCameraControl) return E_POINTER;

		HRESULT hr = E_FAIL;

		//
		// Note: lValue will be valid only when hr = S_OK. For exposure, negative values are still valid values.
		//
		hr = m_pCameraControl->Get(KSPROPERTY_CAMERACONTROL_EXPOSURE, plValue, plFlags);

		EventWriteNumberInfo(M, FL, FN, L"Value", *plValue);
		EventWriteHexInfo(M, FL, FN, L"Flags", *plFlags);
		EventWriteHexInfo(M, FL, FN, L"Status", hr);

		return hr;
	}

	HRESULT SetFlash(CameraFlashEnum newValue)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		if (!m_pIKsControl) return E_POINTER;

		HRESULT hr = E_FAIL;

		if (m_pIKsControl)
		{
			ULONG cbRet = 0;

			KSPROPERTY ksProp;
			ksProp.Set = PROPSETID_VIDCAP_CAMERACONTROL_FLASH;
			ksProp.Id = KSPROPERTY_CAMERACONTROL_FLASH_PROPERTY_ID;
			ksProp.Flags = KSPROPERTY_TYPE_SET;

			KSPROPERTY_CAMERACONTROL_FLASH_S ksPropFlash;
			ZeroMemory(&ksPropFlash, sizeof(ksPropFlash));

			switch (newValue)
			{
			case CameraFlashOff:
				ksPropFlash.Flash = KSPROPERTY_CAMERACONTROL_FLASH_OFF;
				break;
			case CameraFlashOn:
				ksPropFlash.Flash = KSPROPERTY_CAMERACONTROL_FLASH_ON;
				break;
			case CameraFlashAuto:
				ksPropFlash.Flash = KSPROPERTY_CAMERACONTROL_FLASH_AUTO;
				break;
			default:
				ksPropFlash.Flash = KSPROPERTY_CAMERACONTROL_FLASH_AUTO;
				break;
			}

			ksPropFlash.Capabilities = (newValue == CameraFlashAuto)
				? KSPROPERTY_CAMERACONTROL_FLASH_FLAGS_AUTO
				: KSPROPERTY_CAMERACONTROL_FLASH_FLAGS_MANUAL;

			hr = m_pIKsControl->KsProperty(&ksProp, sizeof(ksProp), (LPVOID)&ksPropFlash, sizeof(ksPropFlash), &cbRet);
		}

		return hr;
	}

	HRESULT GetFlash(CameraFlashEnum *pValue, LONG *plFlags)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		if (!m_pIKsControl) return E_POINTER;

		HRESULT hr = E_FAIL;

		if (m_pIKsControl)
		{
			ULONG cbRet = 0;

			KSPROPERTY ksProp;
			ksProp.Set = PROPSETID_VIDCAP_CAMERACONTROL_FLASH;
			ksProp.Id = KSPROPERTY_CAMERACONTROL_FLASH_PROPERTY_ID;
			ksProp.Flags = KSPROPERTY_TYPE_GET;

			KSPROPERTY_CAMERACONTROL_FLASH_S ksPropFlash;
			ZeroMemory(&ksPropFlash, sizeof(ksPropFlash));

			hr = m_pIKsControl->KsProperty(&ksProp, sizeof(ksProp), (LPVOID)&ksPropFlash, sizeof(ksPropFlash), &cbRet);

			if (SUCCEEDED(hr))
			{
				switch (ksPropFlash.Flash)
				{
				case KSPROPERTY_CAMERACONTROL_FLASH_OFF:
					*pValue = CameraFlashOff;
					break;

				case KSPROPERTY_CAMERACONTROL_FLASH_ON:
					*pValue = CameraFlashOn;
					break;

				case KSPROPERTY_CAMERACONTROL_FLASH_AUTO:
					*pValue = CameraFlashAuto;
					break;
				}

				*plFlags = (LONG)ksPropFlash.Capabilities;
			}
		}

		return hr;
	}

	HRESULT GetBrightnessRange(LONG *plMin, LONG *plMax, LONG *plDelta, LONG *plDefault, LONG *plCaps)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		return GetVideoProcAmpParameterRange(KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS, plMin, plMax, plDelta, plDefault, plCaps);
	}

	HRESULT GetContrastRange(LONG *plMin, LONG *plMax, LONG *plDelta, LONG *plDefault, LONG *plCaps)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		return GetVideoProcAmpParameterRange(KSPROPERTY_VIDEOPROCAMP_CONTRAST, plMin, plMax, plDelta, plDefault, plCaps);
	}

	HRESULT GetSaturationRange(LONG *plMin, LONG *plMax, LONG *plDelta, LONG *plDefault, LONG *plCaps)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		return GetVideoProcAmpParameterRange(KSPROPERTY_VIDEOPROCAMP_SATURATION, plMin, plMax, plDelta, plDefault, plCaps);
	}

	HRESULT GetExposureRange(LONG *plMin, LONG *plMax, LONG *plDelta, LONG *plDefault, LONG *plCaps)
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;
		if (!m_pCameraControl) return E_POINTER;

		HRESULT hr = E_FAIL;

		//
		// Note: lValue will be valid only when hr = S_OK. For exposure, negative values are still valid values.
		//
		return m_pCameraControl->GetRange(CameraControl_Exposure, plMin, plMax, plDelta, plDefault, plCaps);
	}

	HRESULT GetCameraProvider(wchar_t *pszFriendlyName, wchar_t *pszProvider, DWORD cchProvSize)
	{
		HDEVINFO hDevInfo;
		DWORD dwItr = 0;
		HRESULT hrReturn = E_FAIL;
		SP_DEVINFO_DATA	sDeviceInfoData;
		wchar_t szProperty[MAX_PATH];

		hDevInfo = SetupDiGetClassDevs(NULL, 0, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES);
		if (hDevInfo == INVALID_HANDLE_VALUE) return E_FAIL;

		sDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		while (TRUE)
		{
			if (SetupDiEnumDeviceInfo(hDevInfo, dwItr, &sDeviceInfoData) == FALSE) break;

			dwItr++;

			HRESULT hr = GetDeviceRegistryPropertyString(
				hDevInfo,
				&sDeviceInfoData,
				SPDRP_FRIENDLYNAME,
				szProperty, MAX_PATH);

			if (FAILED(hr)) continue;

			if (_wcsicmp(szProperty, pszFriendlyName) != 0) continue;

			hr = GetDeviceRegistryPropertyString(hDevInfo, &sDeviceInfoData, SPDRP_MFG, szProperty, MAX_PATH);

			if (SUCCEEDED(hr))
			{
				(void)StringCchCopy(pszProvider, cchProvSize, szProperty);
				hrReturn = S_OK;
				break;
			}
		}

		return	hrReturn;
	}

	HRESULT DumpCameraInfo(wchar_t *pszFriendlyName)
	{
		IBaseFilter **ppSrcFilter = &m_pSrcFilter;
		HRESULT hr = S_OK;

		IBaseFilter *pSrc = NULL;
		IMoniker *pMoniker = NULL;
		ICreateDevEnum *pDevEnum = NULL;
		IEnumMoniker *pClassEnum = NULL;
		ICaptureGraphBuilder2 *pCapGraphBuilder2 = NULL;
		IAMStreamConfig *pStreamConf = NULL;
		BOOL bDeviceFound = TRUE;
		VARIANT varName;
		VARIANT varName2;

		if (!ppSrcFilter) return E_POINTER;

		do
		{
			//
			// Create the system device enumerator
			//
			hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void**)&pDevEnum);
			if (FAILED(hr)) return hr;

			//
			// Create an enumerator for the video capture devices
			//
			hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);

			if (FAILED(hr))
			{
				PrintComError(hr, L"CreateClassEnumerator");
				break;
			}

			//
			// If there are no enumerators for the requested type, then CreateClassEnumerator will succeed,
			// but pClassEnum will be NULL.
			//
			if (pClassEnum == NULL)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			//
			// Use the first video capture device on the device list. Note that if the Next() call succeeds but there are no
			// monikers, it will return S_FALSE (which is not a failure). Therefore, we check that the return code is S_OK
			// instead of using SUCCEEDED() macro.
			//
			(void)pClassEnum->Reset();
			ULONG cFetched;

			while (hr = pClassEnum->Next(1, &pMoniker, &cFetched), hr == S_OK)
			{
				IPropertyBag *pPropBag = NULL;
				HRESULT hrTmp = E_FAIL;

#pragma warning(suppress: 6387)
				hrTmp = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);

				if (FAILED(hrTmp))
				{
					SAFE_RELEASE(pMoniker);
					continue;
				}

				VariantInit(&varName);
				VariantInit(&varName2);

				hrTmp = pPropBag->Read(L"FriendlyName", &varName, 0);

				if (FAILED(hrTmp))
				{
					VariantClear(&varName);
					VariantClear(&varName2);
					SAFE_RELEASE(pPropBag);
					continue;
				}

				hrTmp = pPropBag->Read(L"DevicePath", &varName2, 0);

				if (FAILED(hrTmp))
				{
					VariantClear(&varName);
					VariantClear(&varName2);
					SAFE_RELEASE(pPropBag);
					continue;
				}

				if (varName.bstrVal == NULL || varName2.bstrVal == NULL)
				{
					VariantClear(&varName);
					VariantClear(&varName2);
					SAFE_RELEASE(pPropBag);
					continue;
				}

				if (_wcsicmp(pszFriendlyName, varName.bstrVal) != 0)
				{
					VariantClear(&varName);
					VariantClear(&varName2);
					SAFE_RELEASE(pPropBag);
					continue;
				}
				
				VariantClear(&varName);
				VariantClear(&varName2);
				SAFE_RELEASE(pPropBag);

				bDeviceFound = TRUE;
				break;
			}

			if (bDeviceFound)
			{
#pragma warning(suppress: 6387)
				hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);

				if (FAILED(hr) || pSrc == NULL)
				{
					EventWriteHresultError(M, FL, FN, L"BindToObject", hr);
				}

				*ppSrcFilter = pSrc;

				hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void**)&pCapGraphBuilder2);
				
				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"CoCreateInstance_CLSID_CaptureGraphBuilder2", hr);
					break;
				}

				hr = pCapGraphBuilder2->FindInterface(&PIN_CATEGORY_CAPTURE, 0, m_pSrcFilter, IID_IAMStreamConfig, (void**)&pStreamConf);

				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"FindInterface_IID_IAMStreamConfig", hr);
					break;
				}

				LONG lWidth[256], lHeight[256];
				int iCount = 0, iSize = 0, iFormat;

				hr = pStreamConf->GetNumberOfCapabilities(&iCount, &iSize);
				if (SUCCEEDED(hr))
				{
					if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
					{
						for (iFormat = 0; iFormat < iCount; iFormat++)
						{
							VIDEO_STREAM_CONFIG_CAPS scc;
							AM_MEDIA_TYPE *pmtConf;

							hr = pStreamConf->GetStreamCaps(iFormat, &pmtConf, (BYTE*)&scc);
							if (SUCCEEDED(hr))
							{
								m_pVih2 = (VIDEOINFOHEADER*)pmtConf->pbFormat;

								if (m_pVih2->bmiHeader.biHeight && m_pVih2->bmiHeader.biWidth)
								{
									CHAR c1, c2, c3, c4;
									FOURCC_DMP(m_pVih2->bmiHeader.biCompression, &c1, &c2, &c3, &c4);
									
									UINT32 nume = 0, deno = 0;
									MFAverageTimePerFrameToFrameRate(m_pVih2->AvgTimePerFrame, &nume, &deno);
								}

								DeleteMediaType(pmtConf);
							}
						}
					}
				}
			}
			else
			{
				hr = E_FAIL;
			}
		}
		while (false);

		SAFE_RELEASE(pMoniker);
		SAFE_RELEASE(pStreamConf);
		SAFE_RELEASE(pCapGraphBuilder2);
		SAFE_RELEASE(pClassEnum);
		SAFE_RELEASE(pDevEnum);

		return hr;
	}

	HRESULT LaunchPropertiesFrame()
	{
		if (!m_bInitialized) return E_NOT_VALID_STATE;

		HRESULT hr = E_FAIL;

		if (m_pSrcFilter)
		{
			ISpecifyPropertyPages *pProp = NULL;

			hr = m_pSrcFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);

			if (SUCCEEDED(hr))
			{
				//
				// Get the filter's name and IUnknown pointer.
				//
				FILTER_INFO filterInfo;

				hr = m_pSrcFilter->QueryFilterInfo(&filterInfo);

				IUnknown *pFilterUnk = NULL;

				m_pSrcFilter->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);

				//
				// Show the property page. 
				//
				CAUUID caGUID;

				pProp->GetPages(&caGUID);
				pProp->Release();

				hr = OleCreatePropertyFrame(
					NULL,                   // Parent window
					0, 0,                   // Reserved
					filterInfo.achName,     // Caption for the dialog box
					1,                      // Number of objects (just the filter)
					&pFilterUnk,            // Array of object pointers. 
					caGUID.cElems,          // Number of property pages
					caGUID.pElems,          // Array of property page CLSIDs
					0,                      // Locale identifier
					0, NULL);				// Reserved	

				if (FAILED(hr))
				{
					EventWriteHresultError(M, FL, FN, L"HrError OleCreatePropertyFrame", hr);
				}

				//
				// Clean up.
				//
				if (pFilterUnk) pFilterUnk->Release();
				if (filterInfo.pGraph) filterInfo.pGraph->Release();
				CoTaskMemFree(caGUID.pElems);
			}
		}

		return hr;
	}

	HRESULT CloseInterfaces()
	{
		if (m_bInitialized)
		{
			//
			// Stop previewing data
			//
			if (m_pMediaCtrl) m_pMediaCtrl->StopWhenReady();

			//
			// Relinquish ownership (IMPORTANT!) of the video window.
			// Failing to call put_Owner can lead to assert failures within
			// the video renderer, as it still assumes that it has a valid
			// parent window.
			//

			//
			// Release DirectShow interfaces
			//
			SAFE_RELEASE(m_pIKsControl);
			SAFE_RELEASE(m_pMediaCtrl);
			SAFE_RELEASE(m_pStrmConfig);
			SAFE_RELEASE(m_pGraphBldr);
			SAFE_RELEASE(m_pCapGraphBldr);
			SAFE_RELEASE(m_pSampleGrabberFilter);
			SAFE_RELEASE(m_pCameraControl);
			SAFE_RELEASE(m_pVideoProcAmp);
			SAFE_RELEASE(m_pSrcFilter);

			m_bInitialized = FALSE;
		}

		return S_OK;
	}

private:
	HRESULT SetVideoProcAmpParameter(LONG lProperty, LONG lValue)
	{
		if (!m_pVideoProcAmp) return E_POINTER;
		return m_pVideoProcAmp->Set(lProperty, lValue, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL);
	}

	HRESULT GetVideoProcAmpParameter(LONG lProperty, LONG *plValue, LONG *plFlags)
	{
		if (!m_pVideoProcAmp) return E_POINTER;
		return m_pVideoProcAmp->Get(lProperty, plValue, plFlags);
	}

	HRESULT GetVideoProcAmpParameterRange(LONG lProperty, LONG *plMin, LONG *plMax, LONG *plDelta, LONG *plDefault, LONG *plFlags)
	{
		if (!m_pVideoProcAmp) return E_POINTER;
		return m_pVideoProcAmp->GetRange(lProperty, plMin, plMax, plDelta, plDefault, plFlags);
	}

	HRESULT FindCaptureDevice(wchar_t *pszFriendlyName)
	{
		IBaseFilter **ppSrcFilter = &m_pSrcFilter;
		HRESULT hr = S_OK;
		int iNumVidList = 0;

		IBaseFilter *pSrc = NULL;
		IMoniker* pMoniker = NULL;
		ICreateDevEnum *pDevEnum = NULL;
		IEnumMoniker *pClassEnum = NULL;

		BOOL bDeviceFound = FALSE;
		VARIANT varName;
		VARIANT varName2;

		if (!ppSrcFilter) return E_POINTER;

		//
		// Create the system device enumerator
		//
		hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void**)&pDevEnum);
		if (FAILED(hr)) return hr;

		//
		// Create an enumerator for the video capture devices
		//
		hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
		if (FAILED(hr))
		{
			SAFE_RELEASE(pDevEnum);
			return hr;
		}

		//
		// If there are no enumerators for the requested type, then CreateClassEnumerator will succeed,
		// but pClassEnum will be NULL.
		//
		if (pClassEnum == NULL)
		{
			SAFE_RELEASE(pClassEnum);
			SAFE_RELEASE(pDevEnum);
			hr = E_FAIL;
			return hr;
		}

		//
		// Use the first video capture device on the device list. Note that if the Next() call succeeds but there are no
		// monikers, it will return S_FALSE (which is not a failure). Therefore, we check that the return code is S_OK
		// instead of using SUCCEEDED() macro.
		//
		pClassEnum->Reset();
		ULONG cFetched;

		while (hr = pClassEnum->Next(1, &pMoniker, &cFetched), hr == S_OK)
		{
			IPropertyBag *pPropBag = NULL;
			HRESULT result = E_FAIL;

#pragma warning(suppress: 6387)
			result = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
			if (FAILED(result))
			{
				SAFE_RELEASE(pMoniker);
				continue;
			}

			VariantInit(&varName);
			VariantInit(&varName2);

			result = pPropBag->Read(L"FriendlyName", &varName, 0);
			if (FAILED(result))
			{
				VariantClear(&varName);
				VariantClear(&varName2);
				SAFE_RELEASE(pPropBag);
				continue;
			}

			result = pPropBag->Read(L"DevicePath", &varName2, 0);
			if (FAILED(result))
			{
				VariantClear(&varName);
				VariantClear(&varName2);
				SAFE_RELEASE(pPropBag);
				continue;
			}

			if (varName.bstrVal == NULL || varName2.bstrVal == NULL)
			{
				VariantClear(&varName);
				VariantClear(&varName2);
				SAFE_RELEASE(pPropBag);
				continue;
			}

			if (_wcsicmp(pszFriendlyName, varName.bstrVal) != 0)
			{
				VariantClear(&varName);
				VariantClear(&varName2);
				SAFE_RELEASE(pPropBag);
				continue;
			}

			VariantClear(&varName);
			VariantClear(&varName2);
			SAFE_RELEASE(pPropBag);

			bDeviceFound = TRUE;
			break;
		}

		SAFE_RELEASE(pClassEnum);
		SAFE_RELEASE(pDevEnum);

		if (bDeviceFound)
		{
#pragma warning(suppress: 6387)
			hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);

			if (FAILED(hr))
			{
				EventWriteHresultError(M, FL, FN, L"BindToObject", hr);
			}

			*ppSrcFilter = pSrc;

			hr = m_pSrcFilter->QueryInterface(IID_IAMCameraControl, (void**)&m_pCameraControl);

			if (hr != S_OK)
			{
				m_pCameraControl = NULL;
			}

			hr = m_pSrcFilter->QueryInterface(IID_IAMVideoProcAmp, (void**)&m_pVideoProcAmp);

			if (hr != S_OK)
			{
				m_pVideoProcAmp = NULL;
			}

			hr = m_pSrcFilter->QueryInterface(IID_IKsControl, (void**)&m_pIKsControl);

			if (hr != S_OK)
			{
				m_pIKsControl = NULL;
			}
		}
		else
		{
			hr = E_FAIL;
		}

		SAFE_RELEASE(pMoniker);

		return hr;
	}

	HRESULT GetInterfaces()
	{
		HRESULT hr = S_OK;

		hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&m_pGraphBldr);

		if (FAILED(hr))
		{
			return hr;
		}

		//
		// Create the capture graph builder
		//
		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void**)&m_pCapGraphBldr);

		if (FAILED(hr))
		{
			return hr;
		}

		//
		// Obtain interfaces for media control and Video Window
		//
		hr = m_pGraphBldr->QueryInterface(IID_IMediaControl, (LPVOID*)&m_pMediaCtrl);

		if (FAILED(hr))
		{
			m_pMediaCtrl = NULL;
			return hr;
		}

		hr = m_pCapGraphBldr->FindInterface(&PIN_CATEGORY_CAPTURE, 0, m_pSrcFilter, IID_IAMStreamConfig, (void**)&m_pStrmConfig);

		if (FAILED(hr))
		{
			m_pStrmConfig = NULL;
			return hr;
		}

		return hr;
	}

	HRESULT GetDeviceRegistryPropertyString(
		HDEVINFO hDeviceInfoSet,
		PSP_DEVINFO_DATA pDeviceInfoData,
		DWORD dwProperty,
		wchar_t *pszText,
		DWORD cchTextSize)
	{
		BOOL bStatus = FALSE;
		DWORD dwDataType;
		DWORD dwDataSize;
		BYTE* pData;

		if (pszText == NULL) return	E_POINTER;

		pData = NULL;
		dwDataSize = 0;

		while (TRUE)
		{
			bStatus = SetupDiGetDeviceRegistryProperty(
				hDeviceInfoSet,
				pDeviceInfoData,
				dwProperty,
				&dwDataType,
				pData,
				dwDataSize,
				&dwDataSize);

			if (bStatus || GetLastError() != ERROR_INSUFFICIENT_BUFFER) break;

			if (pData) delete[] pData;

			pData = new BYTE[dwDataSize];
		}

		if (bStatus && pData)
		{
			if (dwDataType == REG_EXPAND_SZ || dwDataType == REG_MULTI_SZ || dwDataType == REG_SZ)
			{
				StringCchCopy(pszText, cchTextSize, (wchar_t*)pData);
			}
		}

		if (pData) delete[] pData;

		return (bStatus && pData) ? S_OK : E_FAIL;
	}

	void FreeMediaType(AM_MEDIA_TYPE& mt)
	{
		if (mt.cbFormat != 0)
		{
			CoTaskMemFree((PVOID)mt.pbFormat);
			mt.cbFormat = 0;
			mt.pbFormat = NULL;
		}

		SAFE_RELEASE(mt.pUnk);
	}

	void DeleteMediaType(AM_MEDIA_TYPE *pmt)
	{
		if (pmt)
		{
			FreeMediaType(*pmt);
			CoTaskMemFree(pmt);
			pmt = NULL;
		}
	}

private:
	IBaseFilter *m_pSrcFilter;
	IGraphBuilder *m_pGraphBldr;
	ICaptureGraphBuilder2 *m_pCapGraphBldr;
	IAMCameraControl *m_pCameraControl;
	IAMVideoProcAmp *m_pVideoProcAmp;
	IMediaControl *m_pMediaCtrl;
	IAMStreamConfig *m_pStrmConfig;
	IBaseFilter *m_pSampleGrabberFilter;
	IKsControl *m_pIKsControl;
	VIDEOINFOHEADER *m_pVih2;
	BOOL m_bInitialized;
	LONG m_lRefCount;
};

//
// Our exported CameraDs factory function.
//
_declspec(dllexport) HRESULT CreateCameraDsInstance(ICameraDs **ppObj)
{
	if (ppObj == NULL) return E_POINTER;

	ICameraDs *pObj = new (std::nothrow) CameraDs();

	if (pObj == NULL) return E_OUTOFMEMORY;

	*ppObj = pObj;

	(*ppObj)->AddRef();

	return S_OK;
}
