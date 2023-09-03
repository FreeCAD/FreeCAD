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
#include "ClassFactory.h"
#include "Common.h"

STDAPI CThumbnailProvider_CreateInstance(REFIID riid, void** ppvObject);


CClassFactory::CClassFactory()
{
    m_cRef = 1;
    DllAddRef();
}


CClassFactory::~CClassFactory()
{
    DllRelease();
}


STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppvObject)
{
    static const QITAB qit[] = {
        QITABENT(CClassFactory, IClassFactory),
        {0},
    };
    return QISearch(this, qit, riid, ppvObject);
}


STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    LONG cRef = InterlockedIncrement(&m_cRef);
    return (ULONG)cRef;
}


STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef) {
        delete this;
    }
    return (ULONG)cRef;
}


STDMETHODIMP CClassFactory::CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppvObject)
{
    if (NULL != punkOuter) {
        return CLASS_E_NOAGGREGATION;
    }

    return CThumbnailProvider_CreateInstance(riid, ppvObject);
}


STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (NULL == ppv) {
        return E_INVALIDARG;
    }

    if (!IsEqualCLSID(CLSID_SampleThumbnailProvider, rclsid)) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    CClassFactory* pcf;
    HRESULT hr;

    pcf = new CClassFactory();
    if (NULL == pcf) {
        return E_OUTOFMEMORY;
    }

    hr = pcf->QueryInterface(riid, ppv);
    pcf->Release();
    return hr;
}
