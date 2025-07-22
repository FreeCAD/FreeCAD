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


#pragma warning(disable : 4995)

#include "ThumbnailProvider.h"
#include "Common.h"

#include <iostream>
#include <wincodec.h>
#include <wincodecsdk.h>
#include <zipios++/zipfile.h>
#include <zipios++/zipinputstream.h>
#pragma comment(lib, "WindowsCodecs.lib")

// The functions
// * CreateStreamOnResource
// * LoadBitmapFromStream
// * CreateHBITMAP
// are taken from https://faithlife.codes/blog/2008/09/displaying_a_splash_screen_with_c_part_i/
// The code is released under an MIT-style license

// Creates a stream object initialized with the data from an executable resource.
IStream* CreateStreamOnResource(void* buffer, size_t length)
{
    // initialize return value
    IStream* ipStream = NULL;

    // allocate memory to hold the resource data
    HGLOBAL hgblResourceData = GlobalAlloc(GMEM_MOVEABLE, length);
    if (hgblResourceData == NULL) {
        goto Return;
    }

    // get a pointer to the allocated memory
    LPVOID pvResourceData = GlobalLock(hgblResourceData);
    if (pvResourceData == NULL) {
        goto FreeData;
    }

    // copy the data from the resource to the new memory block
    CopyMemory(pvResourceData, buffer, length);
    GlobalUnlock(hgblResourceData);

    // create a stream on the HGLOBAL containing the data

    if (SUCCEEDED(CreateStreamOnHGlobal(hgblResourceData, TRUE, &ipStream))) {
        goto Return;
    }

FreeData:
    // couldn't create stream; free the memory

    GlobalFree(hgblResourceData);

Return:
    // no need to unlock or free the resource
    return ipStream;
}

IWICBitmapSource* LoadBitmapFromStream(IStream* ipImageStream)
{
    // initialize return value
    IWICBitmapSource* ipBitmap = NULL;

    // load WIC's PNG decoder
    IWICBitmapDecoder* ipDecoder = NULL;
    if (FAILED(CoCreateInstance(CLSID_WICPngDecoder,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                __uuidof(ipDecoder),
                                reinterpret_cast<void**>(&ipDecoder)))) {
        goto Return;
    }

    // load the PNG
    if (FAILED(ipDecoder->Initialize(ipImageStream, WICDecodeMetadataCacheOnLoad))) {
        goto ReleaseDecoder;
    }

    // check for the presence of the first frame in the bitmap
    UINT nFrameCount = 0;
    if (FAILED(ipDecoder->GetFrameCount(&nFrameCount)) || nFrameCount != 1) {
        goto ReleaseDecoder;
    }

    // load the first frame (i.e., the image)
    IWICBitmapFrameDecode* ipFrame = NULL;
    if (FAILED(ipDecoder->GetFrame(0, &ipFrame))) {
        goto ReleaseDecoder;
    }

    // convert the image to 32bpp BGRA format with pre-multiplied alpha
    //   (it may not be stored in that format natively in the PNG resource,
    //   but we need this format to create the DIB to use on-screen)
    WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, ipFrame, &ipBitmap);
    ipFrame->Release();

ReleaseDecoder:
    ipDecoder->Release();
Return:
    return ipBitmap;
}

HBITMAP CreateHBITMAP(IWICBitmapSource* ipBitmap)
{
    // initialize return value
    HBITMAP hbmp = NULL;

    // get image attributes and check for valid image
    UINT width = 0;
    UINT height = 0;
    if (FAILED(ipBitmap->GetSize(&width, &height)) || width == 0 || height == 0) {
        goto Return;
    }

    // prepare structure giving bitmap information (negative height indicates a top-down DIB)
    BITMAPINFO bminfo;
    ZeroMemory(&bminfo, sizeof(bminfo));
    bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bminfo.bmiHeader.biWidth = width;
    bminfo.bmiHeader.biHeight = -((LONG)height);
    bminfo.bmiHeader.biPlanes = 1;
    bminfo.bmiHeader.biBitCount = 32;
    bminfo.bmiHeader.biCompression = BI_RGB;

    // create a DIB section that can hold the image
    void* pvImageBits = NULL;
    HDC hdcScreen = GetDC(NULL);
    hbmp = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0);
    ReleaseDC(NULL, hdcScreen);
    if (hbmp == NULL) {
        goto Return;
    }

    // extract the image into the HBITMAP

    const UINT cbStride = width * 4;
    const UINT cbImage = cbStride * height;
    if (FAILED(ipBitmap->CopyPixels(NULL, cbStride, cbImage, static_cast<BYTE*>(pvImageBits)))) {
        // couldn't extract image; delete HBITMAP

        DeleteObject(hbmp);
        hbmp = NULL;
    }

Return:
    return hbmp;
}

CThumbnailProvider::CThumbnailProvider()
{
    DllAddRef();
    m_cRef = 1;
    m_pSite = NULL;
}


CThumbnailProvider::~CThumbnailProvider()
{
    if (m_pSite) {
        m_pSite->Release();
        m_pSite = NULL;
    }
    DllRelease();
}


STDMETHODIMP CThumbnailProvider::QueryInterface(REFIID riid, void** ppvObject)
{
    static const QITAB qit[] = {
        // QITABENT(CThumbnailProvider, IInitializeWithStream),
        QITABENT(CThumbnailProvider, IInitializeWithFile),
        QITABENT(CThumbnailProvider, IThumbnailProvider),
        QITABENT(CThumbnailProvider, IObjectWithSite),
        {0},
    };
    return QISearch(this, qit, riid, ppvObject);
}


STDMETHODIMP_(ULONG) CThumbnailProvider::AddRef()
{
    LONG cRef = InterlockedIncrement(&m_cRef);
    return (ULONG)cRef;
}


STDMETHODIMP_(ULONG) CThumbnailProvider::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef) {
        delete this;
    }
    return (ULONG)cRef;
}


STDMETHODIMP CThumbnailProvider::Initialize(IStream* pstm, DWORD grfMode)
{
    return S_OK;
}

STDMETHODIMP CThumbnailProvider::Initialize(LPCWSTR pszFilePath, DWORD grfMode)
{
    wcscpy_s(m_szFile, pszFilePath);
    return S_OK;
}

bool CThumbnailProvider::CheckZip() const
{
    // open file and check magic number (PK\x03\x04)
    std::ifstream zip(m_szFile, std::ios::in | std::ios::binary);
    unsigned char pk[4] = {0x50, 0x4b, 0x03, 0x04};
    for (int i = 0; i < 4; i++) {
        unsigned char c;
        if (!zip.get((char&)c)) {
            return false;
        }
        if (c != pk[i]) {
            return false;
        }
    }

    return true;
}

STDMETHODIMP CThumbnailProvider::GetThumbnail(UINT cx, HBITMAP* phbmp, WTS_ALPHATYPE* pdwAlpha)
{
    try {
        // first make sure we have a zip file but that might still be invalid
        if (!CheckZip()) {
            return NOERROR;
        }

        std::ifstream file(m_szFile, std::ios::in | std::ios::binary);
        zipios::ZipInputStream zipstream(file);
        zipios::ConstEntryPointer entry;
        entry = zipstream.getNextEntry();
        while (entry->isValid() && entry->getName() != "thumbnails/Thumbnail.png") {
            entry = zipstream.getNextEntry();
        }
        if (entry && entry->isValid()) {
            // ok, we have found the file. Now, read it in byte for byte
            std::istream* str = &zipstream;
            std::vector<unsigned char> content;
            unsigned char c;
            while (str->get((char&)c)) {
                content.push_back(c);
            }

            // pass the memory buffer to an IStream to create the bitmap handle
            IStream* stream = CreateStreamOnResource(&(content[0]), content.size());
            if (stream) {
                IWICBitmapSource* bmpSrc = LoadBitmapFromStream(stream);
                stream->Release();
                if (bmpSrc) {
                    *phbmp = CreateHBITMAP(bmpSrc);
                    *pdwAlpha = WTSAT_UNKNOWN;
                    bmpSrc->Release();
                }
            }
        }
    }
    catch (...) {
        // This may happen if the file is corrupted, not a valid zip file
        // or whatever could go wrong
    }

    return NOERROR;
}


STDMETHODIMP CThumbnailProvider::GetSite(REFIID riid, void** ppvSite)
{
    if (m_pSite) {
        return m_pSite->QueryInterface(riid, ppvSite);
    }
    return E_NOINTERFACE;
}


STDMETHODIMP CThumbnailProvider::SetSite(IUnknown* pUnkSite)
{
    if (m_pSite) {
        m_pSite->Release();
        m_pSite = NULL;
    }

    m_pSite = pUnkSite;
    if (m_pSite) {
        m_pSite->AddRef();
    }
    return S_OK;
}


STDAPI CThumbnailProvider_CreateInstance(REFIID riid, void** ppvObject)
{
    *ppvObject = NULL;

    CThumbnailProvider* ptp = new CThumbnailProvider();
    if (!ptp) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = ptp->QueryInterface(riid, ppvObject);
    ptp->Release();
    return hr;
}
