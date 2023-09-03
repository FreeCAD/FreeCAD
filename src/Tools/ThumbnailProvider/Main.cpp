/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer@users.sourceforge.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#define INITGUID
#include "Common.h"

HINSTANCE g_hinstDll = NULL;
LONG g_cRef = 0;


typedef struct _REGKEY_DELETEKEY
{
    HKEY hKey;
    LPCWSTR lpszSubKey;
} REGKEY_DELETEKEY;


typedef struct _REGKEY_SUBKEY_AND_VALUE
{
    HKEY hKey;
    LPCWSTR lpszSubKey;
    LPCWSTR lpszValue;
    DWORD dwType;
    DWORD_PTR dwData;
    ;
} REGKEY_SUBKEY_AND_VALUE;

STDAPI CreateRegistryKeys(REGKEY_SUBKEY_AND_VALUE* aKeys, ULONG cKeys);
STDAPI DeleteRegistryKeys(REGKEY_DELETEKEY* aKeys, ULONG cKeys);


BOOL APIENTRY DllMain(HINSTANCE hinstDll, DWORD dwReason, LPVOID pvReserved)
{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_hinstDll = hinstDll;
            break;
    }
    return TRUE;
}

STDAPI_(HINSTANCE) DllInstance()
{
    return g_hinstDll;
}

STDAPI DllCanUnloadNow()
{
    return g_cRef ? S_FALSE : S_OK;
}

STDAPI_(ULONG) DllAddRef()
{
    LONG cRef = InterlockedIncrement(&g_cRef);
    return cRef;
}

STDAPI_(ULONG) DllRelease()
{
    LONG cRef = InterlockedDecrement(&g_cRef);
    if (0 > cRef) {
        cRef = 0;
    }
    return cRef;
}

STDAPI DllRegisterServer()
{
    // This tells the shell to invalidate the thumbnail cache.  This is important because any
    // .recipe files viewed before registering this handler would otherwise show cached blank
    // thumbnails.
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    WCHAR szModule[MAX_PATH];

    ZeroMemory(szModule, sizeof(szModule));
    GetModuleFileName(g_hinstDll, szModule, ARRAYSIZE(szModule));

    // uncomment the following
    REGKEY_SUBKEY_AND_VALUE keys[] = {
        {HKEY_CLASSES_ROOT,
         L"CLSID\\" szCLSID_SampleThumbnailProvider,
         NULL,
         REG_SZ,
         (DWORD_PTR)L"FCStd Thumbnail Provider"},
#if 1
        //{HKEY_CLASSES_ROOT, L"CLSID\\DisableProcessIsolation", NULL, REG_DWORD, (DWORD) 1},
        {HKEY_CLASSES_ROOT,
         L"CLSID\\" szCLSID_SampleThumbnailProvider,
         L"DisableProcessIsolation",
         REG_DWORD,
         (DWORD)1},
#endif
        {HKEY_CLASSES_ROOT,
         L"CLSID\\" szCLSID_SampleThumbnailProvider L"\\InprocServer32",
         NULL,
         REG_SZ,
         (DWORD_PTR)szModule},
        {HKEY_CLASSES_ROOT,
         L"CLSID\\" szCLSID_SampleThumbnailProvider L"\\InprocServer32",
         L"ThreadingModel",
         REG_SZ,
         (DWORD_PTR)L"Apartment"},
        //{HKEY_CLASSES_ROOT, L".FCStd\\shellex", L"Trick only here to create shellex when not
        // existing",REG_DWORD, 1},
        {HKEY_CLASSES_ROOT,
         L".FCStd\\shellex\\{E357FCCD-A995-4576-B01F-234630154E96}",
         NULL,
         REG_SZ,
         (DWORD_PTR)szCLSID_SampleThumbnailProvider},
        {HKEY_CLASSES_ROOT,
         L".FCBak\\shellex\\{E357FCCD-A995-4576-B01F-234630154E96}",
         NULL,
         REG_SZ,
         (DWORD_PTR)szCLSID_SampleThumbnailProvider}
    };

    return CreateRegistryKeys(keys, ARRAYSIZE(keys));
}

STDAPI DllUnregisterServer()
{
    REGKEY_DELETEKEY keys[] = {{HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_SampleThumbnailProvider}};
    return DeleteRegistryKeys(keys, ARRAYSIZE(keys));
}


STDAPI CreateRegistryKey(REGKEY_SUBKEY_AND_VALUE* pKey)
{
    size_t cbData;
    LPVOID pvData = NULL;
    HRESULT hr = S_OK;

    switch (pKey->dwType) {
        case REG_DWORD:
            pvData = (LPVOID)(LPDWORD)&pKey->dwData;
            cbData = sizeof(DWORD);
            break;

        case REG_SZ:
        case REG_EXPAND_SZ:
            hr = StringCbLength((LPCWSTR)pKey->dwData, STRSAFE_MAX_CCH, &cbData);
            if (SUCCEEDED(hr)) {
                pvData = (LPVOID)(LPCWSTR)pKey->dwData;
                cbData += sizeof(WCHAR);
            }
            break;

        default:
            hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr)) {
        LSTATUS status = SHSetValue(pKey->hKey,
                                    pKey->lpszSubKey,
                                    pKey->lpszValue,
                                    pKey->dwType,
                                    pvData,
                                    (DWORD)cbData);
        if (NOERROR != status) {
            hr = HRESULT_FROM_WIN32(status);
        }
    }

    return hr;
}


STDAPI CreateRegistryKeys(REGKEY_SUBKEY_AND_VALUE* aKeys, ULONG cKeys)
{
    HRESULT hr = S_OK;

    for (ULONG iKey = 0; iKey < cKeys; iKey++) {
        HRESULT hrTemp = CreateRegistryKey(&aKeys[iKey]);
        if (FAILED(hrTemp)) {
            hr = hrTemp;
        }
    }
    return hr;
}


STDAPI DeleteRegistryKeys(REGKEY_DELETEKEY* aKeys, ULONG cKeys)
{
    HRESULT hr = S_OK;
    LSTATUS status;

    for (ULONG iKey = 0; iKey < cKeys; iKey++) {
        status = RegDeleteTree(aKeys[iKey].hKey, aKeys[iKey].lpszSubKey);
        if (NOERROR != status) {
            hr = HRESULT_FROM_WIN32(status);
        }
    }
    return hr;
}
