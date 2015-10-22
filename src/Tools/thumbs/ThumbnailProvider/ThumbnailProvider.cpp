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

#include "Common.h"
#include "ThumbnailProvider.h"

#include <iostream>
#include <zipios++/zipinputstream.h>
#include <zipios++/zipfile.h>
#include <CxImage/xmemfile.h>
#include <CxImage/ximapng.h>

CThumbnailProvider::CThumbnailProvider()
{
    DllAddRef();
    m_cRef = 1;
    m_pSite = NULL;
}


CThumbnailProvider::~CThumbnailProvider()
{
    if (m_pSite)
    {
        m_pSite->Release();
        m_pSite = NULL;
    }
    DllRelease();
}


STDMETHODIMP CThumbnailProvider::QueryInterface(REFIID riid,
                                                void** ppvObject)
{
    static const QITAB qit[] = 
    {
      //QITABENT(CThumbnailProvider, IInitializeWithStream),
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
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}


STDMETHODIMP CThumbnailProvider::Initialize(IStream *pstm, 
                                            DWORD grfMode)
{
    return S_OK;
}

STDMETHODIMP CThumbnailProvider::Initialize(LPCWSTR pszFilePath, 
                                            DWORD grfMode)
{
    wcscpy_s(m_szFile, pszFilePath); 
    return S_OK;
}

bool CThumbnailProvider::CheckZip() const
{
    // open file and check magic number (PK\x03\x04)
    std::ifstream zip(m_szFile, std::ios::in | std::ios::binary);
    unsigned char pk[4] = {0x50, 0x4b, 0x03, 0x04};
    for (int i=0; i<4; i++) {
        unsigned char c;
        if (!zip.get((char&)c))
            return false;
        if (c != pk[i])
            return false;
    }

    return true;
}

STDMETHODIMP CThumbnailProvider::GetThumbnail(UINT cx, 
                                              HBITMAP *phbmp, 
                                              WTS_ALPHATYPE *pdwAlpha)
{
    try {
        // first make sure we have a zip file but that might still be invalid
        if (!CheckZip())
            return NOERROR;

        std::ifstream file(m_szFile, std::ios::in | std::ios::binary);
        zipios::ZipInputStream zipstream(file);
        zipios::ConstEntryPointer entry;
        entry = zipstream.getNextEntry();
        while (entry->isValid() && entry->getName() != "thumbnails/Thumbnail.png")
            entry = zipstream.getNextEntry();
        if (entry && entry->isValid()) {
            // ok, we have found the file. Now, read it in byte for byte
            std::istream *str = &zipstream;
            std::vector<unsigned char> content;
            unsigned char c;
            while (str->get((char&)c)) {
                content.push_back(c);
            }

            // pass the memory buffer to CxImage library to create the bitmap handle
            CxMemFile mem(&(content[0]),content.size());
            CxImagePNG png;
            png.Decode(&mem);
            *phbmp = png.MakeBitmap();
            *pdwAlpha = WTSAT_UNKNOWN;
        }
    }
    catch(...) {
        // This may happen if the file is corrupted, not a valid zip file
        // or whatever could go wrong
    }

    return NOERROR; 
}


STDMETHODIMP CThumbnailProvider::GetSite(REFIID riid, 
                                         void** ppvSite)
{
    if (m_pSite)
    {
        return m_pSite->QueryInterface(riid, ppvSite);
    }
    return E_NOINTERFACE;
}


STDMETHODIMP CThumbnailProvider::SetSite(IUnknown* pUnkSite)
{
    if (m_pSite)
    {
        m_pSite->Release();
        m_pSite = NULL;
    }

    m_pSite = pUnkSite;
    if (m_pSite)
    {
        m_pSite->AddRef();
    }
    return S_OK;
}


STDAPI CThumbnailProvider_CreateInstance(REFIID riid, void** ppvObject)
{
    *ppvObject = NULL;

    CThumbnailProvider* ptp = new CThumbnailProvider();
    if (!ptp)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = ptp->QueryInterface(riid, ppvObject);
    ptp->Release();
    return hr;
}
